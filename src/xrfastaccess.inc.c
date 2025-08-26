// NOTE: The simple "claim table" mechanism used for implementing LL/SC is
//       AWFUL! The cache simulation vastly outperforms it because of the
//       localization of the LL/SC state to the simulated L1 caches of each
//       virtual processor. Imitating this in a lighter-weight form that only
//       gets involved in the LL/SC codepaths is probably the best way to go
//       to make sure FASTMEMORY is actually FAST. Think of a "two-level claim
//       table".

XrClaimTableEntry XrClaimTable[XR_CLAIM_TABLE_SIZE];

#define XR_CLAIM_INDEX(phyaddr) (((phyaddr >> 2) ^ (phyaddr >> 12)) & (XR_CLAIM_TABLE_SIZE - 1))

#ifndef SINGLE_THREAD_MP

static XR_ALWAYS_INLINE uint32_t XrClaimAddress(XrProcessor *proc, uint32_t phyaddr, uint32_t *hostaddr) {
	XrClaimTableEntry *entry = &XrClaimTable[XR_CLAIM_INDEX(phyaddr)];

	XrLockMutex(&entry->Lock);

	entry->ClaimedBy = proc->Id;

	uint32_t val = *hostaddr;

	XrUnlockMutex(&entry->Lock);

	return val;
}

static XR_ALWAYS_INLINE int XrStoreIfClaimed(XrProcessor *proc, uint32_t phyaddr, uint32_t* hostaddr, uint32_t val) {
	XrClaimTableEntry *entry = &XrClaimTable[XR_CLAIM_INDEX(phyaddr)];

	if (entry->ClaimedBy != proc->Id) {
		return 0;
	}

	XrLockMutex(&entry->Lock);

	if (entry->ClaimedBy != proc->Id) {
		XrUnlockMutex(&entry->Lock);

		return 0;
	}

	*hostaddr = val;

	XrUnlockMutex(&entry->Lock);

	return 1;
}

#else

static XR_ALWAYS_INLINE uint32_t XrClaimAddress(XrProcessor *proc, uint32_t phyaddr, uint32_t *hostaddr) {
	XrClaimTableEntry *entry = &XrClaimTable[(phyaddr >> 2) & (XR_CLAIM_TABLE_SIZE - 1)];

	entry->ClaimedBy = proc->Id;

	return *hostaddr;
}

static XR_ALWAYS_INLINE int XrStoreIfClaimed(XrProcessor *proc, uint32_t phyaddr, uint32_t* hostaddr, uint32_t val) {
	XrClaimTableEntry *entry = &XrClaimTable[(phyaddr >> 2) & (XR_CLAIM_TABLE_SIZE - 1)];

	if (entry->ClaimedBy != proc->Id) {
		return 0;
	}

	*hostaddr = val;

	return 1;
}

#endif

static XR_ALWAYS_INLINE int XrLookupItb(XrProcessor *proc, uint32_t virtual, uint64_t *tbe) {
	uint32_t vpn = virtual >> 12;
	uint32_t matching = (proc->Cr[ITBTAG] & 0xFFF00000) | vpn;

	for (int i = 0; i < XR_ITB_SIZE; i++) {
		uint64_t tmp = proc->Itb[i];

		uint32_t mask = (tmp & PTE_GLOBAL) ? 0xFFFFF : 0xFFFFFFFF;

		if (((tmp >> 32) & mask) == (matching & mask)) {
			*tbe = tmp;

			return 1;
		}
	}

	// ITB miss!

	proc->Cr[ITBTAG] = matching;
	proc->Cr[ITBADDR] = (proc->Cr[ITBADDR] & 0xFFC00000) | (vpn << 2);
	proc->LastTbMissWasWrite = 0;

	if (XrLikely((proc->Cr[RS] & RS_TBMISS) == 0)) {
		XrPushMode(proc);
		proc->Cr[TBMISSADDR] = virtual;
		proc->Cr[TBPC] = proc->Pc;
		proc->Cr[RS] |= RS_TBMISS;
	}

	XrVectorException(proc, XR_EXC_ITB);

	return 0;
}

static XR_ALWAYS_INLINE int XrLookupDtb(XrProcessor *proc, uint32_t virtual, uint64_t *tbe, int writing) {
	uint32_t vpn = virtual >> 12;
	uint32_t matching = (proc->Cr[DTBTAG] & 0xFFF00000) | vpn;

	for (int i = 0; i < XR_DTB_SIZE; i++) {
		uint64_t tmp = proc->Dtb[i];

		uint32_t mask = (tmp & PTE_GLOBAL) ? 0xFFFFF : 0xFFFFFFFF;

		if (((tmp >> 32) & mask) == (matching & mask)) {
			*tbe = tmp;

			return i;
		}
	}

	// DTB miss!

	proc->Cr[DTBTAG] = matching;
	proc->Cr[DTBADDR] = (proc->Cr[DTBADDR] & 0xFFC00000) | (vpn << 2);
	proc->LastTbMissWasWrite = writing;

	if (XrLikely((proc->Cr[RS] & RS_TBMISS) == 0)) {
		XrPushMode(proc);
		proc->Cr[TBMISSADDR] = virtual;
		proc->Cr[TBPC] = proc->Pc;
		proc->Cr[RS] |= RS_TBMISS;
	}

	XrVectorException(proc, XR_EXC_DTB);

	return -1;
}

static XR_ALWAYS_INLINE void XrPageFault(XrProcessor *proc, uint32_t virtual, int writing) {
	if (XrUnlikely((proc->Cr[RS] & RS_TBMISS) != 0)) {
		// This page fault happened while handling a TB miss, which means
		// it was a fault on a page table. Clear the TBMISS flag from RS and
		// report the original missed address as the faulting address. Also,
		// set EPC to point to the instruction that caused the original TB
		// miss, so that the faulting PC is reported correctly.

		proc->Cr[EBADADDR] = proc->Cr[TBMISSADDR];
		proc->Cr[EPC] = proc->Cr[TBPC];
		proc->Cr[RS] &= ~RS_TBMISS;
		writing = proc->LastTbMissWasWrite;
	} else {
		proc->Cr[EBADADDR] = virtual;
		proc->Cr[EPC] = proc->Pc;
		XrPushMode(proc);
	}

	XrSetEcause(proc, writing ? XR_EXC_PGW : XR_EXC_PGF);
	XrVectorException(proc, writing ? XR_EXC_PGW : XR_EXC_PGF);
}

static XR_ALWAYS_INLINE int XrTranslateIstream(XrProcessor *proc, uint32_t virtual, uint32_t *phys, int *flags) {
	uint64_t tbe;
	uint32_t vpn = virtual >> 12;

	if (XrLikely(proc->ItbLastVpn == vpn)) {
		// This matches the last lookup, avoid searching the whole ITB.

		tbe = proc->ItbLastResult;
	} else if (XrUnlikely(!XrLookupItb(proc, virtual, &tbe))) {
		return 0;
	}

	if (XrUnlikely((tbe & PTE_VALID) == 0)) {
		// Not valid! Page fault time.

		XrPageFault(proc, virtual, 0);

		return 0;
	}

	if (XrUnlikely((tbe & PTE_KERNEL) && (proc->Cr[RS] & RS_USER))) {
		// Kernel mode page and we're in usermode! 

		proc->Cr[EBADADDR] = virtual;
		XrBasicException(proc, XR_EXC_PGF, proc->Pc);

		return 0;
	}

	proc->ItbLastVpn = vpn;
	proc->ItbLastResult = tbe;

	*flags = tbe & 31;
	*phys = ((tbe & 0x1FFFFE0) << 7) + (virtual & 0xFFF);

	//DBGPRINT("virt=%x phys=%x\n", virt, *phys);

	return 1;
}

static int XrTranslateDstream(XrProcessor *proc, uint32_t virtual, XrIblockDtbEntry *entry, int writing) {
	uint32_t vpn = virtual >> 12;

	uint64_t tbe;
	int index;

	if (vpn == proc->DtbLastVpn) {
		tbe = proc->DtbLastEntry.MatchingDtbe;
	} else {
		index = XrLookupDtb(proc, virtual, &tbe, writing);

		if (index == -1) {
			return 0;
		}
	}

	if (XrUnlikely((tbe & PTE_VALID) == 0)) {
		// Not valid! Page fault time.

		XrPageFault(proc, virtual, writing);

		return 0;
	}

	if (XrUnlikely((tbe & PTE_KERNEL) && (proc->Cr[RS] & RS_USER))) {
		// Kernel mode page and we're in usermode! 

		proc->Cr[EBADADDR] = virtual;
		XrBasicException(proc, writing ? XR_EXC_PGW : XR_EXC_PGF, proc->Pc);

		return 0;
	}

	if (XrUnlikely(writing && !(tbe & PTE_WRITABLE))) {
		// Write to non-writable page.

		proc->Cr[EBADADDR] = virtual;
		XrBasicException(proc, writing ? XR_EXC_PGW : XR_EXC_PGF, proc->Pc);

		return 0;
	}

	if (vpn != proc->DtbLastVpn) {
		proc->DtbLastEntry.MatchingDtbe = tbe;
		proc->DtbLastEntry.Index = index;
		proc->DtbLastEntry.HostPointer = EBusTranslate((tbe >> 5) << 12);
		proc->DtbLastVpn = vpn;

		entry->MatchingDtbe = tbe;
		entry->Index = index;
		entry->HostPointer = proc->DtbLastEntry.HostPointer;
	} else {
		entry->MatchingDtbe = tbe;
		entry->Index = proc->DtbLastEntry.Index;
		entry->HostPointer = proc->DtbLastEntry.HostPointer;

	}

	//DBGPRINT("virt=%x phys=%x\n", virt, *phys);

	return 1;
}

static uint32_t XrAccessMasks[5] = {
	0x00000000,
	0x000000FF,
	0x0000FFFF,
	0x00FFFFFF,
	0xFFFFFFFF
};

static XR_ALWAYS_INLINE int XrDirectEBusWrite(XrProcessor *proc, uint32_t address, uint32_t srcvalue, uint32_t length) {
	return EBusWrite(address, &srcvalue, length, proc) == EBUSSUCCESS;
}

static XR_ALWAYS_INLINE int XrDirectEBusRead(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t length) {
	int status = EBusRead(address, dest, length, proc) == EBUSSUCCESS;

	if (status) {
		*dest &= XrAccessMasks[length];
	}

	return status;
}

static XR_ALWAYS_INLINE int XrAccessWrite(XrProcessor *proc, XrIblock *iblock, uint32_t address, uint32_t srcvalue, uint32_t length, int sc) {
	if (XrUnlikely((address & (length - 1)) != 0)) {
		// Unaligned access.

		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_UNA, proc->Pc);

		return 0;
	}

	if (XrLikely((proc->Cr[RS] & RS_MMU) != 0)) {
		// Look up the DTB lookup cache in the Iblock.

		uint32_t matching = (proc->Cr[DTBTAG] & 0xFFF00000) | (address >> 12);

		int index = XR_IBLOCK_DTB_CACHE_INDEX(address);
		XrIblockDtbEntry *entry = &iblock->DtbStoreCache[index];

		if (XrUnlikely(((entry->MatchingDtbe >> 32) != matching) ||
			(proc->Dtb[entry->Index] != entry->MatchingDtbe) ||
			!(entry->MatchingDtbe & PTE_WRITABLE))) {

			if (!XrTranslateDstream(proc, address, entry, 1)) {
				return 0;
			}
		}

		if (XrLikely(entry->HostPointer != 0)) {
			// We can do the access inline directly through the pointer.

			if (sc) {
				uint32_t phyaddr = ((entry->MatchingDtbe >> 5) << 12) | (address & 0xFFF);

				if (!XrStoreIfClaimed(proc, phyaddr, entry->HostPointer + (address & 0xFFF), srcvalue)) {
					return 2;
				} else {
					return 1;
				}
			} else {
				CopyWithLength(entry->HostPointer + (address & 0xFFF), &srcvalue, length);

				return 1;
			}
		}

		address = ((entry->MatchingDtbe >> 5) << 12) | (address & 0xFFF);
	}

	int status = XrDirectEBusWrite(proc, address, srcvalue, length);

	if (!status) {
		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_BUS, proc->Pc);
	}

	return status;
}

static XR_ALWAYS_INLINE int XrAccessRead(XrProcessor *proc, XrIblock *iblock, uint32_t address, uint32_t *dest, uint32_t length, int ll) {
	if (XrUnlikely((address & (length - 1)) != 0)) {
		// Unaligned access.

		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_UNA, proc->Pc);

		return 0;
	}

	if (XrLikely((proc->Cr[RS] & RS_MMU) != 0)) {
		// Look up the DTB lookup cache in the Iblock.

		uint32_t matching = (proc->Cr[DTBTAG] & 0xFFF00000) | (address >> 12);

		int index = XR_IBLOCK_DTB_CACHE_INDEX(address);
		XrIblockDtbEntry *entry = &iblock->DtbLoadCache[index];

		if (XrLikely(((entry->MatchingDtbe >> 32) != matching) ||
			(proc->Dtb[entry->Index] != entry->MatchingDtbe))) {

			if (!XrTranslateDstream(proc, address, entry, 0)) {
				return 0;
			}
		}

		if (XrLikely(entry->HostPointer != 0)) {
			// We can do the access inline directly through the pointer.

			if (ll) {
				uint32_t phyaddr = ((entry->MatchingDtbe >> 5) << 12) | (address & 0xFFF);

				*dest = XrClaimAddress(proc, phyaddr, entry->HostPointer + (address & 0xFFF));
			} else {
				CopyWithLengthZext(dest, entry->HostPointer + (address & 0xFFF), length);
			}

			return 1;
		}

		address = ((entry->MatchingDtbe >> 5) << 12) | (address & 0xFFF);
	}

	int status = XrDirectEBusRead(proc, address, dest, length);

	if (!status) {
		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_BUS, proc->Pc);
	}

	return status;
}

#define XrReadByte(_proc, _address, _value) XrAccessRead(_proc, block, _address, _value, 1, 0)
#define XrReadInt(_proc, _address, _value) XrAccessRead(_proc, block, _address, _value, 2, 0)
#define XrReadLong(_proc, _address, _value) XrAccessRead(_proc, block, _address, _value, 4, 0)
#define XrReadLongLl(_proc, _address, _value) XrAccessRead(_proc, block, _address, _value, 4, 1)

#define XrWriteByte(_proc, _address, _value) XrAccessWrite(_proc, block, _address, _value, 1, 0)
#define XrWriteInt(_proc, _address, _value) XrAccessWrite(_proc, block, _address, _value, 2, 0)
#define XrWriteLong(_proc, _address, _value) XrAccessWrite(_proc, block, _address, _value, 4, 0)
#define XrWriteLongSc(_proc, _address, _value) XrAccessWrite(_proc, block, _address,  _value, 4, 1)
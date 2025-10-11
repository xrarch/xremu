XrClaimTableEntry XrL2ClaimTable[XR_L2_CLAIM_TABLE_SIZE];

#define XR_CLAIM_HASH(phyaddr) ((phyaddr >> 2) ^ (phyaddr >> 12))
#define XR_L2_CLAIM_INDEX(phyaddr) (XR_CLAIM_HASH(phyaddr) % XR_L2_CLAIM_TABLE_SIZE)
#define XR_L1_CLAIM_INDEX(phyaddr) (XR_CLAIM_HASH(phyaddr) % XR_L1_CLAIM_TABLE_SIZE)

#ifndef SINGLE_THREAD_MP

static XR_ALWAYS_INLINE uint32_t XrClaimAddress(XrProcessor *proc, uint32_t phyaddr, uint32_t *hostaddr) {
	XrClaimTableEntry *l1entry = &proc->L1ClaimTable[XR_L1_CLAIM_INDEX(phyaddr)];
	XrClaimTableEntry *l2entry = &XrL2ClaimTable[XR_L2_CLAIM_INDEX(phyaddr)];

	uint32_t val;

	XrLockMutex(&l1entry->Lock);

	if (l1entry->OtherEntry == l2entry) {
		val = *hostaddr;

		XrUnlockMutex(&l1entry->Lock);
	} else {
		// The entry was not already claimed in our L1 claim table so we need to
		// allocate an entry in the L2 claim table.

		if (l1entry->OtherEntry) {
			// Clear the L2 entry that previously pointed to this L1 entry.
			// This is safe to do under only the L1 entry lock because both
			// locks are held before a remote invalidation is performed.

			l1entry->OtherEntry->OtherEntry = 0;
			l1entry->OtherEntry = 0;
		}

		XrUnlockMutex(&l1entry->Lock);

		// The L2 entry was not already claimed by the L1.

		XrLockMutex(&l2entry->Lock);

		XrClaimTableEntry *remotel1entry = l2entry->OtherEntry;

		// Invalidate the remote L1 entry that it referred to.

		if (remotel1entry) {
			XrLockMutex(&remotel1entry->Lock);

			if (l2entry->OtherEntry) {
				// If the L2 entry was not invalidated in the meanwhile, then
				// we still need to invalidate the L1 entry.

				remotel1entry->OtherEntry = 0;
			}

			XrUnlockMutex(&remotel1entry->Lock);
		}

		l2entry->OtherEntry = l1entry;
		l1entry->OtherEntry = l2entry;

		val = *hostaddr;

		XrUnlockMutex(&l2entry->Lock);
	}

	return val;
}

static XR_ALWAYS_INLINE int XrStoreIfClaimed(XrProcessor *proc, uint32_t phyaddr, uint32_t* hostaddr, uint32_t val) {
	XrClaimTableEntry *l1entry = &proc->L1ClaimTable[XR_L1_CLAIM_INDEX(phyaddr)];
	XrClaimTableEntry *l2entry = &XrL2ClaimTable[XR_L2_CLAIM_INDEX(phyaddr)];

	XrLockMutex(&l1entry->Lock);

	if (l1entry->OtherEntry != l2entry) {
		XrUnlockMutex(&l1entry->Lock);

		return 0;
	}

	*hostaddr = val;

	XrUnlockMutex(&l1entry->Lock);

	return 1;
}

#else

static XR_ALWAYS_INLINE uint32_t XrClaimAddress(XrProcessor *proc, uint32_t phyaddr, uint32_t *hostaddr) {
	XrClaimTableEntry *l1entry = &proc->L1ClaimTable[XR_L1_CLAIM_INDEX(phyaddr)];
	XrClaimTableEntry *l2entry = &XrL2ClaimTable[XR_L2_CLAIM_INDEX(phyaddr)];

	uint32_t val;

	if (l1entry->OtherEntry == l2entry) {
		val = *hostaddr;
	} else {
		// The entry was not already claimed in our L1 claim table so we need to
		// allocate an entry in the L2 claim table.

		if (l1entry->OtherEntry) {
			// Clear the L2 entry that previously pointed to this L1 entry.
			// This is safe to do under only the L1 entry lock because both
			// locks are held before a remote invalidation is performed.

			l1entry->OtherEntry->OtherEntry = 0;
			l1entry->OtherEntry = 0;
		}

		// The L2 entry was not already claimed by the L1.

		XrClaimTableEntry *remotel1entry = l2entry->OtherEntry;

		// Invalidate the remote L1 entry that it referred to.

		if (remotel1entry) {
			remotel1entry->OtherEntry = 0;
		}

		l2entry->OtherEntry = l1entry;
		l1entry->OtherEntry = l2entry;

		val = *hostaddr;
	}

	return val;
}

static XR_ALWAYS_INLINE int XrStoreIfClaimed(XrProcessor *proc, uint32_t phyaddr, uint32_t* hostaddr, uint32_t val) {
	XrClaimTableEntry *l1entry = &proc->L1ClaimTable[XR_L1_CLAIM_INDEX(phyaddr)];
	XrClaimTableEntry *l2entry = &XrL2ClaimTable[XR_L2_CLAIM_INDEX(phyaddr)];

	if (l1entry->OtherEntry != l2entry) {
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

static XR_ALWAYS_INLINE uint64_t *XrLookupDtb(XrProcessor *proc, uint32_t virtual, uint64_t *tbe, int writing) {
	uint32_t vpn = virtual >> 12;
	uint32_t matching = (proc->Cr[DTBTAG] & 0xFFF00000) | vpn;

	for (int i = 0; i < XR_DTB_SIZE; i++) {
		uint64_t tmp = proc->Dtb[i];

		uint32_t mask = (tmp & PTE_GLOBAL) ? 0xFFFFF : 0xFFFFFFFF;

		if (((tmp >> 32) & mask) == (matching & mask)) {
			*tbe = tmp;

			return &proc->Dtb[i];
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

	return 0;
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
	uint64_t *tbeptr;

	if (vpn == proc->DtbLastVpn) {
		tbe = proc->DtbLastEntry.MatchingDtbe;
	} else {
		tbeptr = XrLookupDtb(proc, virtual, &tbe, writing);

		if (!tbeptr) {
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
		proc->DtbLastEntry.DtbePointer = tbeptr;
		proc->DtbLastEntry.HostPointer = EBusTranslate((tbe >> 5) << 12);
		proc->DtbLastVpn = vpn;

		entry->MatchingDtbe = tbe;
		entry->DtbePointer = tbeptr;
		entry->HostPointer = proc->DtbLastEntry.HostPointer;
	} else {
		entry->MatchingDtbe = tbe;
		entry->DtbePointer = proc->DtbLastEntry.DtbePointer;
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

		XrIblockDtbEntry *entry = &iblock->DtbStoreCache[XR_IBLOCK_DTB_CACHE_INDEX(address)];

		if (XrUnlikely(((entry->MatchingDtbe >> 32) != matching) ||
			(entry->DtbePointer[0] != entry->MatchingDtbe) ||
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

		if (XrUnlikely(((entry->MatchingDtbe >> 32) != matching) ||
			(entry->DtbePointer[0] != entry->MatchingDtbe))) {

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
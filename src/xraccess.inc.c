uint8_t XrPrintCache = 0;

uint32_t XrScacheTags[XR_SC_LINE_COUNT];
uint32_t XrScacheReplacementIndex;
uint8_t XrScacheFlags[XR_SC_LINE_COUNT];
uint8_t XrScacheExclusiveIds[XR_SC_LINE_COUNT];

#define XR_LINE_INVALID 0
#define XR_LINE_SHARED 1
#define XR_LINE_EXCLUSIVE 2

#define XrReadByte(_proc, _address, _value) XrAccess(_proc, _address, _value, 0, 1, 0)
#define XrReadInt(_proc, _address, _value) XrAccess(_proc, _address, _value, 0, 2, 0)
#define XrReadLong(_proc, _address, _value) XrAccess(_proc, _address, _value, 0, 4, 0)

#define XrWriteByte(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 1, 0)
#define XrWriteInt(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 2, 0)
#define XrWriteLong(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 4, 0)
#define XrWriteLongSc(_proc, _address, _value) XrAccess(_proc, _address, 0, _value, 4, 1)

#define XrTranslateIstream(_proc, _virtual, _phys, _flags) XrTranslate(_proc, _virtual, _phys, _flags, 0, 1)

static inline uint8_t XrLookupItb(XrProcessor *proc, uint32_t virtual, uint64_t *tbe) {
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

static inline uint8_t XrLookupDtb(XrProcessor *proc, uint32_t virtual, uint64_t *tbe, uint8_t writing) {
	uint32_t vpn = virtual >> 12;
	uint32_t matching = (proc->Cr[DTBTAG] & 0xFFF00000) | vpn;

	for (int i = 0; i < XR_DTB_SIZE; i++) {
		uint64_t tmp = proc->Dtb[i];

		uint32_t mask = (tmp & PTE_GLOBAL) ? 0xFFFFF : 0xFFFFFFFF;

		if (((tmp >> 32) & mask) == (matching & mask)) {
			*tbe = tmp;

			return 1;
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

static inline int XrTranslate(XrProcessor *proc, uint32_t virtual, uint32_t *phys, int *flags, bool writing, bool ifetch) {
	uint64_t tbe;
	uint32_t vpn = virtual >> 12;

	if (ifetch) {
		if (XrLikely(proc->ItbLastVpn == vpn)) {
			// This matches the last lookup, avoid searching the whole ITB.

			tbe = proc->ItbLastResult;
		} else if (XrUnlikely(!XrLookupItb(proc, virtual, &tbe))) {
			return 0;
		}
	} else {
		if (XrLikely(proc->DtbLastVpn == vpn)) {
			// This matches the last lookup, avoid searching the whole DTB.

			tbe = proc->DtbLastResult;
		} else if (XrUnlikely(!XrLookupDtb(proc, virtual, &tbe, writing))) {
			return 0;
		}
	}

	if (XrUnlikely((tbe & PTE_VALID) == 0)) {
		// Not valid! Page fault time.

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

		return 0;
	}

	if (XrUnlikely((tbe & PTE_KERNEL) && (proc->Cr[RS] & RS_USER))) {
		// Kernel mode page and we're in usermode! 

		proc->Cr[EBADADDR] = virtual;
		XrBasicException(proc, writing ? XR_EXC_PGW : XR_EXC_PGF, proc->Pc);

		return 0;
	}

	if (XrUnlikely(writing && !(tbe & PTE_WRITABLE))) {
		proc->Cr[EBADADDR] = virtual;
		XrBasicException(proc, writing ? XR_EXC_PGW : XR_EXC_PGF, proc->Pc);

		return 0;
	}

	if (ifetch) {
		proc->ItbLastVpn = vpn;
		proc->ItbLastResult = tbe;
	} else {
		proc->DtbLastVpn = vpn;
		proc->DtbLastResult = tbe;
	}

	*flags = tbe & 31;
	*phys = ((tbe & 0x1FFFFE0) << 7) + (virtual & 0xFFF);

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

static inline int XrNoncachedAccess(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t srcvalue, uint32_t length) {
#if XR_SIMULATE_CACHE_STALLS
	proc->StallCycles += XR_UNCACHED_STALL;
#endif

	int result;

	if (dest) {
		result = EBusRead(address, dest, length, proc);
	} else {
		result = EBusWrite(address, &srcvalue, length, proc);
	}

	if (XrUnlikely(result == EBUSERROR)) {
		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_BUS, proc->Pc);

		return 0;
	}

	return 1;
}

static inline uint32_t *XrIcacheAccess(XrProcessor *proc, uint32_t address) {
	// Access Icache. Quite fast, don't need to take any locks or anything
	// as there is no coherence. Returns a pointer to the data within the
	// Icache for direct access, or NULLPTR if a bus error occurred.

	uint32_t tag = address & ~(XR_IC_LINE_SIZE - 1);

	uint32_t setnumber = XR_IC_SET_NUMBER(address);
	uint32_t cacheindex = setnumber << XR_IC_WAY_LOG;

	for (int i = 0; i < XR_IC_WAYS; i++) {
		if (proc->IcFlags[cacheindex + i] && proc->IcTags[cacheindex + i] == tag) {
			// Found it!

#ifdef PROFCPU
			proc->IcHitCount++;
#endif

			uint32_t cacheoff = (cacheindex + i) << XR_IC_LINE_SIZE_LOG;

			return (uint32_t*)(&proc->Ic[cacheoff]);
		}
	}

#ifdef PROFCPU
	proc->IcMissCount++;
#endif

#if XR_SIMULATE_CACHE_STALLS
	// Unfortunately there was a miss. Incur a penalty.

	proc->StallCycles += XR_MISS_STALL;
#endif

	// Replace a random-ish line within the set.

	uint32_t newindex = cacheindex + (proc->IcReplacementIndex & (XR_IC_WAYS - 1));
	proc->IcReplacementIndex += 1;

	uint32_t cacheoff = newindex << XR_IC_LINE_SIZE_LOG;

	int result = EBusRead(tag, &proc->Ic[cacheoff], XR_IC_LINE_SIZE, proc);

	if (XrUnlikely(result == EBUSERROR)) {
		proc->IcFlags[newindex] = XR_LINE_INVALID;

		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_BUS, proc->Pc);

		return 0;
	}

	proc->IcFlags[newindex] = XR_LINE_SHARED;
	proc->IcTags[newindex] = tag;

	return (uint32_t*)(&proc->Ic[cacheoff]);
}

static inline void XrFlushWriteBuffer(XrProcessor *proc) {
	// Flush the write buffer of the given processor.

	for (int i = 0; i < XR_WB_DEPTH; i++) {
		uint32_t index = proc->WbIndices[i];

		if (index != XR_CACHE_INDEX_INVALID) {
			// Found an entry. Try locking the corresponding tag.

			uint32_t tag = proc->DcTags[index];

			XrLockCache(proc, tag);

			// Check if the buffer entry is still valid. It may have been
			// flushed out due to cache invalidation.

			if (proc->WbIndices[i] == XR_CACHE_INDEX_INVALID) {
				// It was flushed.

				XrUnlockCache(proc, tag);

				continue;
			}

			// Write out the entry.

			DBGPRINT("flush wb write %x from cacheindex %d\n", tag, index);

			EBusWrite(tag, &proc->Dc[index << XR_DC_LINE_SIZE_LOG], XR_DC_LINE_SIZE, proc);

			// Invalidate it.

			proc->WbIndices[i] = XR_CACHE_INDEX_INVALID;
			proc->DcIndexToWbIndex[index] = XR_WB_INDEX_INVALID;

			// Unlock the tag.

			XrUnlockCache(proc, tag);
		}
	}
}

static inline uint32_t XrFindInScache(uint32_t tag) {
	// Find a tag in the Scache.

	uint32_t setnumber = XR_SC_SET_NUMBER(tag);
	uint32_t cacheindex = setnumber << XR_SC_WAY_LOG;

	for (int i = 0; i < XR_SC_WAYS; i++) {
		DBGPRINT("scache search %d: flags=%d tag=%x\n", cacheindex+i, XrScacheFlags[cacheindex+i], XrScacheTags[cacheindex+i]);

		if (XrScacheFlags[cacheindex + i] && XrScacheTags[cacheindex + i] == tag) {
			// Found it!

			DBGPRINT("found %x at scache %d\n", tag, cacheindex + i);

			return cacheindex + i;
		}
	}

	return -1;
}

static inline void XrDowngradeLine(XrProcessor *proc, uint32_t tag, uint32_t newstate) {
	// Invalidate a line if found in the given processor's Dcache.

	uint32_t setnumber = XR_DC_SET_NUMBER(tag);
	uint32_t cacheindex = setnumber << XR_DC_WAY_LOG;

	XrLockCache(proc, tag);

	// Find it in the Dcache.

	for (int i = 0; i < XR_DC_WAYS; i++) {
		DBGPRINT("search %d %x %d\n", cacheindex + i, proc->DcTags[cacheindex + i], proc->DcFlags[cacheindex + i]);

		uint32_t index = cacheindex + i;

		if (proc->DcFlags[index] > newstate && proc->DcTags[index] == tag) {
			// Found it! Kill it.

			DBGPRINT("found to inval %x %d %d\n", tag, proc->DcFlags[cacheindex + i], cacheindex + i);

			// Find it in the writebuffer.

			uint32_t wbindex = proc->DcIndexToWbIndex[index];

			if (wbindex != XR_WB_INDEX_INVALID) {
				// Force a write out.

				DBGPRINT("forced wb write %x from cacheindex %d\n", tag, index);

				EBusWrite(tag, &proc->Dc[index << XR_DC_LINE_SIZE_LOG], XR_DC_LINE_SIZE, proc);

				// Invalidate.

				proc->WbIndices[wbindex] = XR_CACHE_INDEX_INVALID;
				proc->DcIndexToWbIndex[index] = XR_WB_INDEX_INVALID;
			}

			proc->DcFlags[index] = newstate;

			break;
		}
	}

	XrUnlockCache(proc, tag);
}

static inline void XrDowngradeAll(XrProcessor *thisproc, uint32_t tag, uint32_t newstate) {
	// Invalidate a Dcache line in all processors except the one provided.

	for (int i = 0; i < XrProcessorCount; i++) {
		XrProcessor *proc = CpuTable[i];

		if (proc == thisproc) {
			continue;
		}

		XrDowngradeLine(proc, tag, newstate);
	}
}

static inline uint32_t XrFindOrReplaceInScache(XrProcessor *thisproc, uint32_t tag, uint32_t newstate, uint32_t *created) {
	// Find a tag in the Scache, or replace one if not found.

	*created = 0;

	uint32_t setnumber = XR_SC_SET_NUMBER(tag);
	uint32_t cacheindex = setnumber << XR_SC_WAY_LOG;

	for (int i = 0; i < XR_SC_WAYS; i++) {
		uint32_t index = cacheindex + i;

		if (XrScacheFlags[index] && XrScacheTags[index] == tag) {
			// Found it!

			DBGPRINT("scache hit %x\n", tag);

			return index;
		}
	}

	// Didn't find it so select one to replace.

	cacheindex += XrScacheReplacementIndex & (XR_SC_WAYS - 1);
	XrScacheReplacementIndex += 1;

	if (XrScacheFlags[cacheindex] == XR_LINE_INVALID) {
		// It's invalid, we can just take it.

	} else {
		// Lock the tag in this selected entry.

		uint32_t oldtag = XrScacheTags[cacheindex];

		DBGPRINT("scache take %x\n", oldtag);

		if (XrScacheFlags[cacheindex] == XR_LINE_SHARED) {
			// We have to remove it from everyone's Dcache, including mine.

			DBGPRINT("scache steal %x\n", oldtag);

			XrDowngradeAll(0, oldtag, XR_LINE_INVALID);

		} else if (XrScacheFlags[cacheindex] == XR_LINE_EXCLUSIVE) {
			// Remove it from the owner's Dcache.

			DBGPRINT("scache steal from exclusive %x\n", oldtag);

			XrDowngradeLine(CpuTable[XrScacheExclusiveIds[cacheindex]], oldtag, XR_LINE_INVALID);
		}

		// Mark invalid for the time where we have no tags locked.

		XrScacheFlags[cacheindex] = XR_LINE_INVALID;
	}

	XrScacheFlags[cacheindex] = newstate;
	XrScacheTags[cacheindex] = tag;
	XrScacheExclusiveIds[cacheindex] = thisproc->Id;

	DBGPRINT("scache fill %x\n", tag);

	*created = 1;

	return cacheindex;
}

static int XrDcacheAccess(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t srcvalue, uint32_t length, int sc) {
	// Access Dcache. Scary! We have to worry about coherency with the other
	// Dcaches in the system, and this can be a 1, 2, or 4 byte access.
	// If dest == 0, this is a write. Otherwise it's a read.

	uint32_t tag = address & ~(XR_DC_LINE_SIZE - 1);
	uint32_t lineoffset = address & (XR_DC_LINE_SIZE - 1);
	uint32_t setnumber = XR_DC_SET_NUMBER(address);
	uint32_t cacheindex = setnumber << XR_DC_WAY_LOG;
	uint32_t freewbindex = -1;

restart:

	if (dest == 0) {
		// This is a write; find a write buffer entry.

		XrLockCache(proc, tag);

		for (int i = 0; i < XR_WB_DEPTH; i++) {
			uint32_t index = proc->WbIndices[i];

			// DBGPRINT("%x\n", index);

			if (index == XR_CACHE_INDEX_INVALID) {
				freewbindex = i;

			} else if (proc->DcTags[index] == tag) {
				// Found it. We can quickly merge the write into the cache line
				// and continue.

				// Note that this means we have the cache line exclusive as the
				// writebuffer entry should be purged with the tag locked
				// whenever the line is invalidated or downgraded.

				uint32_t cacheoff = index << XR_DC_LINE_SIZE_LOG;

				CopyWithLength(&proc->Dc[cacheoff + lineoffset], &srcvalue, length);

				XrUnlockCache(proc, tag);

				// DBGPRINT("write hit %x merge in writebuffer\n", tag);

				return 1;
			}
		}

		// Failed to find it in the write buffer.

		if (freewbindex == -1) {
			// Write buffer is full. Flush and retry.

			XrUnlockCache(proc, tag);

#if XR_SIMULATE_CACHE_STALLS
			proc->StallCycles += XR_UNCACHED_STALL * XR_WB_DEPTH;
#endif

			XrFlushWriteBuffer(proc);

			DBGPRINT("flush writebuffer full %x\n", tag);

			goto restart;
		}

		// We're going to insert an entry into the writebuffer, so start the
		// writebuffer write countdown.

		if (proc->WbCycles == 0) {
			proc->WbCycles = XR_UNCACHED_STALL;
		}
	}

	// Look up the cache line.

	for (int i = 0; i < XR_DC_WAYS; i++) {
		uint32_t index = cacheindex + i;

		if (proc->DcFlags[index] && proc->DcTags[index] == tag) {
			// Cache hit.

			uint32_t cacheoff = index << XR_DC_LINE_SIZE_LOG;

#ifdef PROFCPU
			proc->DcHitCount += 1;
#endif

			if (dest != 0) {
				// We're reading. Copy out the data.

				CopyWithLengthZext(dest, &proc->Dc[cacheoff + lineoffset], length);

				// DBGPRINT("read hit %x\n", tag);

				return 1;
			}

			if (proc->DcFlags[index] == XR_LINE_EXCLUSIVE) {
				// We're writing. We got a write buffer index earlier so set
				// it as representing this cache line.

				proc->WbIndices[freewbindex] = index;
				proc->DcIndexToWbIndex[index] = freewbindex;

				// Now merge in the data.

				CopyWithLength(&proc->Dc[cacheoff + lineoffset], &srcvalue, length);

				XrUnlockCache(proc, tag);

				// DBGPRINT("write hit %x already exclusive\n", tag);

				return 1;
			}

			// This is a write and the cache line is shared. We need to remove
			// it from everybody else's cache, and set it to exclusive in ours.

			// Since we're doing a global operation we need to drop our lock and
			// acquire the Scache lock.

			DBGPRINT("write hit %x to exclusive from shared\n", tag);

			XrUnlockCache(proc, tag);

			// If the cache line is shared that actually doesn't violate the SC
			// condition, since that means nobody else wrote to the cache line
			// in the interim since LL.

#if 0
			if (sc && dest == 0) {
				// We failed the SC condition, since it was only shared, not
				// exclusive.

				return 2;
			}
#endif

			XrLockScache(tag);

			// While locks were dropped the state of our line might have
			// changed (namely, it may have been invalidated).

			if (proc->DcFlags[index] == XR_LINE_INVALID) {
				// It was invalidated. Unlock and start all over.

				DBGPRINT("invalid retry %x\n", tag);

				XrUnlockScache(tag);

				goto restart;
			}

			// Look up the Scache line.

			uint32_t scacheindex = XrFindInScache(tag);

			// If it's valid in our cache, it must be valid in the Scache,
			// since our cache is maintained as a subset of the Scache.

			if (scacheindex == -1) {
				fprintf(stderr, "Not found in scache %08x %d %d\n", address, index, proc->DcFlags[index]);
				abort();
			}

			// Downgrade all matching lines to INVALID.

			if (XrScacheFlags[scacheindex] == XR_LINE_SHARED) {
				// Really have to search everyone.

				DBGPRINT("steal on write upgrade %x\n", tag);

				XrDowngradeAll(proc, tag, XR_LINE_INVALID);

			} else if (XrScacheFlags[scacheindex] == XR_LINE_EXCLUSIVE) {
				// Since it's exclusive it can only be in one Dcache.

				DBGPRINT("steal exclusive on write upgrade %x\n", tag);

				XrDowngradeLine(CpuTable[XrScacheExclusiveIds[scacheindex]], tag, XR_LINE_INVALID);
			}

			// Set the new cache states.

			XrScacheFlags[scacheindex] = XR_LINE_EXCLUSIVE;
			XrScacheExclusiveIds[scacheindex] = proc->Id;
			proc->DcFlags[index] = XR_LINE_EXCLUSIVE;

			// We got a write buffer index earlier so set
			// it as representing this cache line.

			proc->WbIndices[freewbindex] = index;
			proc->DcIndexToWbIndex[index] = freewbindex;

			// Now merge in the data.

			CopyWithLength(&proc->Dc[cacheoff + lineoffset], &srcvalue, length);

			XrUnlockScache(tag);

			return 1;
		}
	}

	DBGPRINT("miss on %x\n", tag);

	if (!dest) {
		// Cache miss. Unlock our cache.
		// Only need to do this if we're writing, since on a read, we didn't
		// lock it to begin with.

		XrUnlockCache(proc, tag);
	}

	if (sc) {
		// We failed the SC condition since it was invalid.

		return 2;
	}

#ifdef PROFCPU
	proc->DcMissCount += 1;
#endif

	// If this is a write, we want the line exclusive; otherwise shared.

	uint32_t newstate = (dest == 0) ? XR_LINE_EXCLUSIVE : XR_LINE_SHARED;

	// Lock the Scache.

	XrLockScache(tag);

	// Look up the tag.

	uint32_t created;

	uint32_t scacheindex = XrFindOrReplaceInScache(proc, tag, newstate, &created);

	if (created == 0) {
		// The line wasn't created for us. It already existed in the Scache and
		// we might need to do something about it.

		if (XrScacheFlags[scacheindex] == XR_LINE_EXCLUSIVE) {
			// We have to remove it from this cache who has it exclusive.
			// If we're writing, we want to downgrade it to invalid, because
			// we're taking it exclusive. Otherwise downgrade it to shared.

			DBGPRINT("remove exclusive %x after miss\n", tag);

			XrDowngradeLine(CpuTable[XrScacheExclusiveIds[scacheindex]], tag, (dest == 0) ? XR_LINE_INVALID : XR_LINE_SHARED);

		} else if (dest == 0 && XrScacheFlags[scacheindex] == XR_LINE_SHARED) {
			// We're writing and this line was shared. We have to invalidate it
			// in everybody.

			DBGPRINT("remove shared %x after miss\n", tag);

			XrDowngradeAll(0, tag, XR_LINE_INVALID);
		}

		XrScacheFlags[scacheindex] = newstate;
		XrScacheExclusiveIds[scacheindex] = proc->Id;
	}

	// The line is now in the desired state in the Scache (and in everyone
	// else's Dcache).

	XrLockCache(proc, tag);

	// We have to select a Dcache line to replace now.

	uint32_t index = cacheindex + (proc->DcReplacementIndex++ & (XR_DC_WAYS - 1));

	if (proc->DcFlags[index] != XR_LINE_INVALID) {
		// Lock the tag in this selected entry.

		uint32_t oldtag = proc->DcTags[index];

		DBGPRINT("reclaim %x after miss\n", oldtag);

		// See if it's in the writebuffer. If so we'll have to force a
		// write.

		uint32_t wbindex = proc->DcIndexToWbIndex[index];

		if (wbindex != XR_WB_INDEX_INVALID) {
			// Force a write out.

			DBGPRINT("forced self wb write %x from cacheindex %d\n", tag, index);

			EBusWrite(oldtag, &proc->Dc[index << XR_DC_LINE_SIZE_LOG], XR_DC_LINE_SIZE, proc);

			// Invalidate.

			proc->WbIndices[wbindex] = XR_CACHE_INDEX_INVALID;
			proc->DcIndexToWbIndex[index] = XR_WB_INDEX_INVALID;
		}

		// Mark invalid for the time where we have no tags locked.

		proc->DcFlags[index] = XR_LINE_INVALID;
	}

	// Initialize the cache line.

	DBGPRINT("new cacheline setnumber=%x cacheindex=%x index=%x out of %x\n", setnumber, cacheindex, index, XR_DC_LINE_COUNT);

	proc->DcFlags[index] = newstate;
	proc->DcTags[index] = tag;

#if XR_SIMULATE_CACHE_STALLS
	// Incur a stall.

	proc->StallCycles += XR_MISS_STALL;
#endif

	// Read in the cache line contents.

	uint32_t cacheoff = index << XR_DC_LINE_SIZE_LOG;

	int result = EBusRead(tag, &proc->Dc[cacheoff], XR_DC_LINE_SIZE, proc);

	if (XrUnlikely(result == EBUSERROR)) {
		// We failed. Back out.

		XrScacheFlags[scacheindex] = XR_LINE_INVALID;
		proc->DcFlags[index] = XR_LINE_INVALID;

		XrUnlockCache(proc, tag);
		XrUnlockScache(tag);

		DBGPRINT("bus error on %x\n", tag);

		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_BUS, proc->Pc);

		return 0;
	}

	XrUnlockScache(tag);

	if (dest == 0) {
		// We're writing. We got a write buffer index earlier so set
		// it as representing this cache line.

		proc->WbIndices[freewbindex] = index;
		proc->DcIndexToWbIndex[index] = freewbindex;

		// Now merge in the data.

		CopyWithLength(&proc->Dc[cacheoff + lineoffset], &srcvalue, length);

		// DBGPRINT("write %x after miss\n", tag);

		XrUnlockCache(proc, tag);

		return 1;

	}

	// We're reading.

	// DBGPRINT("read %x after miss\n", tag);

	CopyWithLengthZext(dest, &proc->Dc[cacheoff + lineoffset], length);

	XrUnlockCache(proc, tag);

	return 1;
}

static inline int XrAccess(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t srcvalue, uint32_t length, int sc) {
	if (XrUnlikely((address & (length - 1)) != 0)) {
		// Unaligned access.

		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_UNA, proc->Pc);

		return 0;
	}

	int flags = 0;

	if (XrLikely((proc->Cr[RS] & RS_MMU) != 0)) {
		if (!XrTranslate(proc, address, &address, &flags, dest ? 0 : 1, 0)) {
			return 0;
		}
	} else if (address >= XR_NONCACHED_PHYS_BASE) {
		flags |= PTE_NONCACHED;
	}

	if (!XR_SIMULATE_CACHES) {
		flags |= PTE_NONCACHED;
	}

	if (flags & PTE_NONCACHED) {
		if (XrUnlikely(sc && XR_SIMULATE_CACHES)) {
			// Attempt to use SC on noncached memory. Nonsense! Just kill the
			// evildoer with a bus error.

			proc->Cr[EBADADDR] = address;
			XrBasicException(proc, XR_EXC_BUS, proc->Pc);

			return 0;
		}

		int status = XrNoncachedAccess(proc, address, dest, srcvalue, length);

		if (XrUnlikely(!status)) {
			return 0;
		}

		if (dest) {
			*dest &= XrAccessMasks[length];
		}

		return 1;
	}

	return XrDcacheAccess(proc, address, dest, srcvalue, length, sc);
}

static inline void XrWriteWbEntry(XrProcessor *proc) {
	// Write out a write buffer entry starting at the WriteIndex.

	for (int i = 0; i < XR_WB_DEPTH; i++) {
		int wbindex = (proc->WbWriteIndex + i) % XR_WB_DEPTH;

		uint32_t index = proc->WbIndices[wbindex];

		if (index != XR_CACHE_INDEX_INVALID) {
			// Found an entry. Try locking the corresponding tag.

			uint32_t tag = proc->DcTags[index];

			XrLockCache(proc, tag);

			// Check if the buffer entry is still valid. It may have been
			// flushed out due to cache invalidation.

			if (proc->WbIndices[wbindex] == XR_CACHE_INDEX_INVALID) {
				// It was flushed.

				XrUnlockCache(proc, tag);

				continue;
			}

			// Write out the entry.

			DBGPRINT("timed wb write %x from cacheindex %d\n", tag, index);

			EBusWrite(tag, &proc->Dc[index << XR_DC_LINE_SIZE_LOG], XR_DC_LINE_SIZE, proc);

			// Invalidate it.

			proc->WbIndices[wbindex] = XR_CACHE_INDEX_INVALID;
			proc->DcIndexToWbIndex[index] = XR_WB_INDEX_INVALID;

			// Unlock the tag.

			XrUnlockCache(proc, tag);

			// Set the write index.

			proc->WbWriteIndex = i + 1;

			// Reset the write cycle counter.

			proc->WbCycles = XR_UNCACHED_STALL;

			// Break out.

			break;
		}
	}
}
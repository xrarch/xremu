static uint32_t XrAccessMasks[5] = {
	0x00000000,
	0x000000FF,
	0x0000FFFF,
	0x00FFFFFF,
	0xFFFFFFFF
};

static inline int XrDirectEBusWrite(XrProcessor *proc, uint32_t address, uint32_t srcvalue, uint32_t length) {
	return EBusWrite(address, &srcvalue, length, proc) == EBUSSUCCESS;
}

static inline int XrDirectEBusRead(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t length) {
	int status = EBusRead(address, dest, length, proc) == EBUSSUCCESS;

	if (status) {
		*dest &= XrAccessMasks[length];
	}

	return status;
}

static inline int XrAccessWrite(XrProcessor *proc, uint32_t address, uint32_t srcvalue, uint32_t length, int sc) {
	if (XrUnlikely((address & (length - 1)) != 0)) {
		// Unaligned access.

		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_UNA, proc->Pc);

		return 0;
	}

	int flags = 0;

	if (XrLikely((proc->Cr[RS] & RS_MMU) != 0)) {
		if (!XrTranslate(proc, address, &address, &flags, 1, 0)) {
			return 0;
		}
	} else if (address >= XR_NONCACHED_PHYS_BASE) {
		flags |= PTE_NONCACHED;
	}

	int status;

	if (flags & PTE_NONCACHED) {
		if (XrUnlikely(sc)) {
			// Attempt to use SC on noncached memory. Nonsense! Just kill the
			// evildoer with a bus error.

			proc->Cr[EBADADDR] = address;
			XrBasicException(proc, XR_EXC_BUS, proc->Pc);

			return 0;
		}

		status = XrDirectEBusWrite(proc, address, srcvalue, length);
	} else {
		status = XrDirectEBusWrite(proc, address, srcvalue, length);
	}

	if (!status) {
		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_BUS, proc->Pc);
	}

	return status;
}

static inline int XrAccessRead(XrProcessor *proc, uint32_t address, uint32_t *dest, uint32_t length) {
	if (XrUnlikely((address & (length - 1)) != 0)) {
		// Unaligned access.

		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_UNA, proc->Pc);

		return 0;
	}

	int flags = 0;

	if (XrLikely((proc->Cr[RS] & RS_MMU) != 0)) {
		if (!XrTranslate(proc, address, &address, &flags, 0, 0)) {
			return 0;
		}
	} else if (address >= XR_NONCACHED_PHYS_BASE) {
		flags |= PTE_NONCACHED;
	}

	int status;

	if (flags & PTE_NONCACHED) {
		status = XrDirectEBusRead(proc, address, dest, length);
	} else {
		status = XrDirectEBusRead(proc, address, dest, length);
	}

	if (!status) {
		proc->Cr[EBADADDR] = address;
		XrBasicException(proc, XR_EXC_BUS, proc->Pc);
	}

	return status;
}

#define XrReadByte(_proc, _address, _value) XrAccessRead(_proc, _address, _value, 1);
#define XrReadInt(_proc, _address, _value) XrAccessRead(_proc, _address, _value, 2);
#define XrReadLong(_proc, _address, _value) XrAccessRead(_proc, _address, _value, 4);

#define XrWriteByte(_proc, _address, _value) XrAccessWrite(_proc, _address, _value, 1, 0);
#define XrWriteInt(_proc, _address, _value) XrAccessWrite(_proc, _address, _value, 2, 0);
#define XrWriteLong(_proc, _address, _value) XrAccessWrite(_proc, _address, _value, 4, 0);
#define XrWriteLongSc(_proc, _address, _value) XrAccessWrite(_proc, _address,  _value, 4, 1);
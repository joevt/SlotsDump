#include "SlotsCommon.h"


size_t OutAddr(void *base)
{
	return (size_t)base;
}


std::string ErrS(OSErr err)
{
	char* s;
	char buf[100];

	switch (err) {
		case    0: s = "noErr - No error"; break;
		case -300: s = "smEmptySlot - No card in slot"; break;
		case -305: s = "smDisabledSlot - This slot is disabled"; break;
		case -309: s = "smBLFieldBad - ByteLanes field was bad"; break;
		case -337: s = "smSlotOOBErr - Slot out of bounds"; break;
		case -346: s = "smBadsPtrErr - Bad pointer was passed to sCalcPointer"; break;
		case -344: s = "smNoMoresRsrcs - No more sResources"; break;
		default  : s = "unknown err";
	}
	snprintf(buf, sizeof(buf), "%" PRId16 " = %s", err, s);
	return buf;
}

void DoErr(const char * msg, OSErr err)
{
	if (err != noErr)
	{
		printf("Error occured.%s in %s\n", ErrS(err).c_str(), msg);
	}
}

Ptr CalcAddr(void* base, int32_t offset)
{
	SpBlock sp;
	OSErr   err;

	sp.spsPointer = (Ptr)base;
	sp.spOffsetData = offset;
	sp.spByteLanes = gByteLanes;
	err = SCalcSPointer(&sp);
	DoErr("CalcAddr", err);
	if (offset == 0)
		if (base != sp.spsPointer)
			printf("base:%08" PRIXPTR " offset:0 ptr:%08" PRIXPTR "\n", OutAddr(base), OutAddr(sp.spsPointer));
	return sp.spsPointer;
}


void DumpROM(uint8_t slot, Ptr start, int32_t len)
{
	FILE * dumpFile = NULL;
	Ptr buf = NULL;
	const int32_t bufSize = 16384;
	char fileName[100];
	Ptr source = start;

	if (!start || len <= 0)
		goto error;

	buf = (Ptr)malloc(bufSize);
	if (!buf)
		goto error;

	snprintf(fileName, sizeof(fileName), "Slot_%02" PRIX8 ".bin", slot);
	dumpFile = fopen(fileName, "wb");
	if (!dumpFile)
		goto error;

	printf("\nSaving ROM to file \"%s\"...\n", fileName);
	while (len > 0) {
		int32_t numBytes = len;
		if (numBytes > bufSize)
			numBytes = bufSize;
		GetBytes(source, buf, numBytes);
		fwrite(buf, 1, size_t(numBytes), dumpFile);
		source = CalcAddr(source, numBytes);
		len -= numBytes;
	}
	goto done;
error:
	printf("DumpROM error slot:%02" PRIX8 "\n", slot);
done:
	if (buf)
		free(buf);
	if (dumpFile)
		fclose(dumpFile);
}


void WriteSInfoRecordAndFHeaderRec(uint8_t slot)
{
	FHeaderRec fh;
	memset(&fh, 0, sizeof(fh));

	SpBlock sp;
	memset(&sp, 0, sizeof(sp));
	sp.spSlot = (int8_t)slot;
	sp.spResult = int32_t(&fh);

	OSErr headErr = SReadFHeader(&sp);
	if (headErr)
		return;

	OSErr infoErr = SFindSInfoRecPtr(&sp);
	if (infoErr)
		return;

	fprintf(gOutFile, "\n");
	fprintf(gOutFile, "-----------------------------------------\n");
	fprintf(gOutFile, "slot %02" PRIX8 ":\n", slot);

	SInfoRecPtr si = SInfoRecPtr(sp.spResult);
	gByteLanes = int8_t(si->siCPUByteLanes & 0x0F);
	gTopOfRom = CalcAddr(si->siROMAddr, 1);
	Ptr headerWhere = CalcAddr(gTopOfRom, -sizeof(fh));
	gStartOfRom = CalcAddr(gTopOfRom, -fh.fhLength);

	fprintf(gOutFile, "\n");
	fprintf(gOutFile, "FHeaderRec:\n");
	Ptr sResDirFromHead = NULL;
	WriteFHeaderRec(fh, sResDirFromHead, headerWhere);
	DumpROM(slot, gStartOfRom, fh.fhLength);
} /* WriteSInfoRecordAndFHeaderRec */


void SlotsDumpMain()
{
	uint8_t slot = 0;
	do {
		WriteSInfoRecordAndFHeaderRec(slot);
		slot++;
	} while (slot != 0);
} /* SlotsDumpMain */


int main(/*int argc, char **argv */)
{
	printf("Scanning slots...\n");

	gOutFile = stdout;

	SlotsDumpMain();

	fprintf(gOutFile, "\n");
	fprintf(gOutFile, "-----------------------------------------\n");
	printf("done\n");
} /* Main */

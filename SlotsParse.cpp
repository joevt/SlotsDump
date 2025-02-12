#include "SlotsCommon.h"


const int gByteLanesPerInt32[] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};


size_t OutAddr(void *base)
{
	int32_t bytes = int32_t(gTopOfRom - (Ptr)base);
	if (bytes == 0)
		return gAddrAfterRom;
	int quads = (bytes - 1) / gByteLanesPerInt32[gByteLanes];
	int lanes = bytes - quads * gByteLanesPerInt32[gByteLanes];
	int quadOffset = 4;
	for (int test = 8; lanes > 0; test >>= 1, quadOffset--)
		if (gByteLanes & test)
			lanes--;
	return gAddrAfterRom - (quads + 1) * 4 + quadOffset;
}

#if 0
                    gTopOfRom points to first byte after end of ROM
                    gAddrAfterRom is the simulated 32-bit address of the first quad after the end of ROM
                    |
                    v
    0   4   8   C   0 quads are 4 bytes
                0123  quadOffset is the two least significant bits of the simulated 32-bit address result.
                      It will be one of 0,1,2,3

        |---------------------------|
        |  bytes per quad (32 bits) |
        |---------------------------|
        |1      2       3       4   |
|-------|---------------------------| bytes = source bytes from gTopOfRom for adjusting byte lane address result
|       |quads  q       q       q   | quads = whole quads to adjust
|bytes  |  lanes  l       l       l | lanes = lanes remaining in last quad
|-------|---------------------------|
|1      |0 1    0 1     0 1     0 1 |
|2      |1 1    0 2     0 2     0 2 |
|3      |2 1    1 1     0 3     0 3 |
|4      |3 1    1 2     1 1     0 4 |
|5      |4 1    2 1     1 2     1 1 |
|6      |5 1    2 2     1 3     1 2 |
|7      |6 1    3 1     2 1     1 3 |
|8      |7 1    3 1     2 2     1 2 |
|-------|---------------------------|
#endif


Ptr CalcAddr(void* base, int32_t offset)
{
    return (int8_t*)base + offset;
}


void WriteIcon(Ptr iconp, const char * title)
{
} /* WriteIcon */


void WriteIcl(void * icon, int16_t depth, const char * title)
{
} /* WriteIcl */


void WriteCIcon(Ptr miscData, const char * title)
{
} /* WriteCIcon */


void usage()
{
	fprintf(stderr, "Usage: SlotsParse rom_path\n");
	exit(1);
}


void WriteSInfoRecordAndFHeaderRec(Ptr &sResDirFromHead, Ptr &sRootDirFromXHead)
{
	XFHeaderRec xfh;
	FHeaderRecPtr fh = (FHeaderRecPtr)&xfh.fhXDirOffset;

	sResDirFromHead = NULL;
	sRootDirFromXHead = NULL;
	if (gCoveredPtr) {
		free(gCoveredPtr);
		gCoveredPtr = NULL;
		numToCover = 0;
	}

	memset(&xfh, 0, sizeof(xfh));

	if (gTopOfRom - gStartOfRom < sizeof(FHeaderRec))
	{
		fprintf(stderr, "Rom size %" PRId64 " is too small\n", int64_t(gTopOfRom - gStartOfRom));
		exit(1);
	}

	Ptr headerWhere = CalcAddr(gTopOfRom, int32_t(-sizeof(FHeaderRec)));
	Ptr xHeaderWhere = CalcAddr(gTopOfRom, int32_t(-sizeof(XFHeaderRec)));

	for (int i = 0; i < 2; i++) {
		if (gTopOfRom - gStartOfRom >= sizeof(XFHeaderRec))
		{
			GetBytes(xHeaderWhere, &xfh, sizeof(XFHeaderRec));
		}
		else
		{
			GetBytes(headerWhere, fh, sizeof(FHeaderRec));
			xHeaderWhere = NULL;
		}
		BE_TO_HOST_32(xfh.fhXSuperInit);
		BE_TO_HOST_32(xfh.fhXSDirOffset);
		BE_TO_HOST_32(xfh.fhXEOL);
		BE_TO_HOST_32(xfh.fhXSTstPat);
		BE_TO_HOST_32(xfh.fhXDirOffset);
		BE_TO_HOST_32(xfh.fhXLength);
		BE_TO_HOST_32(xfh.fhXCRC);
		BE_TO_HOST_32(xfh.fhXTstPat);

		if (fh->fhTstPat == ~testPattern) {
			fprintf(gOutFile, "Rom is inverted!\n");
			for (uint8_t *p = (uint8_t *)gStartOfRom; p < (uint8_t *)gTopOfRom; p++)
				*p = ~*p;
		} else
			break;
	}

	if (fh->fhLength > 0)
	{
		Ptr calcStartOfRom = CalcAddr(gTopOfRom, -fh->fhLength);
		if (calcStartOfRom < gStartOfRom)
		{
			fprintf(stderr, "Rom size is only %" PRId64 " but header says it should be %" PRId64 "\n", int64_t(gTopOfRom - gStartOfRom), int64_t(fh->fhLength));
			exit(1);
		}
		if (calcStartOfRom > gStartOfRom)
		{
			fprintf(stderr, "Rom size is %" PRId64 " but header says only %" PRId64 " is used for slot rom data\n", int64_t(gTopOfRom - gStartOfRom), int64_t(fh->fhLength));
			gStartOfRom = calcStartOfRom;
		}
	}

	if (gStartOfRom < gTopOfRom)
	{
		gByteLanes = xfh.fhXByteLanes & 0xF;

		numToCover = int32_t(gTopOfRom - gStartOfRom + 8) & ~7;
		gCoveredPtr = (Ptr)malloc(numToCover / 8);
		memset(gCoveredPtr, 0, size_t(numToCover / 8));

		if (xHeaderWhere && xfh.fhXSTstPat == testPattern) {
			WriteXFHeaderRec(xfh, sRootDirFromXHead, xHeaderWhere);
		}

		fprintf(gOutFile, "\n");
		fprintf(gOutFile, "FHeaderRec:\n");
		WriteFHeaderRec(*fh, sResDirFromHead, headerWhere);
		SetCovered(headerWhere, int32_t(gTopOfRom-headerWhere));
	}

	gLevel = -1;
} /* WriteSInfoRecordAndFHeaderRec */


void SlotsParseMain()
{
	Ptr sResDirFromHead;
	Ptr sRootDirFromXHead;

	InitLoopingGlobals();

	gDoCovering = true;
	memset(&gSRsrcTypeData, -1, sizeof(gSRsrcTypeData));

	WriteSInfoRecordAndFHeaderRec(sResDirFromHead, sRootDirFromXHead);

	if (sResDirFromHead || sRootDirFromXHead)
	{
		if (sRootDirFromXHead)
		{
			fprintf(gOutFile, "\n");
			fprintf(gOutFile, "Root Directory from Extended Header:\n");
			WriteSResourceDirectory(sRootDirFromXHead, rootDir, "(root dir) ");
			CheckAddrRange(sRootDirFromXHead);
		}

		if (sResDirFromHead)
		{
			fprintf(gOutFile, "\n");
			fprintf(gOutFile, "Slot Resource Directory from Header:\n");
			WriteSResourceDirectory(sResDirFromHead, rsrcDirDir, "(from header) ");
		}

		fprintf(gOutFile, "\n");

		CheckAddrRange(sResDirFromHead);

		fprintf(gOutFile, "minAddr = %08" PRIXPTR "   maxAddr = %08" PRIXPTR "\n", OutAddr(gMinAddr), OutAddr(gMaxAddr));
		fprintf(gOutFile, "\n");
		WriteUncoveredBytes();
	} /* if */
} /* SlotsParseMain */


int main(int argc, char **argv)
{
	if (argc != 2) {
		usage();
	}

	FILE * romFile = fopen(argv[1], "r");
	if (!romFile) {
		fprintf(stderr, "Could not open \"%s\"\n", argv[1]);
		exit(1);
	}

	fseek(romFile, 0, SEEK_END);
	size_t size = ftell(romFile);
	if (size > 0x1000000) {
		fprintf(stderr, "Rom size %" PRId64 " is too big\n", int64_t(size));
		exit(1);
	}
	gRomFileSize = uint32_t(size);
	
	fseek(romFile, 0, SEEK_SET);
	void *romData = malloc(gRomFileSize);
	if (!romData) {
		fprintf(stderr, "Not enough memory\n");
		exit(1);
	}

	fread(romData, 1, gRomFileSize, romFile);
	fclose(romFile);

	gOutFile = stdout;
	gTopOfRom = (Ptr)romData + gRomFileSize;
	gStartOfRom = (Ptr)romData;
	gAddrAfterRom = 0xFEFFFFFF + 1; // slot E

	SlotsParseMain();
} /* Main */

#include "SlotsCommon.h"

dirListEntryPtr gDirList = NULL;
int32_t gDirListSize = 0;

Ptr* gAddrList = NULL;
int32_t gAddrListSize = 0;

Ptr gTopOfRom = NULL;
Ptr gStartOfRom = NULL;
size_t gAddrAfterRom = NULL;
int8_t gByteLanes = 0;	// 0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F

sRsrcTypeRec gSRsrcTypeData;               /* current sRsrc type */

Ptr gMinAddr, gMaxAddr;

FILE * gOutFile;

int32_t numToCover = 0;
Ptr gCoveredPtr = NULL;
bool gDoCovering = false;

int16_t gLevel;


std::string GetTab(int i)
{
	return std::string(i, 9);
}


void SetCovered(void * p, int32_t len)
{
	int32_t bitNum;
	int32_t numBits;

	if (gDoCovering && gCoveredPtr)
	{
		numBits = int32_t(gTopOfRom - (Ptr)p);
		if (numBits < 0)
		{
			printf("topOfRom:%08" PRIXPTR " p:%08" PRIXPTR " numBits:%" PRId32 "\n", OutAddr(gTopOfRom), OutAddr(p), numBits);
			fprintf(gOutFile, "topOfRom:%08" PRIXPTR " p:%08" PRIXPTR " numBits:%" PRId32 "\n", OutAddr(gTopOfRom), OutAddr(p), numBits);
		}
		for (bitNum = numBits - len; bitNum < numBits; bitNum++)
			if (bitNum >= 0)
				if (bitNum < numToCover)
					BitSet(gCoveredPtr, bitNum);
	}
}


void WriteCString(char *p)
{
	while (*p)
	{
		unsigned char ch = (unsigned char)*p;
		if (ch < ' ') ch = '.';
		fputc(ch, gOutFile);
		p++;
	}
}


std::string Hex(const void *hexPtr, int numBytes)
{
	char buf[200];
	buf[0] = '\0';
	int len = 0;
	while (numBytes > 0) {
		len += snprintf(buf + len, sizeof(buf) - len, "%02" PRIX8 "", *(uint8_t*)hexPtr);
        hexPtr = (uint8_t*)hexPtr + 1;
		numBytes--;
	}
	return buf;
}


void WriteChars(uint8_t *p, int32_t numChars)
{
	while (numChars > 0)
	{
		unsigned char ch = (unsigned char)*p;
		if (ch < ' ') ch = '.';
		fputc(ch, gOutFile);
		p++;
		numChars--;
	}
}


bool FindAddrEntry(Ptr findAddr)
{
	if (!findAddr)
		return false;

	int32_t i;
	Ptr * ae = gAddrList;
	for (i = 0; i < gAddrListSize / sizeof(Ptr); i++, ae++) {
		if (findAddr == *ae)
			return true;
	}
	return false;
}


void AddAddrEntry(Ptr newAddr)
{
	if (!newAddr)
		return;

	int32_t curLength = gAddrListSize;
	gAddrListSize += sizeof(Ptr);
	gAddrList = (Ptr*)realloc(gAddrList, (size_t)gAddrListSize);
	Ptr* ae = (Ptr*)((Ptr)gAddrList + curLength);
	*ae = newAddr;
}


void WriteUncoveredBytes()
{
	Ptr addr;
	int32_t count;
	int32_t total;
	uint8_t bytes[32];
	int16_t i;
	int32_t bitNum;

	if (!gCoveredPtr)
		return;

	gDoCovering = false;
	bitNum = numToCover - 1;

	while ((bitNum >= 0) && !BitTst(gCoveredPtr,bitNum))
		bitNum = bitNum - 1;

	while (bitNum >= 0)
	{
		while ((bitNum >= 0) && BitTst(gCoveredPtr,bitNum))
			bitNum = bitNum - 1;

		total = 0;
		if (bitNum >= 0)
		{
			addr = gTopOfRom - bitNum - 1;
			count = 0;
			fprintf(gOutFile, "%08" PRIXPTR ": ", OutAddr(addr));
			while ((bitNum >= 0) && !BitTst(gCoveredPtr,bitNum) && (count == 0 || !FindAddrEntry(addr)))
			{
				bitNum = bitNum - 1;
				bytes[count] = (uint8_t)*addr;
				count++;
				addr++;
				if (count > 31)
				{
					for (i = 0; i < 16; i++)
						fprintf(gOutFile, "%02" PRIX8 "%02" PRIX8 " ", bytes[i*2], bytes[i*2+1]);
					fprintf(gOutFile, " %s", kOpenQuote);
					WriteChars(bytes, 32);
					fprintf(gOutFile, "%s\n", kCloseQuote);
					fprintf(gOutFile, "%08" PRIXPTR ": ", OutAddr(addr));
					count = 0;
					total += 32;
				}
			} /* while */
			if (count != 0)
			{
				for (i = 0; i < 32; i++)
				{
					if (i < count)
						fprintf(gOutFile, "%02" PRIX8 "", bytes[i]);
					else
						fprintf(gOutFile, "  ");
					if (i % 2 == 1)
						fprintf(gOutFile, " ");
				}
				fprintf(gOutFile, " %s", kOpenQuote);
				WriteChars(bytes, count);
				fprintf(gOutFile, "%s\n", kCloseQuote);
				fprintf(gOutFile, "%08" PRIXPTR ": ", OutAddr(addr));
			}
			total += count;

		} /* if */
		fprintf(gOutFile, "(%" PRId32 " = %08" PRIX32 " bytes)\n", total, total);
		fprintf(gOutFile, "\n");
	} /* while */
	gDoCovering = true;
} /* WriteUncoveredBytes */


int32_t Get3(int32_t n)
{
	return (n << 8) >> 8;
}


bool CheckDataAddr(void * dataAddr)
{
	if (!dataAddr) {
		fprintf(gOutFile, "no data expected\n");
		return false;
	}
	return true;
}


uint32_t CalcChecksum(Ptr start, int32_t len)
{
    uint32_t sum = 0;
	while (len > 0) {
        // rotate sum left by one bit
        if (sum & 0x80000000UL)
            sum = (sum << 1) | 1;
        else
            sum = (sum << 1) | 0;
		if (len < 9 || len > 12)
			sum += *(uint8_t*)start;
		start = CalcAddr(start, 1);
		len--;
	}
	return sum;
}


void GetBytes(void* source, void* dest, int32_t numBytes, bool doSetCovered)
{
	if (!CheckDataAddr(source))
		return;

	char* startPtr = (char*)source;
	while (numBytes > 0)
	{
		if (source < gTopOfRom)
		{
			*(char*)dest = *(char*)source;
            dest = (char*)dest + 1;
			source = CalcAddr(source, 1);
			numBytes--;
		}
		else
		{
			printf("Bytes would exceed rom length numBytes:%" PRId32 " start:%" PRIXPTR " source:%" PRIXPTR "\n", numBytes, OutAddr(startPtr), OutAddr(source));
			fprintf(gOutFile, "Bytes would exceed rom length numBytes:%" PRId32 " start:%" PRIXPTR " source:%" PRIXPTR "\n", numBytes, OutAddr(startPtr), OutAddr(source));
			fflush(gOutFile);
			break;
		}
	}
	if (doSetCovered)
		SetCovered(startPtr, int32_t((char*)source - startPtr));
}


void RsrcToIdAndOffset(int32_t rsrc, uint8_t &s, int32_t &offset)
{
	s = uint8_t(uint32_t(rsrc) >> 24);
	offset = Get3(rsrc);
}


void GetRsrc(Ptr p, uint8_t &s, int32_t &offset)
{
	GetBytes(p, &offset, 4);
	BE_TO_HOST_32(offset);
	RsrcToIdAndOffset(offset, s, offset);
}


int32_t GetSBlock(Ptr source, Ptr &dest)
{
	int32_t len;

	GetBytes(source, &len, 4);
	BE_TO_HOST_32(len);
	len = len - 4;
    dest = (Ptr)realloc(dest, (size_t)len);
	source = CalcAddr(source, 4);
	GetBytes(source, dest, len);
	return len;
}


void GetCString(Ptr source, Ptr &dest)
{
	if (!CheckDataAddr(source))
		return;

	int32_t len;
	Ptr destPtr;
	Ptr startPtr;

	startPtr = source;

	len = 0;
	do {
        len++;
        dest = (Ptr)realloc(dest, (size_t)len);
        destPtr = dest + len - 1;
        if (source < gTopOfRom) {
            *destPtr = *source;
            source = CalcAddr(source, 1);
        } else {
            *destPtr = '\0';
			printf("CString would exceed rom length\n");
			fprintf(gOutFile, "CString would exceed rom length\n");
        }
	} while (!(*destPtr == 0));

	// include second null for even length strings

	if (len & 1)
		if (source < gTopOfRom)
			if (!*source)
				source = CalcAddr(source, 1);

	SetCovered(startPtr, int32_t(source - startPtr));
}


void WriteByteLanes(int8_t theByteLanes)
{
	fprintf(gOutFile, "= byte lanes: ");
	switch (theByteLanes) {
		case 0x1: fprintf(gOutFile, "0\n"); break;
		case 0x2: fprintf(gOutFile, "1\n"); break;
		case 0x3: fprintf(gOutFile, "0,1\n"); break;
		case 0x4: fprintf(gOutFile, "2\n"); break;
		case 0x5: fprintf(gOutFile, "0,2\n"); break;
		case 0x6: fprintf(gOutFile, "1,2\n"); break;
		case 0x7: fprintf(gOutFile, "0,1,2\n"); break;
		case 0x8: fprintf(gOutFile, "3\n"); break;
		case 0x9: fprintf(gOutFile, "0,3\n"); break;
		case 0xA: fprintf(gOutFile, "1,3\n"); break;
		case 0xB: fprintf(gOutFile, "0,1,3\n"); break;
		case 0xC: fprintf(gOutFile, "2,3\n"); break;
		case 0xD: fprintf(gOutFile, "0,2,3\n"); break;
		case 0xE: fprintf(gOutFile, "1,2,3\n"); break;
		case 0xF: fprintf(gOutFile, "0,1,2,3\n"); break;
		default : fprintf(gOutFile, "illegal\n"); break;
	}
} /* WriteByteLanes */


void WriteByteLanes2(uint8_t theByteLanes)
{
	switch (theByteLanes) {
		case 0xE1:
		case 0xD2:
		case 0xC3:
		case 0xB4:
		case 0xA5:
		case 0x96:
		case 0x87:
		case 0x78:
		case 0x69:
		case 0x5A:
		case 0x4B:
		case 0x3C:
		case 0x2D:
		case 0x1E:
		case 0x0F: WriteByteLanes(int8_t(theByteLanes & 0x0F)); break;
		default  : fprintf(gOutFile, "= byte lanes: illegal\n"); break;
	}
} /* WriteByteLanes2 */



void WriteFHeaderRec(FHeaderRec &fh, Ptr &sResDirFromHead, Ptr headerWhere)
{
	int32_t theOffset = Get3(fh.fhDirOffset);

	sResDirFromHead = CalcAddr(gTopOfRom, theOffset - int32_t(sizeof(FHeaderRec)));

	fprintf(gOutFile, "%08" PRIXPTR "\n", OutAddr(headerWhere));

	fprintf(gOutFile, "%20s%02" PRIX8 " %06" PRIX32 " = %08" PRIXPTR "\n",  "fhDirOffset: ", uint8_t(uint32_t(fh.fhDirOffset) >> 24), fh.fhDirOffset & 0x00ffffff, OutAddr(sResDirFromHead));
	fprintf(gOutFile, "%20s%" PRId32 " ; Start of ROM: %08" PRIXPTR "\n", "fhLength: ", fh.fhLength, OutAddr(gStartOfRom));

	fprintf(gOutFile, "%20s%08" PRIX32, "fhCRC: ", fh.fhCRC);
	uint32_t checksum = CalcChecksum(gStartOfRom, fh.fhLength);
	if (fh.fhCRC == checksum)
		fprintf(gOutFile, " = ok\n");
	else
		fprintf(gOutFile, " = bad (expected: %08" PRIX32 ")\n", checksum);

	fprintf(gOutFile, "%20s%" PRId8 "", "fhROMRev: ", fh.fhROMRev);
	if (fh.fhROMRev == romRevision)
		fprintf(gOutFile, " = romRevision\n");
	else
		fprintf(gOutFile, "\n");

	fprintf(gOutFile, "%20s%" PRId8 "", "fhFormat: ", fh.fhFormat);
	if (fh.fhFormat == appleFormat)
		fprintf(gOutFile, " = appleFormat\n");
	else
		fprintf(gOutFile, "\n");

	fprintf(gOutFile, "%20s%08" PRIX32 "", "fhTstPat: ", fh.fhTstPat);
	if (fh.fhTstPat == testPattern)
		fprintf(gOutFile, " = testPattern\n");
	else
		fprintf(gOutFile, "\n");

	fprintf(gOutFile, "%20s%02" PRIX8 "\n", "fhReserved: ", (uint8_t)fh.fhReserved);

	fprintf(gOutFile, "%20s%02" PRIX8 " ", "fhByteLanes: ", (uint8_t)fh.fhByteLanes);
	WriteByteLanes2(uint8_t(fh.fhByteLanes));
} /* WriteFHeaderRec */


void WriteXFHeaderRec(XFHeaderRec &xfh, Ptr &sRootDirFromXHead, Ptr xHeaderWhere)
{
	sRootDirFromXHead = xHeaderWhere;

	SetCovered(xHeaderWhere, int32_t(gTopOfRom-xHeaderWhere));
	fprintf(gOutFile, "\n");
	fprintf(gOutFile, "XFHeaderRec:\n");
	fprintf(gOutFile, "%08" PRIXPTR "\n", OutAddr(xHeaderWhere));
	fprintf(gOutFile, "%20s%02" PRIX8 " %06" PRIX32 " = %08" PRIXPTR "\n", "fhXSuperInit: " , uint8_t(xfh.fhXSuperInit  >> 24), xfh.fhXSuperInit  & 0x00ffffff, OutAddr(CalcAddr(CalcAddr(xHeaderWhere, 0), Get3(xfh.fhXSuperInit ))));
	fprintf(gOutFile, "%20s%02" PRIX8 " %06" PRIX32 " = %08" PRIXPTR "\n", "fhXSDirOffset: ", uint8_t(xfh.fhXSDirOffset >> 24), xfh.fhXSDirOffset & 0x00ffffff, OutAddr(CalcAddr(CalcAddr(xHeaderWhere, 4), Get3(xfh.fhXSDirOffset))));
	fprintf(gOutFile, "%20s%02" PRIX8 " %06" PRIX32 "\n", "fhXEOL: ", uint8_t(xfh.fhXEOL >> 24), xfh.fhXEOL & 0x00ffffff);
	fprintf(gOutFile, "%20s%08" PRIX32 "", "fhXSTstPat: ", xfh.fhXSTstPat);
	if (xfh.fhXSTstPat == testPattern)
		fprintf(gOutFile, " = testPattern\n");
	else
		fprintf(gOutFile, "\n");
}


void WriteTable()
{
	fprintf(gOutFile, "sResource Directory Ids:\n");
	fprintf(gOutFile, "%20s%s", "name", " $id id  Description\n");
	fprintf(gOutFile, "%20s%s", "board", "  0   0  Board sResource - Required on all boards\n");
	fprintf(gOutFile, "%20s%s", "endOfList", " FF 255 End of list\n");
	fprintf(gOutFile, "\n");
	fprintf(gOutFile, "sResource field Ids:\n");
	fprintf(gOutFile, "%20s%s", "name", " $id id  Description\n");
	fprintf(gOutFile, "%20s%s", "sRsrcType", "  1   1  Type of the sResource\n");
	fprintf(gOutFile, "%20s%s", "sRsrcName", "  2   2  Name of the sResource\n");
	fprintf(gOutFile, "%20s%s", "sRsrcIcon", "  3   3  Icon for the sResource\n");
	fprintf(gOutFile, "%20s%s", "sRsrcDrvr", "  4   4  Driver directory for the sResource\n");
	fprintf(gOutFile, "%20s%s", "sRsrcLoadRec", "  5   5  Load record for the sResource\n");
	fprintf(gOutFile, "%20s%s", "sRsrcBootRec", "  6   6  Boot record\n");
	fprintf(gOutFile, "%20s%s", "sRsrcFlags", "  7   7  sResource flags\n");
	fprintf(gOutFile, "%20s%s", "sRsrcHWDevId", "  8   8  Hardware device ID\n");
	fprintf(gOutFile, "%20s%s", "minorBaseOS", "  A  10  Offset from dCtlDevBase to the sResource" kApostrophe "s hardware base in standard slot space\n");
	fprintf(gOutFile, "%20s%s", "minorLength", "  B  11  Length of the sResource" kApostrophe "s address space in standard slot space\n");
	fprintf(gOutFile, "%20s%s", "majorBaseOS", "  C  12  Offset from dCtlDevBase to the sResource" kApostrophe "s address space in standard slot space\n");
	fprintf(gOutFile, "%20s%s", "majorLength", "  D  13  Length of the sResource in super slot space\n");
	fprintf(gOutFile, "%20s%s", "sRsrccicn", "  F  15  Color icon\n");
	fprintf(gOutFile, "%20s%s", "sRsrcicl8", " 10  16  8-bit icon data\n");
	fprintf(gOutFile, "%20s%s", "sRsrcicl4", " 11  17  4-bit icon data\n");
	fprintf(gOutFile, "%20s%s", "sGammaDir", " 40  64  Gamma directory\n");
	fprintf(gOutFile, "\n");
	fprintf(gOutFile, "%20s%s", "sDRVRDir", " 10  16  sDriver directory\n");
	fprintf(gOutFile, "\n");
	fprintf(gOutFile, "%20s%s", "endOfList", " FF 255  End of list\n");
	fprintf(gOutFile, "\n");
	fprintf(gOutFile, "\n");
	fprintf(gOutFile, "Format of sRsrcType: Category, cType, DrSW, DrHW\n");
	fprintf(gOutFile, "\n");
}


const char * GetRsrcKind(uint8_t sRsrcId, DirType whichDir)
{
	if (sRsrcId == 255)
		return " ; endOfList       ";

	switch (whichDir) {

		case rootDir:
			switch (sRsrcId) {
				case appleFormat: return " ; appleFormat     ";
				case sSuperDir  : return " ; sSuperDir       ";
				default         : return " ; sRsrcUnknown    ";
			}

		case superDir:
			switch (sRsrcId) {
				case    1: return " ; sRsrcUnknownDir    ";
				case 0x7B: return " ; sRsrcATIDir        ";
				case 0x7C: return " ; sRsrcCSCDir        ";
				case 0x7D: return " ; sRsrcCivicDir      ";
				case 0x7E: return " ; sRsrcSonoraDir     ";
				case 0x7F: return " ; sRsrcBFBasedDir    ";
				default  : return " ; sRsrcUnknown       ";
			}

		case rsrcUnknownDir:
			switch (sRsrcId) {
				case  1: return " ; sRsrcUnknownBd  ";
				default: return " ; sRsrcUnknown    ";
			}

		case rsrcDirDir:
			if (sRsrcId == 0)
				return " ; board           ";
			else
				return "                   ";

		case rsrcDir:
		case gammaDir:
			switch (sRsrcId) {
				case 1 : return " ; sRsrcType       ";
				case 2 : return " ; sRsrcName       ";
				case 3 : return " ; sRsrcIcon       ";
				case 4 : return " ; sRsrcDrvrDir    ";
				case 5 : return " ; sRsrcLoadRec    ";
				case 6 : return " ; sRsrcBootRec    ";
				case 7 : return " ; sRsrcFlags      ";
				case 8 : return " ; sRsrcHWDevId    ";
				case 10: return " ; minorBaseOS     ";
				case 11: return " ; minorLength     ";
				case 12: return " ; majorBaseOS     ";
				case 13: return " ; majorLength     ";
				case 15: return " ; sRsrcCicn       ";
				case 16: return " ; sRsrcicl8       ";
				case 17: return " ; sRsrcicl4       ";
				case 64: return " ; sGammaDir       ";
				case 32: return " ; boardId         ";
				case 33: return " ; PRAMInitData    ";
				case 34: return " ; primaryInit     ";
				case 35: return " ; timeOutConst    ";
				case 36: return " ; vendorInfo      ";
				case 37: return " ; boardFlags      ";
				case 38: return " ; secondaryInit   ";
				case 65: return " ; sRsrcVidNames   ";
				default:
					switch (gSRsrcTypeData.category)
					{
						case catBoard:
							switch (sRsrcId) {
								case   20: return " ; sBlockTransferInfo ";
								case   21: return " ; sMaxLockedTransferCount ";
								case 0x7B: return " ; sVidAuxParams   ";
								case 0x7C: return " ; sDebugger       ";
								case 0x7E: return " ; sVidParmDir     ";
								case 0xB6: return " ; sUndefinedID    ";
								default  : return " ; sRsrcUnknown    ";
							}
						case catDisplay:
							switch (sRsrcId) {
								case 0x7D: return " ; sVidAttributes  ";
								case  128: return " ; 1:firstVidMode  ";
								case  129: return " ; 2:secondVidMode ";
								case  130: return " ; 4:thirdVidMode  ";
								case  131: return " ; 8:fourthVidMode ";
								case  132: return " ; 16:fifthVidMode ";
								case  133: return " ; 32:sixthVidMode ";
								default:
									if (whichDir == rsrcDir)
										return " ; sRsrcUnknown    ";
									else
										return " ; unknownVidMode  ";
							}
						case catCPU:
							switch (sRsrcId) {
								case 128: return " ; CPU_FlagsID     ";
								case 129: return " ; MajRAMSp        ";
								case 130: return " ; MinROMSp        ";
								default : return " ; sRsrcUnknown    ";
							}
						default:
							return " ; sRsrcUnknown    ";
					}
			}

		case drvrDir:
			switch (sRsrcId) {
				case 1 : return " ; sMacOS68000     ";
				case 2 : return " ; sMacOS68020     ";
				case 3 : return " ; sMacOS68030     ";
				case 4 : return " ; sMacOS68040     ";
				default: return " ; sMacOSUnknown   ";
			}

		case vendorDir:
			switch (sRsrcId) {
				case 1 : return " ; vendorID        ";
				case 2 : return " ; serialNum       ";
				case 3 : return " ; revLevel        ";
				case 4 : return " ; partNum         ";
				case 5 : return " ; date            ";
				default: return " ; unknownInfo     ";
			}

		case vidModeDir:
			switch (sRsrcId) {
				case 1 : return " ; mVidParams      ";
				case 2 : return " ; mTable          ";
				case 3 : return " ; mPageCnt        ";
				case 4 : return " ; mDevType        ";
				case 5 : return " ; mHRes           ";
				case 6 : return " ; mVRes           ";
				case 7 : return " ; mPixelType      ";
				case 8 : return " ; mPixelSize      ";
				case 9 : return " ; mCmpCount       ";
				case 10: return " ; mCmpSize        ";
				case 11: return " ; mPlaneBytes     ";
				case 14: return " ; mVertRefRate    ";
				default: return " ; mUnknown        ";
			}

		case loadDir:
		case vidNamesDir:
		case vidAuxParamsDir:
		case vidParmDir:
			return " ;                 ";

	} /* switch whichDir */
	return " ;                 ";
}


void WriteRsrcKind(uint8_t id, DirType whichDir)
{
	fprintf(gOutFile, "%s", GetRsrcKind(id, whichDir));
} /* WriteRsrcKind */


const char * GetspParamDataEnabledDisabledString(int32_t spParamData)
{
	switch (spParamData) {
		case 0 : return " ; Enabled";
		case 1 : return " ; Disabled";
		default: return " ; ???";
	}
} /* GetspParamDataEnabledDisabledString */


/* $S SlotsDump2 */

std::string CategoryString(int16_t category)
{
	const char *s;
	char buf[100];
	switch (category) {
		case catBoard  : s = "=CatBoard"; break;
		case catTest   : s = "=CatTest"; break;
		case catDisplay: s = "=CatDisplay"; break;
		case catNetwork: s = "=CatNetwork"; break;
		case catScanner: s = "=CatScanner"; break;
		case catCPU    : s = "=CatCPU"; break;
		case catIntBus : s = "=CatIntBus"; break;
		case catProto  : s = "=CatProto"; break;
		case catDock   : s = "=CatDock"; break;
		default        : s = "=CatUnknown"; break;
	}
	snprintf(buf, sizeof(buf), "%04" PRIX16 "%s", category, s);
	return buf;
}

std::string cTypeString(sRsrcTypeRec &theSRsrcTypeData)
{
	const char *s;
	char buf[100];
	switch (theSRsrcTypeData.cType) {
		case  0: s = "=TypBoard"; break;
		case  1:
			switch (theSRsrcTypeData.category) {
				case catBoard  : s = "=TypeApple"; break;
				case catDisplay: s = "=TypVideo"; break;
				case catNetwork: s = "=TypEthernet"; break;
				case catDock   : s = "=TypStation"; break;
				default        : s = "=TypeUnknown"; break;
		    }
		    break;
		case  2:
			switch (theSRsrcTypeData.category) {
				case catDisplay: s = "=TypLCD"; break;
				case catCPU    : s = "=Typ68000"; break;
				case catDock   : s = "=TypDesk"; break;
				default        : s = "=TypeUnnown"; break;
		    }
		    break;
		case  3:
			switch (theSRsrcTypeData.category) {
				case catCPU    : s = "=Typ68020"; break;
				default        : s = "=TypeUnnown"; break;
		    }
		    break;
		case  4:
			switch (theSRsrcTypeData.category) {
				case catCPU    : s = "=Typ68030"; break;
				default        : s = "=TypeUnnown"; break;
		    }
		    break;
		case  5:
			switch (theSRsrcTypeData.category) {
				case catCPU    : s = "=Typ68040"; break;
				default        : s = "=TypeUnnown"; break;
		    }
		    break;
		case 11:
			switch (theSRsrcTypeData.category) {
				case catIntBus : s = "=TypXPT"; break;
				default        : s = "=TypeUnnown"; break;
		    }
		    break;
		case 12:
			switch (theSRsrcTypeData.category) {
				case catIntBus : s = "=TypSIM"; break;
				default        : s = "=TypeUnnown"; break;
		    }
		    break;
		case 21:
			switch (theSRsrcTypeData.category) {
				case catCPU    : s = "=TypAppleII"; break;
				default        : s = "=TypeUnnown"; break;
		    }
		    break;
		default: s = "=TypUnknown"; break;
	}
	snprintf(buf, sizeof(buf), "%04" PRIX16 "%s", theSRsrcTypeData.cType, s);
	return buf;
}

std::string DrSWString(sRsrcTypeRec &theSRsrcTypeData)
{
	const char *s;
	char buf[100];
	switch (theSRsrcTypeData.DrSW) {
		case     0:
			switch (theSRsrcTypeData.category) {
				case catBoard  : s = "=DrSWBoard"; break;
				case catCPU    : s = "=DrSWMacCPU"; break;
				default        : s = "=DrSWUnknown"; break;
		    }
		    break;
		case     1:
			switch (theSRsrcTypeData.category) {
				case catCPU    : s = "=DrSWAppleIIe"; break;
				default        : s = "=DrSWApple"; break;
		    }
		    break;
		case     2: s = "=DrSWCompanyA"; break;
		case 0x101: s = "=DrSWDepewEngineering"; break;
		case 0x104: s = "=DrSWMacsBug"; break;
		default   : s = "=DrSWUnknown"; break;
	}
	snprintf(buf, sizeof(buf), "%04" PRIX16 "%s", theSRsrcTypeData.DrSW, s);
	return buf;
}

std::string DrHWString(sRsrcTypeRec &theSRsrcTypeData)
{
	const char *s;
	char buf[100];
	switch (theSRsrcTypeData.DrHW) {
		case   0:
			switch (theSRsrcTypeData.category) {
				case catBoard  : s = "=DrHWBoard"; break;
				default        : s = "=DrHWUnknown"; break;
		    }
		    break;
		case   1:
			switch (theSRsrcTypeData.category) {
				case catDisplay: s = "=DrHWTFB"; break;
				case catCPU    : s = "=DrHWDblExp"; break;
				case catNetwork: s = "=DrHW3Com"; break;
				case catDock   : s = "=DrHWDeskBar"; break;
//				case cat?      : s = "=DrHWATT3210"; break;
				default        : s = "=DrHWUnknown"; break;
		    }
		    break;
		case   2:
			switch (theSRsrcTypeData.category) {
//				case catDisplay: s = "=DrHWSTB3"; break; /* Assigned by Kevin Mellander for STB-3 hardware. */
				case catDisplay: s = "=DrHWSTB"; break; /* (Both STB-3 and STB-4 share the same video hardware.) */
				case catDock   : s = "=DrHWHooperDock"; break;
				default        : s = "=DrHWproductA"; break;
		    }
		    break;
		case   3:
			switch (theSRsrcTypeData.category) {
				case catDisplay: s = "=DrHWproductB"; break;
				default        : s = "=DrHWBSC"; break;
		    }
		    break;
		case   4: s = "=DrHWproductC"; break;
		case   5: s = "=DrHWproductD"; break;

		case 0x0100:
			switch (theSRsrcTypeData.category) {
				case catDisplay: s = "=DrHWMicroDock"; break; /* video hardware id's  - <catDisplay><typVideo>*/
				case catProto  : s = "=DrHWBootBug"; break;
				default        : s = "=DrHWUnknown"; break;
		    }
		    break;

		case 0x0018: s = "=DrHWRBV"; break; /* IIci Aurora25/16 hw ID */
		case 0x0019: s = "=DrHWJMFB"; break; /* 4¥8/8¥24 NuBus card */
		case 0x001A: s = "=DrHWElsie"; break; /* V8 */
		case 0x001B: s = "=DrHWTim"; break; /* TIM */
		case 0x001C: s = "=DrHWDAFB"; break;
		case 0x001D: s = "=DrHWDolphin"; break; /* 8¥24GC NuBus card */
		case 0x001E: s = "=DrHWGSC"; break; /* (Renamed from GSC drHWDBLite) */
		case 0x001F: s = "=DrHWDAFBPDS"; break;
		case 0x0020: s = "=DrHWVSC"; break;
		case 0x0021: s = "=DrHWApollo"; break;
		case 0x0022: s = "=DrHWSonora"; break;
		case 0x0023: s = "=DrHWReserved2"; break;
		case 0x0024: s = "=DrHWColumbia"; break;
		case 0x0025: s = "=DrHWCivic"; break;
		case 0x0026: s = "=DrHWBrazil"; break;
		case 0x0027: s = "=DrHWPBLCD"; break;
		case 0x0028: s = "=DrHWCSC"; break;
		case 0x0029: s = "=DrHWJET"; break;
		case 0x002A: s = "=DrHWMEMCjr"; break;
		case 0x002B: s = "=DrHWBoogie"; break; /* 8¥24AC nuBus video card (built by Radius) */
		case 0x002C: s = "=DrHWHPV"; break; /* High performance Video (HPV) PDS card for original PowerMacs */
		case 0x002D: s = "=DrHWPlanaria"; break; /*PowerMac 6100/7100/8100 PDS AV video*/
		case 0x002E: s = "=DRHwValkyrie"; break;
		case 0x002F: s = "=DrHWKeystone"; break;
		case 0x0055: s = "=DrHWATI"; break;
		case 0x0056: s = "=DrHWGammaFormula"; break; /* Use for gType of display mgr gamma tables */
		case 0x0110: s = "=DrHWSonic"; break;
		case 0x0114: s = "=DrHWMace"; break;

		default : s = "=DrHWUnknown"; break;
	}
	snprintf(buf, sizeof(buf), "%04" PRIX16 "%s", theSRsrcTypeData.DrHW, s);
	return buf;
}


std::string GetSRsrcTypeDataString(sRsrcTypeRec &theSRsrcTypeData)
{
	return CategoryString(theSRsrcTypeData.category) + " " + cTypeString(gSRsrcTypeData) + " " + DrSWString(gSRsrcTypeData) + " " + DrHWString(gSRsrcTypeData);
}


const char* BoardIDString(uint16_t boardID)
{
	switch (boardID) {
		case  MIIBoardId       : return " = MIIBoardId"       ; // 0x0010
		case  ciVidBoardID     : return " = ciVidBoardID"     ; // 0x001F
		case  CX16VidBoardID   : return " = CX16VidBoardID"   ; // 0x0020
		case  MIIxBoardId      : return " = MIIxBoardId"      ; // 0x0021
		case  SE30BoardID      : return " = SE30BoardID"      ; // 0x0022
		case  MIIcxBoardId     : return " = MIIcxBoardId"     ; // 0x0023
		case  MIIfxBoardId     : return " = MIIfxBoardId"     ; // 0x0024
		case  EricksonBoardID  : return " = EricksonBoardID"  ; // 0x0028
		case  ElsieBoardID     : return " = ElsieBoardID"     ; // 0x0029
		case  TIMBoardID       : return " = TIMBoardID"       ; // 0x002A
		case  EclipseBoardID   : return " = EclipseBoardID"   ; // 0x002B
		case  SpikeBoardID     : return " = SpikeBoardID"     ; // 0x0033
		case  DBLiteBoardID    : return " = DBLiteBoardID"    ; // 0x0035
		case  ZydecoBrdID      : return " = ZydecoBrdID"      ; // 0x0036
		case  ApolloBoardID    : return " = ApolloBoardID"    ; // 0x0038
		case  PDMBrdID         : return " = PDMBrdID"         ; // 0x0039
		case  VailBoardID      : return " = VailBoardID"      ; // 0x003A
		case  WombatBrdID      : return " = WombatBrdID"      ; // 0x003B
		case  ColumbiaBrdID    : return " = ColumbiaBrdID"    ; // 0x003C
		case  CycloneBrdID     : return " = CycloneBrdID"     ; // 0x003D
		case  CompanionBrdID   : return " = CompanionBrdID"   ; // 0x003E
		case  DartanianBoardID : return " = DartanianBoardID" ; // 0x0040
		case  DartExtVidBoardID: return " = DartExtVidBoardID"; // 0x0046
		case  HookBoardID      : return " = HookBoardID"      ; // 0x0047
		case  EscherBoardID    : return " = EscherBoardID"    ; // 0x004A
		case  POBoardID        : return " = POBoardID"        ; // 0x004D
		case  TempestBrdID     : return " = TempestBrdID"     ; // 0x0050
		case  BlackBirdBdID    : return " = BlackBirdBdID"    ; // 0x0058
		case  BBExtVidBdID     : return " = BBExtVidBdID"     ; // 0x0059
		case  YeagerBoardID    : return " = YeagerBoardID"    ; // 0x005A
		case  BBEtherNetBdID   : return " = BBEtherNetBdID"   ; // 0x005E
		case  TELLBoardID      : return " = TELLBoardID"      ; // 0x0065
		case  MalcolmBoardID   : return " = MalcolmBoardID"   ; // 0x065E
		case  AJBoardID        : return " = AJBoardID"        ; // 0x065F
		case  M2BoardID        : return " = M2BoardID"        ; // 0x0660
		case  OmegaBoardID     : return " = OmegaBoardID"     ; // 0x0661
		case  TNTBoardID       : return " = TNTBoardID"       ; // 0x0670
		case  HooperBoardID    : return " = HooperBoardID"    ; // 0x06CD
		case  BoardIDDblExp    : return " = BoardIDDblExp"    ; // 0x002F
		case  DAFBPDSBoardID   : return " = DAFBPDSBoardID"   ; // 0x0037
		case  MonetBoardID     : return " = MonetBoardID"     ; // 0x0048
		case  SacSONIC16BoardID: return " = SacSONIC16BoardID"; // 0x004E
		case  SacSONIC32BoardID: return " = SacSONIC32BoardID"; // 0x004F
		case  0                : return " ; for Unknown Macintosh";
		default                : return " ; Unknown Board ID";
	}
}


void WriteSExecBlock(std::string tabStr, Ptr dataAddr)
{
	if (!CheckDataAddr(dataAddr))
		return;

	sExecBlock se;

	tabStr += ltab1;
	GetBytes(dataAddr, &se, sizeof(se));
	BE_TO_HOST_32(se.blockSize);
	BE_TO_HOST_16(se.reserved);
	BE_TO_HOST_32(se.codeOffset);

	fprintf(gOutFile, "%20s%02" PRIX8 " %06" PRIX32 "\n", "block size = ", uint8_t(se.blockSize >> 24), se.blockSize & 0x00ffffff);
	SetCovered(dataAddr, int32_t(CalcAddr(dataAddr, int32_t(se.blockSize)) - CalcAddr(dataAddr,0)));

	fprintf(gOutFile,"%s%20s%" PRId8 "", tabStr.c_str(), "revision level: ", se.revisionLevel);
	if (se.revisionLevel == sCodeRev)
		fprintf(gOutFile, " = sCodeRev\n");
	else
		fprintf(gOutFile, "\n");

	fprintf(gOutFile, "%s%20s%" PRId8 "", tabStr.c_str(), "CPU ID: ", se.cpuID);
	switch (se.cpuID) {
		case  1: fprintf(gOutFile, " = sCPU68000\n"); break;
		case  2: fprintf(gOutFile, " = sCPU68020\n"); break;
		case  3: fprintf(gOutFile, " = sCPU68030\n"); break;
		case  4: fprintf(gOutFile, " = sCPU68040\n"); break;
		default: fprintf(gOutFile, "\n");
	} /* switch */

	fprintf(gOutFile, "%s%20s%08" PRIX32 " = %08" PRIXPTR "\n", tabStr.c_str(), "code offset: ", se.codeOffset, OutAddr(CalcAddr(dataAddr, 8 + se.codeOffset)));
} /* WriteSExecBlock */


void WriteSResourceDirectory(Ptr dirPtr, DirType whichDir, const std::string curStr);


void WritePRAMInitData(Ptr dataAddr)
{
	if (!CheckDataAddr(dataAddr))
		return;

	SPRAMRecord pd;

	GetBytes(dataAddr, &pd, sizeof(SPRAMRecord));
	BE_TO_HOST_32(pd.blockSize);
	BE_TO_HOST_16(pd.boardID);

	fprintf(gOutFile, "\n");

	fprintf(gOutFile, "%s%20s%02" PRIX8 " %06" PRIX32 "\n", GetTab(gLevel).c_str(), "block size = ", uint8_t(pd.blockSize >> 24), pd.blockSize & 0x00ffffff);
	SetCovered(dataAddr, int32_t(CalcAddr(dataAddr,int32_t(pd.blockSize))-CalcAddr(dataAddr,0)));

	fprintf(gOutFile, "%s%20s%04" PRIX16 "%s\n", GetTab(gLevel).c_str(), "boardID: ", pd.boardID, BoardIDString(pd.boardID));
	fprintf(gOutFile, "%s%20s%02" PRIX8 " %02" PRIX8 " %02" PRIX8 " %02" PRIX8 " %02" PRIX8 " %02" PRIX8 "\n", GetTab(gLevel).c_str(), "vendorUse: ", pd.vendorUse1, pd.vendorUse2, pd.vendorUse3, pd.vendorUse4, pd.vendorUse5, pd.vendorUse6);
}


void WriteSBlock(Ptr dataAddr, Ptr &miscData, const char *description)
{
	if (!CheckDataAddr(dataAddr))
		return;

	int16_t i;
	uint32_t longNum;

	int32_t miscDataLen = GetSBlock(dataAddr, miscData);
	longNum = (uint32_t)miscDataLen + 4;
	fprintf(gOutFile, "block size = %02" PRIX8 " %06" PRIX32 "  %s:", uint8_t(longNum >> 24), longNum & 0x00ffffff, description);
	longNum = (longNum - 4) / 2;
	const char *moreIndicator = "";
	if (longNum > 16 && strcmp(description, "sVidParms")) {
		longNum = 16;
		moreIndicator = " ...";
	}
	for (i = 0; i < longNum; i++)
		fprintf(gOutFile, " %02" PRIX8 "%02" PRIX8 "", (uint8_t)(miscData[i*2]), (uint8_t)(miscData[i*2+1]));
	fprintf(gOutFile, "%s\n", moreIndicator);
}


bool FindDirEntry(Ptr findAddr)
{
	if (!findAddr)
		return false;

	int32_t i;
	dirListEntryPtr de = gDirList;
	for (i = 0; i < gDirListSize / sizeof(dirListEntry); i++, de++) {
		if (findAddr == de->addr)
		{
			fprintf(gOutFile, "duplicate: %s\n", de->name);
			return true;
		}
	}
	return false;
}

void AddDirListEntry(Ptr newAddr, const char *newName)
{
	if (!newAddr)
		return;

	int32_t curLength = gDirListSize;
	gDirListSize += sizeof(dirListEntry);
	gDirList = (dirListEntryPtr)realloc(gDirList, (size_t)gDirListSize);
	dirListEntryPtr de = dirListEntryPtr((Ptr)gDirList + curLength);
	de->addr = newAddr;
	snprintf(de->name, sizeof(de->name), "%s", newName);
}


void CheckAddrRange(Ptr addr)
{
	if ((addr < gMinAddr) || (gMinAddr == NULL))
		gMinAddr = addr;
	else if ((addr > gMaxAddr) || (gMaxAddr == NULL))
		gMaxAddr = addr;
}


bool GetAddrResult(uint8_t sRsrcId, DirType whichDir, Ptr addr, char *buf)
{
	if (
		((whichDir == rsrcDir) && (sRsrcId == sRsrcFlags || sRsrcId == sRsrcHWDevId || sRsrcId == boardId || sRsrcId == timeOutConst || sRsrcId == boardFlags))
		||
		((whichDir == vidModeDir) && (sRsrcId == mPageCnt || sRsrcId == mDevType))
		||
		((whichDir == rsrcDir) && (gSRsrcTypeData.category == catCPU) && (sRsrcId == CPU_FlagsID))
		||
		((whichDir == rsrcDir) && (gSRsrcTypeData.category == catDisplay) && (sRsrcId == sVidAttributes))
	) {
		snprintf(buf, 9, "        ");
		return false;
	}

	CheckAddrRange(addr);
	snprintf(buf, 9, "%08" PRIXPTR "", OutAddr(addr));
	return true;
}


void GetAndWriteCString(Ptr dataAddr, Ptr &miscData)
{
	if (!CheckDataAddr(dataAddr))
		return;

	GetCString(dataAddr, miscData);
	fprintf(gOutFile, "%s", kOpenQuote);
	WriteCString((char*)miscData);
	fprintf(gOutFile, "%s\n", kCloseQuote);
}

uint8_t temp_bytes[1024];


void DoRsrcDirDir(uint8_t sRsrcId, DirType whichDir, Ptr dataAddr, const std::string curStr)
{
	fprintf(gOutFile, "\n");
	if (dataAddr && sRsrcId != endOfList)
		WriteSResourceDirectory(dataAddr, DirType(whichDir + 1), curStr);
}


void DoRsrcDir(uint8_t sRsrcId, int32_t offset, Ptr dataAddr, Ptr &miscData, const std::string curStr)
{
	uint32_t longNum;
	uint32_t longNums[2];

	switch (sRsrcId) {
		case sRsrcType:
		{
			if (!CheckDataAddr(dataAddr))
				return;
			GetBytes(dataAddr, &gSRsrcTypeData, 8);
			BE_TO_HOST_16(gSRsrcTypeData.category);
			BE_TO_HOST_16(gSRsrcTypeData.cType);
			BE_TO_HOST_16(gSRsrcTypeData.DrSW);
			BE_TO_HOST_16(gSRsrcTypeData.DrHW);

			fprintf(gOutFile, "%s\n", GetSRsrcTypeDataString(gSRsrcTypeData).c_str());
			break;
		}
		case sRsrcName:
			if (!CheckDataAddr(dataAddr))
				return;
			GetAndWriteCString(dataAddr, miscData);
			break;
		case sRsrcIcon:
		{
			if (!CheckDataAddr(dataAddr))
				return;
            GetBytes(dataAddr, &temp_bytes[0], 128);
			WriteIcon((Ptr)(&temp_bytes[0]), curStr.c_str());
			fprintf(gOutFile, "\n");
			break;
		}
		case sRsrcDrvrDir:
		{
			if (!CheckDataAddr(dataAddr))
				return;
			AddDirListEntry(dataAddr, curStr.c_str());
			fprintf(gOutFile, "\n");
			WriteSResourceDirectory(dataAddr, drvrDir, curStr);
			break;
		}
		case sRsrcLoadDir:
		{
			if (!CheckDataAddr(dataAddr))
				return;
			AddDirListEntry(dataAddr, curStr.c_str());
			fprintf(gOutFile, "\n");
			WriteSResourceDirectory(dataAddr, loadDir, curStr);
			break;
		}
		case sRsrcBootRec:
		case primaryInit:
		case secondaryInit:
			if (!CheckDataAddr(dataAddr))
				return;
			WriteSExecBlock(GetTab(gLevel), dataAddr);
			break;
		case sRsrcFlags:
			fprintf(gOutFile, "%04" PRIX16 " %d:f32BitMode=%d  %d:fOpenAtStart=%d\n", uint16_t(offset),
				int(f32BitMode)  , int(offset >> f32BitMode  ) & 1,
				int(fOpenAtStart), int(offset >> fOpenAtStart) & 1
			);
			break;
		case sRsrcHWDevId:
			fprintf(gOutFile, "%02" PRIX8 "\n", (uint8_t)offset);
			break;
		case minorBaseOS:
		{
			if (!CheckDataAddr(dataAddr))
				return;
			GetBytes(dataAddr, &longNum, 4);
			BE_TO_HOST_32(longNum);
			fprintf(gOutFile, "$Fs00 0000 + %08" PRIX32 "\n", longNum);
			break;
		}
		case majorBaseOS:
		{
			if (!CheckDataAddr(dataAddr))
				return;
			GetBytes(dataAddr, &longNum, 4);
			BE_TO_HOST_32(longNum);
			fprintf(gOutFile, "$s000 0000 + %08" PRIX32 "\n", longNum);
			break;
		}
		case minorLength:
		case majorLength:
		{
			if (!CheckDataAddr(dataAddr))
				return;
			GetBytes(dataAddr, &longNum, 4);
			BE_TO_HOST_32(longNum);
			fprintf(gOutFile, "%08" PRIX32 "\n", longNum);
			break;
		}
		case sRsrccicn:
		{
			if (!CheckDataAddr(dataAddr))
				return;
			WriteCIcon(miscData, curStr.c_str());
			fprintf(gOutFile, "\n");
			break;
		}

		case sRsrcicl8:
		{
			if (!CheckDataAddr(dataAddr))
				return;
			GetBytes(dataAddr, &temp_bytes[0], 1024);
			WriteIcl(&temp_bytes[0], 8, curStr.c_str());
			fprintf(gOutFile, "\n");
			break;
		}
		case sRsrcicl4:
		{
			if (!CheckDataAddr(dataAddr))
				return;
			GetBytes(dataAddr, &temp_bytes[0], 512);
			WriteIcl(&temp_bytes[0], 4, curStr.c_str());
			fprintf(gOutFile, "\n");
			break;
		}

		case sGammaDir:
		{
			if (!CheckDataAddr(dataAddr))
				return;
			AddDirListEntry(dataAddr, curStr.c_str());
			fprintf(gOutFile, "\n");
			WriteSResourceDirectory(dataAddr, gammaDir, curStr);
			break;
		}

		case boardId:
			fprintf(gOutFile, "%04" PRIX16 "%s\n", uint16_t(offset), BoardIDString(uint16_t(offset)));
			break;

		case timeOutConst:
		case boardFlags:
			fprintf(gOutFile, "%04" PRIX16 "\n", uint16_t(offset));
			break;

		case pRAMInitData:
			if (!CheckDataAddr(dataAddr))
				return;
			WritePRAMInitData(dataAddr);
			break;

		case vendorInfo:
		{
			if (!CheckDataAddr(dataAddr))
				return;
			AddDirListEntry(dataAddr, curStr.c_str());
			fprintf(gOutFile, "\n");
			WriteSResourceDirectory(dataAddr, vendorDir, curStr);
			break;
		}

		case sRsrcVidNames:
		{
			if (!CheckDataAddr(dataAddr))
				return;
			AddDirListEntry(dataAddr, curStr.c_str());
			fprintf(gOutFile, "\n");
			WriteSResourceDirectory(dataAddr, vidNamesDir, curStr);
			break;
		}

		case firstVidMode:
		case secondVidMode:
		case thirdVidMode:
		case fourthVidMode:
		case fifthVidMode:
		case sixthVidMode:
			if (gSRsrcTypeData.category == catDisplay)
			{
				if (!CheckDataAddr(dataAddr))
					return;
				AddDirListEntry(dataAddr, curStr.c_str());
				fprintf(gOutFile, "\n");
				WriteSResourceDirectory(dataAddr, vidModeDir, curStr);
			}
			else if (gSRsrcTypeData.category == catCPU && (sRsrcId == MajRAMSp || sRsrcId == MinROMSp))
			{
				if (!CheckDataAddr(dataAddr))
					return;
				GetBytes(dataAddr, &longNums, 8);
				BE_TO_HOST_32(longNums[0]);
				BE_TO_HOST_32(longNums[1]);
				fprintf(gOutFile, "%08" PRIX32 " %08" PRIX32 "\n", longNums[0], longNums[1]);
			}
			else if (gSRsrcTypeData.category == catCPU && sRsrcId == CPU_FlagsID)
			{
				fprintf(gOutFile, "%04" PRIX32 " "
					" 0:hasPixelClock=%d"
					" 1:hasIWM=%d"
					" 2:hasJoystick=%d"
					" 15:hasDERegs=%d"
					"\n",
					offset,
					int(offset >> hasPixelClock) & 1,
					int(offset >> hasIWM) & 1,
					int(offset >> hasJoystick) & 1,
					int(offset >> hasDERegs) & 1
				);
			}
			else
			{
				if (!CheckDataAddr(dataAddr))
					return;
				GetBytes(dataAddr, &longNum, 4);
				BE_TO_HOST_32(longNum);
				fprintf(gOutFile, "%08" PRIX32 "\n", longNum);
			}
			break;

		default:
			switch (gSRsrcTypeData.category)
			{
				case catBoard:
					switch (sRsrcId) {
						case  sBlockTransferInfo:
							{
								if (!CheckDataAddr(dataAddr))
									return;
								GetBytes(dataAddr, &longNum, 4);
								BE_TO_HOST_32(longNum);
								fprintf(gOutFile, "%08" PRIX32 " "
									" 31:fIsMaster=%d"
									" 30:fMstrLockedXfer=%d"
									" 19:fMstrXferSz16=%d"
									" 18:fMstrXferSz8=%d"
									" 17:fMstrXferSz4=%d"
									" 16:fMstrXferSz2=%d"
									" 15:fIsSlave=%d"
									" 3:fSlvXferSz16=%d"
									" 2:fSlvXferSz8=%d"
									" 1:fSlvXferSz4=%d"
									" 0:fSlvXferSz2=%d"
									"\n",
									longNum,
									int(longNum >> fIsMaster) & 1,
									int(longNum >> fMstrLockedXfer) & 1,
									int(longNum >> fMstrXferSz16) & 1,
									int(longNum >> fMstrXferSz8) & 1,
									int(longNum >> fMstrXferSz4) & 1,
									int(longNum >> fMstrXferSz2) & 1,
									int(longNum >> fIsSlave) & 1,
									int(longNum >> fSlvXferSz16) & 1,
									int(longNum >> fSlvXferSz8) & 1,
									int(longNum >> fSlvXferSz4) & 1,
									int(longNum >> fSlvXferSz2) & 1
								);
								break;
							}
							break;
						case  sMaxLockedTransferCount:
							{
								if (!CheckDataAddr(dataAddr))
									return;
								GetBytes(dataAddr, &longNum, 4);
								BE_TO_HOST_32(longNum);
								fprintf(gOutFile, "%" PRId32 "\n", longNum);
								break;
							}
							break;
						case sVidAuxParams:
						{
							if (!CheckDataAddr(dataAddr))
								return;
							AddDirListEntry(dataAddr, curStr.c_str());
							fprintf(gOutFile, "\n");
							WriteSResourceDirectory(dataAddr, vidAuxParamsDir, curStr);
							break;
						}
						case sDebugger:
							fprintf(gOutFile, "%" PRId32 "\n", offset);
							break;

						case sVidParmDir:
						{
							if (!CheckDataAddr(dataAddr))
								return;
							AddDirListEntry(dataAddr, curStr.c_str());
							fprintf(gOutFile, "\n");
							WriteSResourceDirectory(dataAddr, vidParmDir, curStr);
							break;
						}
						case 0xB6:
						{
							if (!CheckDataAddr(dataAddr))
								return;
							WriteSBlock(dataAddr, miscData, "sPict");
							break;
						}
						default:
							fprintf(gOutFile, "\n");
							break;
					}
					break;
				case catDisplay:
					switch (sRsrcId) {
						case sVidAttributes:
							fprintf(gOutFile, "%02" PRIX32 "  0:fLCDScreen=%d 1:fBuiltInDisplay=%d 2:fDefaultColor=%d 3:fActiveBlack=%d 4:fDimMinAt1=%d\n",
								offset & 0x00ffffff,
								int(offset >> 0) & 1,
								int(offset >> 1) & 1,
								int(offset >> 2) & 1,
								int(offset >> 3) & 1,
								int(offset >> 4) & 1
							);
							break;
						default:
							fprintf(gOutFile, "\n");
							break;
					}
					break;
				default:
					fprintf(gOutFile, "\n");
					break;
			}
			break;
	} /* switch sRsrcId */
} /* DoRsrcDir */


void DoVidModeDir(uint8_t sRsrcId, int32_t offset, Ptr dataAddr, Ptr &miscData)
{
	uint32_t longNum;

	switch (sRsrcId) {
		case mVidParams:
		{
			if (!CheckDataAddr(dataAddr))
				return;

			std::string tabStr = GetTab(gLevel) + ltab1;

			int32_t miscDataSize = GetSBlock(dataAddr, miscData);
			VPBlockPtr vp = VPBlockPtr(miscData);
			BE_TO_HOST_32(vp->vpBaseOffset);
			BE_TO_HOST_16(vp->vpRowBytes);
			BE_TO_HOST_16(vp->vpBounds.top);
			BE_TO_HOST_16(vp->vpBounds.left);
			BE_TO_HOST_16(vp->vpBounds.bottom);
			BE_TO_HOST_16(vp->vpBounds.right);
			BE_TO_HOST_16(vp->vpVersion);
			BE_TO_HOST_16(vp->vpPackType);
			BE_TO_HOST_32(vp->vpPackSize);
			BE_TO_HOST_32(vp->vpHRes);
			BE_TO_HOST_32(vp->vpVRes);
			BE_TO_HOST_16(vp->vpPixelType);
			BE_TO_HOST_16(vp->vpPixelSize);
			BE_TO_HOST_16(vp->vpCmpCount);
			BE_TO_HOST_16(vp->vpCmpSize);
			BE_TO_HOST_32(vp->vpPlaneBytes);

			longNum = (uint32_t)miscDataSize + 4;
			fprintf(gOutFile, "block size = %02" PRIX8 " %06" PRIX32 "\n", uint8_t(longNum >> 24), longNum & 0x00ffffff);

			fprintf(gOutFile, "%s%20s%08" PRIX32 "\n", tabStr.c_str(), "vpBaseOffset: ", vp->vpBaseOffset);
			fprintf(gOutFile, "%s%20s%04" PRIX16 "\n", tabStr.c_str(), "vpRowBytes: ", vp->vpRowBytes);
			fprintf(gOutFile, "%s%20s%" PRId16 ",%" PRId16 ",%" PRId16 ",%" PRId16 "\n", tabStr.c_str(), "vpBounds (t,l,b,r): ", vp->vpBounds.top, vp->vpBounds.left, vp->vpBounds.bottom, vp->vpBounds.right);

			fprintf(gOutFile, "%s%20s%" PRId16 "", tabStr.c_str(), "vpVersion: ", vp->vpVersion);
			if (vp->vpVersion == 4)
				fprintf(gOutFile, " = baseAddr32\n");
			else
				fprintf(gOutFile, "\n");

			fprintf(gOutFile, "%s%20s%" PRId16 "\n", tabStr.c_str(), "vpPackType: ", vp->vpPackType);
			fprintf(gOutFile, "%s%20s%" PRId32 "\n", tabStr.c_str(), "vpPackSize: ", vp->vpPackSize);
			fprintf(gOutFile, "%s%20s%.2f\n", tabStr.c_str(), "vpHRes: ", vp->vpHRes / 65536.0);
			fprintf(gOutFile, "%s%20s%.2f\n", tabStr.c_str(), "vpVRes: ", vp->vpVRes / 65536.0);

			fprintf(gOutFile, "%s%20s%" PRId16 "", tabStr.c_str(), "vpPixelType: ", vp->vpPixelType);
			switch (vp->vpPixelType & 0x0F) {
				case  0: fprintf(gOutFile, " = chunky"); break;
				case  1: fprintf(gOutFile, " = chunkyPlanar"); break;
				case  2: fprintf(gOutFile, " = planar"); break;
				default: fprintf(gOutFile, " = unknown pixel type"); break;
			}
			if (vp->vpPixelType & (1 << 4))
				fprintf(gOutFile, ", RGBDirect\n");
			else
				fprintf(gOutFile, ", indexed\n");

			fprintf(gOutFile, "%s%20s%" PRId16 "\n", tabStr.c_str(), "vpPixelSize: ", vp->vpPixelSize);
			fprintf(gOutFile, "%s%20s%" PRId16 "\n", tabStr.c_str(), "vpCmpCount: ", vp->vpCmpCount);
			fprintf(gOutFile, "%s%20s%" PRId16 "\n", tabStr.c_str(), "vpCmpSize: ", vp->vpCmpSize);
			fprintf(gOutFile, "%s%20s%08" PRIX32 "\n", tabStr.c_str(), "vpPlaneBytes: ", vp->vpPlaneBytes);

			break;
		}
		case mTable:
		{
			if (!CheckDataAddr(dataAddr))
				return;

			int32_t miscDataSize = GetSBlock(dataAddr, miscData);
			CTabPtr ct = CTabPtr(miscData);
			BE_TO_HOST_32(ct->ctSeed);
			BE_TO_HOST_16(ct->ctFlags);
			BE_TO_HOST_16(ct->ctSize);

			longNum = (uint32_t)miscDataSize + 4;
			fprintf(gOutFile, "block size = %02" PRIX8 " %06" PRIX32 "", uint8_t(longNum >> 24), longNum & 0x00ffffff);
			fprintf(gOutFile, "  ctSeed: %08" PRIX32 "  ctFlags: %04" PRIX16 "  ctSize: %" PRId16 "\n", ct->ctSeed, ct->ctFlags, ct->ctSize);
			break;
		}
		case mPageCnt:
			fprintf(gOutFile,"%" PRId32 "\n", offset);
			break;
		case mDevType:
		{
			fprintf(gOutFile,"%" PRId32 "", offset);
			switch (offset) {
				case  0: fprintf(gOutFile, " = indexed CLUT device\n"); break;
				case  1: fprintf(gOutFile, " = indexed fixed device\n"); break;
				case  2: fprintf(gOutFile, " = direct device\n"); break;
				default: fprintf(gOutFile, " = unknown device\n"); break;
			}
			break;
		}
		case endOfList:
			fprintf(gOutFile, "\n");
			break;

	} /* switch sRsrcId */
} /* vidModeDir */


void DoDrvrDir(uint8_t sRsrcId, Ptr dataAddr)
{
	DriverHeaderRec dh;

	if (sRsrcId != endOfList)
	{
		if (!CheckDataAddr(dataAddr))
			return;
		std::string tabStr = GetTab(gLevel) + ltab1;

		GetBytes(dataAddr, &dh, sizeof(DriverHeaderRec));
		BE_TO_HOST_32(dh.blockSize);
		BE_TO_HOST_16(dh.drvrFlags);
		BE_TO_HOST_16(dh.drvrDelay);
		BE_TO_HOST_16(dh.drvrEMask);
		BE_TO_HOST_16(dh.drvrMenu);
		BE_TO_HOST_16(dh.drvrOpen);
		BE_TO_HOST_16(dh.drvrPrime);
		BE_TO_HOST_16(dh.drvrCtl);
		BE_TO_HOST_16(dh.drvrStatus);
		BE_TO_HOST_16(dh.drvrClose);

		fprintf(gOutFile, "block size = %02" PRIX8 " %06" PRIX32 "\n", uint8_t(dh.blockSize >> 24), dh.blockSize & 0x00ffffff);
		SetCovered(dataAddr, int32_t(CalcAddr(dataAddr,int32_t(dh.blockSize))-CalcAddr(dataAddr,0)));

		fprintf(gOutFile, "%s%20s%04" PRIX16 "  8:dReadEnable=%d 9:dWriteEnable=%d 10:dCtlEnable=%d 11:dStatEnable=%d 12:dNeedGoodBye=%d 13:dNeedTime=%d 14:dNeedLock=%d",
			tabStr.c_str(), "drvrFlags: ", dh.drvrFlags,
			int(dh.drvrFlags >> 8) & 1,
			int(dh.drvrFlags >> 9) & 1,
			int(dh.drvrFlags >> 10) & 1,
			int(dh.drvrFlags >> 11) & 1,
			int(dh.drvrFlags >> 12) & 1,
			int(dh.drvrFlags >> 13) & 1,
			int(dh.drvrFlags >> 14) & 1
		);
		fprintf(gOutFile, "\n");

		fprintf(gOutFile, "%s%20s%" PRId16 "\n", tabStr.c_str(), "drvrDelay: ", dh.drvrDelay);
		fprintf(gOutFile, "%s%20s%04" PRIX16 " ", tabStr.c_str(), "drvrEMask: ", dh.drvrEMask);
		if (dh.drvrEMask & (1 << 1))
			fprintf(gOutFile, " 1:mouseDown");
		if (dh.drvrEMask & (1 << 2))
			fprintf(gOutFile, " 2:mouseUp");
		if (dh.drvrEMask & (1 << 3))
			fprintf(gOutFile, " 3:keyDown");
		if (dh.drvrEMask & (1 << 4))
			fprintf(gOutFile, " 4:keyUp");
		if (dh.drvrEMask & (1 << 5))
			fprintf(gOutFile, " 5:autoKey");
		if (dh.drvrEMask & (1 << 6))
			fprintf(gOutFile, " 6:update");
		if (dh.drvrEMask & (1 << 7))
			fprintf(gOutFile, " 7:disk");
		if (dh.drvrEMask & (1 << 8))
			fprintf(gOutFile, " 8:activate");
		if (dh.drvrEMask & (1 << 10))
			fprintf(gOutFile, " 10:network");
		if (dh.drvrEMask & (1 << 11))
			fprintf(gOutFile, " 11:driver");
		if (dh.drvrEMask & (1 << 12))
			fprintf(gOutFile, " 12:app1");
		if (dh.drvrEMask & (1 << 13))
			fprintf(gOutFile, " 13:app2");
		if (dh.drvrEMask & (1 << 14))
			fprintf(gOutFile, " 14:app3");
		if (dh.drvrEMask & (1 << 15))
			fprintf(gOutFile, " 15:app4");
		fprintf(gOutFile, "\n");

		fprintf(gOutFile, "%s%20s%" PRId16 "\n", tabStr.c_str(), "drvrMenu: ", dh.drvrMenu);
		fprintf(gOutFile, "%s%20s%04" PRIX16 "", tabStr.c_str(), "drvrOpen: ", dh.drvrOpen);
		if (dh.drvrOpen != 0)
			fprintf(gOutFile, " = %08" PRIXPTR "", OutAddr(CalcAddr(dataAddr, 4 + dh.drvrOpen)));
		fprintf(gOutFile, "\n");

		fprintf(gOutFile, "%s%20s%04" PRIX16 "", tabStr.c_str(), "drvrPrime: ", dh.drvrPrime);
		if (dh.drvrPrime != 0)
			fprintf(gOutFile, " = %08" PRIXPTR "", OutAddr(CalcAddr(dataAddr, 4 + dh.drvrPrime)));
		fprintf(gOutFile, "\n");

		fprintf(gOutFile, "%s%20s%04" PRIX16 "", tabStr.c_str(), "drvrCtl: ", dh.drvrCtl);
		if (dh.drvrCtl != 0)
			fprintf(gOutFile, " = %08" PRIXPTR "", OutAddr(CalcAddr(dataAddr, 4 + dh.drvrCtl)));
		fprintf(gOutFile, "\n");

		fprintf(gOutFile, "%s%20s%04" PRIX16 "", tabStr.c_str(), "drvrStatus: ", dh.drvrStatus);
		if (dh.drvrStatus != 0)
			fprintf(gOutFile, " = %08" PRIXPTR "", OutAddr(CalcAddr(dataAddr, 4 + dh.drvrStatus)));
		fprintf(gOutFile, "\n");

		fprintf(gOutFile, "%s%20s%04" PRIX16 "", tabStr.c_str(), "drvrClose: ", dh.drvrClose);
		if (dh.drvrClose != 0)
			fprintf(gOutFile, " = %08" PRIXPTR "", OutAddr(CalcAddr(dataAddr, 4 + dh.drvrClose)));
		fprintf(gOutFile, "\n");

		fprintf(gOutFile, "%s%20s%s%.*s%s\n", tabStr.c_str(), "drvrName: ", kOpenQuote, (int)dh.drvrName[0], dh.drvrName + 1, kCloseQuote);
	} else
		fprintf(gOutFile, "\n");
} /* drvrDir */


void DoGammaDirOrVidNamesDir(uint8_t sRsrcId, DirType whichDir, Ptr dataAddr, Ptr &miscData)
{
	uint32_t longNum;

	if (sRsrcId != endOfList)
	{
		if (!CheckDataAddr(dataAddr))
			return;
		std::string tabStr = GetTab(gLevel) + ltab1;
		int32_t miscDataSize = GetSBlock(dataAddr, miscData);
		int16_t ID = *(int16_t*)(miscData);
		BE_TO_HOST_16(ID);

		longNum = (uint32_t)miscDataSize + 4;
		fprintf(gOutFile, "block size = %02" PRIX8 " %06" PRIX32 "  ID: %04" PRIX16 "  name: %s", uint8_t(longNum >> 24), longNum & 0x00ffffff, ID, kOpenQuote);
		WriteCString((char*)miscData + 2);
		fprintf(gOutFile, "%s\n", kCloseQuote);
		if (whichDir == gammaDir)
		{
			longNum = uint32_t((strlen((char*)miscData + 2) + 1) & ~1);
			GammaTblPtr p = GammaTblPtr(miscData + 2 + longNum);
			BE_TO_HOST_16(p->gVersion);
			BE_TO_HOST_16(p->gType);
			BE_TO_HOST_16(p->gFormulaSize);
			BE_TO_HOST_16(p->gChanCnt);
			BE_TO_HOST_16(p->gDataCnt);
			BE_TO_HOST_16(p->gDataWidth);

			fprintf(gOutFile, "%s%20s%" PRId16 "\n", tabStr.c_str(), "gVersion: ", p->gVersion);
			fprintf(gOutFile, "%s%20s%" PRId16 "\n", tabStr.c_str(), "gType: ", p->gType);
			fprintf(gOutFile, "%s%20s%" PRId16 "\n", tabStr.c_str(), "gFormulaSize: ", p->gFormulaSize);
			fprintf(gOutFile, "%s%20s%" PRId16 "\n", tabStr.c_str(), "gChanCnt: ", p->gChanCnt);
			fprintf(gOutFile, "%s%20s%" PRId16 "\n", tabStr.c_str(), "gDataCnt: ", p->gDataCnt);
			fprintf(gOutFile, "%s%20s%" PRId16 "\n", tabStr.c_str(), "gDataWidth: ", p->gDataWidth);
			fprintf(gOutFile, "%s%20s%" PRId32 "\n", tabStr.c_str(), "gFormulaData len: ", int32_t(miscDataSize - sizeof(GammaTbl) - longNum));
		}
	}
	else
		fprintf(gOutFile, "\n");
} /* gammaDir, vidNamesDir */


void DoVidParmDir(uint8_t sRsrcId, Ptr dataAddr, Ptr &miscData)
{
	if (sRsrcId != endOfList)
	{
		if (!CheckDataAddr(dataAddr))
			return;
		WriteSBlock(dataAddr, miscData, "sVidParms");
	}
	else
		fprintf(gOutFile, "\n");
} /* vidParmDir */


void DoVidAuxParamsDir(uint8_t sRsrcId, Ptr dataAddr)
{
	uint32_t longNum;

	if (sRsrcId != endOfList)
	{
		if (!CheckDataAddr(dataAddr))
			return;
		GetBytes(dataAddr, &longNum, 4);
		BE_TO_HOST_32(longNum);
		fprintf(gOutFile, "%" PRId32, longNum);
		const char * what;

		if (longNum == timingInvalid_SM_T24) {
			GetBytes(CalcAddr(dataAddr, 4), &longNum, 4);
			BE_TO_HOST_32(longNum);
			fprintf(gOutFile, " = timingInvalid_SM_T24 = block size  %" PRId32, longNum);
		}

		switch (longNum)
		{
			case timingInvalid             : what = "timingInvalid"; break;
			case timingInvalid_SM_T24      : what = "timingInvalid_SM_T24"; break;
			case timingApple_FixedRateLCD  : what = "timingApple_FixedRateLCD"; break;
			case timingVESA_640x480_72hz   : what = "timingVESA_640x480_72hz"; break;
			case timingVESA_640x480_75hz   : what = "timingVESA_640x480_75hz"; break;
			case timingVESA_640x480_85hz   : what = "timingVESA_640x480_85hz"; break;
			case timingGTF_640x480_120hz   : what = "timingGTF_640x480_120hz"; break;
			case timingVESA_800x600_60hz   : what = "timingVESA_800x600_60hz"; break;
			case timingVESA_800x600_72hz   : what = "timingVESA_800x600_72hz"; break;
			case timingVESA_800x600_75hz   : what = "timingVESA_800x600_75hz"; break;
			case timingVESA_800x600_85hz   : what = "timingVESA_800x600_85hz"; break;
			case timingVESA_1024x768_75hz  : what = "timingVESA_1024x768_75hz"; break;
			case timingVESA_1024x768_85hz  : what = "timingVESA_1024x768_85hz"; break;
			case timingApple_512x384_60hz  : what = "timingApple_512x384_60hz  ; timingApple12"; break;
			case timingApple_560x384_60hz  : what = "timingApple_560x384_60hz  ; timingApple12x"; break;
			case timingApple_640x480_67hz  : what = "timingApple_640x480_67hz  ; timingApple13"; break;
			case timingApple_640x400_67hz  : what = "timingApple_640x400_67hz  ; timingApple13x"; break;
			case timingVESA_640x480_60hz   : what = "timingVESA_640x480_60hz   ; timingAppleVGA"; break;
			case timingApple_640x870_75hz  : what = "timingApple_640x870_75hz  ; timingApple15"; break;
			case timingApple_640x818_75hz  : what = "timingApple_640x818_75hz  ; timingApple15x"; break;
			case timingApple_832x624_75hz  : what = "timingApple_832x624_75hz  ; timingApple16"; break;
			case timingVESA_800x600_56hz   : what = "timingVESA_800x600_56hz   ; timingAppleSVGA"; break;
			case timingVESA_1024x768_60hz  : what = "timingVESA_1024x768_60hz  ; timingApple1Ka"; break;
			case timingVESA_1024x768_70hz  : what = "timingVESA_1024x768_70hz  ; timingApple1Kb"; break;
			case timingApple_1024x768_75hz : what = "timingApple_1024x768_75hz ; timingApple19"; break;
			case timingApple_1152x870_75hz : what = "timingApple_1152x870_75hz ; timingApple21"; break;
			case timingAppleNTSC_ST        : what = "timingAppleNTSC_ST"; break;
			case timingAppleNTSC_FF        : what = "timingAppleNTSC_FF"; break;
			case timingAppleNTSC_STconv    : what = "timingAppleNTSC_STconv"; break;
			case timingAppleNTSC_FFconv    : what = "timingAppleNTSC_FFconv"; break;
			case timingApplePAL_ST         : what = "timingApplePAL_ST"; break;
			case timingApplePAL_FF         : what = "timingApplePAL_FF"; break;
			case timingApplePAL_STconv     : what = "timingApplePAL_STconv"; break;
			case timingApplePAL_FFconv     : what = "timingApplePAL_FFconv"; break;
			case timingVESA_1280x960_75hz  : what = "timingVESA_1280x960_75hz"; break;
			case timingVESA_1280x960_60hz  : what = "timingVESA_1280x960_60hz"; break;
			case timingVESA_1280x960_85hz  : what = "timingVESA_1280x960_85hz"; break;
			case timingVESA_1280x1024_60hz : what = "timingVESA_1280x1024_60hz"; break;
			case timingVESA_1280x1024_75hz : what = "timingVESA_1280x1024_75hz"; break;
			case timingVESA_1280x1024_85hz : what = "timingVESA_1280x1024_85hz"; break;
			case timingVESA_1600x1200_60hz : what = "timingVESA_1600x1200_60hz"; break;
			case timingVESA_1600x1200_65hz : what = "timingVESA_1600x1200_65hz"; break;
			case timingVESA_1600x1200_70hz : what = "timingVESA_1600x1200_70hz"; break;
			case timingVESA_1600x1200_75hz : what = "timingVESA_1600x1200_75hz"; break;
			case timingVESA_1600x1200_80hz : what = "timingVESA_1600x1200_80hz"; break;
			case timingVESA_1600x1200_85hz : what = "timingVESA_1600x1200_85hz"; break;
			case timingSMPTE240M_60hz      : what = "timingSMPTE240M_60hz"; break;
			case timingFilmRate_48hz       : what = "timingFilmRate_48hz"; break;
			case timingSony_1600x1024_76hz : what = "timingSony_1600x1024_76hz"; break;
			case timingSony_1920x1080_60hz : what = "timingSony_1920x1080_60hz"; break;
			case timingSony_1920x1080_72hz : what = "timingSony_1920x1080_72hz"; break;
			case timingSony_1900x1200_74hz : what = "timingSony_1900x1200_74hz"; break;
			case timingSony_1900x1200_76hz : what = "timingSony_1900x1200_76hz"; break;
			default                        : what = "timingUnknown"; break;
		}
		fprintf(gOutFile, " = %s\n", what);
	}
	else
		fprintf(gOutFile, "\n");
} /* vidAuxParamsDir */


void WriteData (
	uint8_t sRsrcId,
	int32_t offset,
	Ptr dataAddr,
	const std::string curStr,
	DirType whichDir
)
{
	Ptr miscData = NULL;

	if (!FindDirEntry(dataAddr))
	{
		AddAddrEntry(dataAddr);
		if (sRsrcId != endOfList)
			switch (whichDir) {
				case rootDir:
				case superDir:
				case rsrcUnknownDir:
				case rsrcDirDir:
				case gammaDir:
				case loadDir:
				case drvrDir:
					AddDirListEntry(dataAddr, curStr.c_str());
					break;
				default: ;
			}

		switch (whichDir) {
			case rsrcDirDir:
				DoRsrcDirDir(sRsrcId, whichDir, dataAddr, curStr);
				break;

			case rsrcDir:
				DoRsrcDir(sRsrcId, offset, dataAddr, miscData, curStr);
				break;

			case rootDir:
				switch (sRsrcId) {
					case appleFormat:
						WriteSExecBlock(GetTab(gLevel), dataAddr);
						break;
					case sSuperDir:
						if (!CheckDataAddr(dataAddr))
							return;
						AddDirListEntry(dataAddr, curStr.c_str());
						fprintf(gOutFile, "\n");
						WriteSResourceDirectory(dataAddr, superDir, curStr);
						break;
					default:
						if (!CheckDataAddr(dataAddr))
							return;
						AddDirListEntry(dataAddr, curStr.c_str());
						fprintf(gOutFile, "\n");
						WriteSResourceDirectory(dataAddr, rsrcDirDir, curStr);
						break;
				}
				break;

			case superDir:
				switch (sRsrcId) {
					case 1:
						if (!CheckDataAddr(dataAddr))
							return;
						AddDirListEntry(dataAddr, curStr.c_str());
						fprintf(gOutFile, "\n");
						WriteSResourceDirectory(dataAddr, rsrcUnknownDir, curStr);
						break;
					default:
						if (!CheckDataAddr(dataAddr))
							return;
						AddDirListEntry(dataAddr, curStr.c_str());
						fprintf(gOutFile, "\n");
						WriteSResourceDirectory(dataAddr, rsrcDirDir, curStr);
						break;
				}
				break;

			case rsrcUnknownDir:
				if (!CheckDataAddr(dataAddr))
					return;
				AddDirListEntry(dataAddr, curStr.c_str());
				fprintf(gOutFile, "\n");
				WriteSResourceDirectory(dataAddr, rsrcDir, curStr);
				break;

			case vidModeDir:
				DoVidModeDir(sRsrcId, offset, dataAddr, miscData);
				break;

			case drvrDir:
				DoDrvrDir(sRsrcId, dataAddr);
				break;

			case loadDir:
				if (sRsrcId != endOfList)
					WriteSExecBlock(GetTab(gLevel), dataAddr);
				else
					fprintf(gOutFile, "\n");
				break;

			case vendorDir:
				if (sRsrcId != endOfList)
					GetAndWriteCString(dataAddr, miscData);
				else
					fprintf(gOutFile, "\n");
				break;

			case gammaDir:
			case vidNamesDir:
				DoGammaDirOrVidNamesDir(sRsrcId, whichDir, dataAddr, miscData);
				break;

			case vidParmDir:
				DoVidParmDir(sRsrcId, dataAddr, miscData);
				break;

			case vidAuxParamsDir:
				DoVidAuxParamsDir(sRsrcId, dataAddr);
				break;

		} /* switch whichDir */

		if (miscData != NULL) {
			free(miscData);
			miscData = NULL;
		}
	} /* if !findDir */
} /* WriteData */


void WriteSResourceDirectory(Ptr dirPtr, DirType whichDir, const std::string curStr)
{
	uint8_t sRsrcId;
	int32_t offset;
	int16_t i;
	std::string s;
	Ptr resultAddr;

	gLevel++;
	if (gLevel > 5)
	{
		fprintf(gOutFile, "too many levels\n");
		gLevel = gLevel - 1;
		return;
	}

	s = GetTab(gLevel);

	i = 0;
	if (gLevel == 0)
	{
		fprintf(gOutFile, "%08" PRIXPTR "\n", OutAddr(dirPtr));
/*
		fprintf(gOutFile, "        sResource Directory                |            sResource Fields        \n");
		fprintf(gOutFile, " #  sRsrcId  offset  result   note         |  #     id     offset  result   note\n");
*/
	}
	do {
#if 0
		if (TestKey(0x3B))
			Debugger();
		do { } while (TestKey(0x39));
#endif
		GetRsrc(dirPtr, sRsrcId, offset);
		resultAddr = CalcAddr(dirPtr, offset);

		char addrStr[10];
		bool hasAddr = GetAddrResult(sRsrcId, whichDir, resultAddr, addrStr);

		fprintf(gOutFile, "%s%3d    %02" PRIX8 "    %06" PRIX32 " %s", s.c_str(), i, sRsrcId, offset & 0x00ffffff, addrStr);
		WriteRsrcKind(sRsrcId, whichDir);
		if (
			sRsrcId == endOfList
#if 0
			|| TestKey(0x3A)
#endif
		)
			fprintf(gOutFile, "\n");
		else {
			char buf[10];
			snprintf(buf, sizeof(buf), "%02" PRIX8 "", sRsrcId);
			WriteData(sRsrcId, offset, hasAddr ? resultAddr : NULL, curStr + "id:" + buf + " ", whichDir);
		}

		dirPtr = CalcAddr(dirPtr,4);
		i++;
	} while (sRsrcId != endOfList);
	gLevel = gLevel - 1;
} /* WriteSResourceDirectory */



void InitLoopingGlobals()
{
	gMinAddr = NULL;
	gMaxAddr = NULL;
	gDirListSize = 0;
	gAddrListSize = 0;
}

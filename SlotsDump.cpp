#include "SlotsCommon.h"


#if 0
const uint8_t* KeyMapAddr = (uint8_t*)0x174;
#define TestKey(x) ((KeyMapAddr[(x)>>3] >> ((x)&7)) & 1)
#endif

const int defaultWidth = 64;
const int defaultTop = 10;
const int defaultLeft = 10;
const int textLineHeight = 12;

WindowPtr gWind = NULL;

int16_t gCurWidth, gCurLeft, gCurTop;


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
		fprintf(gOutFile, "Error occured.%s in %s\n", ErrS(err).c_str(), msg);
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
	const int32_t bufSize = 1024;
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
	
	gDoCovering = false;
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

	gDoCovering = true;
}


void WriteIcon(Ptr iconp, const char * title)
{
    Handle icon = NewHandle(128);
    HLock(icon);
    memcpy(*icon, iconp, 128);
    HUnlock(icon);
    
	Rect r;
	int16_t width;

	SetRect(&r, 0, 0, 32, 32);
	if (gCurTop > gWind->portRect.bottom - 32 - textLineHeight)
	{
		gCurTop = defaultTop;
		gCurLeft = gCurLeft + gCurWidth;
		gCurWidth = defaultWidth;
	}
	OffsetRect(&r, gCurLeft, gCurTop);
	PlotIcon(&r, icon);
    DisposeHandle(icon);
	MoveTo(gCurLeft, gCurTop + 32 + textLineHeight);

	Str255 titleStr;
	strncpy((char*)(&titleStr[1]), title, 255);
	titleStr[0] = (unsigned char)strlen(title);
	DrawString(titleStr);

	width = StringWidth(titleStr) + 10;
	if (gCurWidth < width)
		gCurWidth = width;
	gCurTop = gCurTop + 32 + textLineHeight + 4;
} /* WriteIcon */


void WriteIcl(void * icon, int16_t depth, const char * title)
{
	PixMap pix;
	Rect r;
	int16_t width;

	pix = **(CGrafPtr(gWind)->portPixMap);

	pix.baseAddr = Ptr(icon);
	pix.pixelType = 0;
	pix.cmpCount = 1;
	SetRect(&pix.bounds, 0, 0, 32, 32);
	if (depth == 4)
	{
		pix.pixelSize = 4;
		pix.cmpSize = 4;
		pix.pmTable = GetCTable(4);
	}
	else if (depth == 8)
	{
		pix.pixelSize = 8;
		pix.cmpSize = 8;
		pix.pmTable = GetCTable(8);
	}

	SetRect(&r, 0, 0, 32, 32);
	if (gCurTop > gWind->portRect.bottom - 32 - textLineHeight)
	{
		gCurTop = defaultTop;
		gCurLeft = gCurLeft + gCurWidth;
		gCurWidth = defaultWidth;
	}
	OffsetRect(&r, gCurLeft, gCurTop);
	CopyBits(BitMapPtr(&pix), &gWind->portBits, &pix.bounds, &r, srcCopy, NULL);
	MoveTo(gCurLeft, gCurTop + 32 + textLineHeight);

	Str255 titleStr;
	strncpy((char*)(&titleStr[1]), title, 255);
	titleStr[0] = (unsigned char)strlen(title);
	DrawString(titleStr);

	width = StringWidth(titleStr) + 10;
	if (gCurWidth < width)
		gCurWidth = width;
	gCurTop = gCurTop + 32 + textLineHeight + 4;
} /* WriteIcl */


void WriteCIcon(Ptr miscData, const char * title)
{
	int32_t iconSize = GetSBlock(miscData, miscData);
	Handle icon = NewHandle(iconSize);
	if (!icon)
		return;
	
	Rect    r;
	int16_t width;

	int16_t iconHeight;
	int16_t iconWidth;

	int32_t bmapSize;
	int32_t maskSize;
	int32_t colorSize;
	int32_t pixSize;

	Handle  pixData;
	CTabHandle  colors;

	Ptr maskPtr;
	Ptr bmapPtr;
	CTabPtr colorPtr;
	Ptr pixPtr;

	HLock(icon);
	memcpy(*icon, miscData, (size_t)iconSize);

	CIconPtr iconP = *CIconHandle(icon);

	maskSize = iconP->iconMask.rowBytes * (iconP->iconMask.bounds.bottom - iconP->iconMask.bounds.top);
	maskPtr = Ptr(iconP) + sizeof(PixMap) + 2 * sizeof(BitMap) + 4;
	iconP->iconMask.baseAddr = maskPtr;

	bmapSize = iconP->iconBMap.rowBytes * (iconP->iconBMap.bounds.bottom - iconP->iconBMap.bounds.top);
	bmapPtr = maskPtr + maskSize;
	iconP->iconBMap.baseAddr = bmapPtr;

	colorPtr = CTabPtr(bmapPtr + bmapSize);
	colorSize = 8 + 8 * (colorPtr->ctSize + 1);
	colors = CTabHandle(NewHandle(colorSize));
	BlockMove(colorPtr, *colors, colorSize);

	pixPtr = Ptr(colorPtr) + colorSize;

	iconWidth = iconP->iconPMap.bounds.right - iconP->iconPMap.bounds.left;
	iconHeight = iconP->iconPMap.bounds.bottom - iconP->iconPMap.bounds.top;
	pixSize = (iconP->iconPMap.rowBytes & 0x7FFF) * iconHeight;
	pixData = NewHandle(pixSize);
	BlockMove(pixPtr, *pixData, pixSize);
	iconP->iconPMap.baseAddr = *pixData;
	iconP->iconPMap.pmTable = colors;

	iconP->iconData = pixData;

	HUnlock(icon);

	SetRect(&r, 0, 0, iconWidth, iconHeight);
	if (gCurTop > gWind->portRect.bottom - iconHeight - textLineHeight)
	{
		gCurTop = defaultTop;
		gCurLeft = gCurLeft + gCurWidth;
		gCurWidth = defaultWidth;
	}
	OffsetRect(&r, gCurLeft, gCurTop);
	PlotCIcon(&r, CIconHandle(icon));
	DisposeHandle(icon);

	DisposeHandle(pixData);
	DisposeHandle(Handle(colors));
	MoveTo(gCurLeft, gCurTop + 32 + textLineHeight);

	Str255 titleStr;
	strncpy((char*)(&titleStr[1]), title, 255);
	titleStr[0] = (unsigned char)strlen(title);
	DrawString(titleStr);

	width = StringWidth(titleStr) + 10;
	iconWidth = iconWidth + 10;
	if (width < iconWidth)
		width = iconWidth;
	if (gCurWidth < width)
		gCurWidth = width;
	gCurTop = gCurTop + iconHeight + textLineHeight + 4;
} /* WriteCIcon */


void WriteSInfoRecord(SInfoRecord &si, Ptr &sResDirFromInfo)
{
	if ((si.siInitStatusA == 0) || (kIgnoreError && (si.siCPUByteLanes != 0)))
	{
		fprintf(gOutFile, "%20s%08" PRIXPTR "\n", "siDirPtr: ", OutAddr(si.siDirPtr));

		sResDirFromInfo = si.siDirPtr;

		fprintf(gOutFile, "%20s%s\n", "siInitStatusA: ", ErrS(si.siInitStatusA).c_str());
		fprintf(gOutFile, "%20s%" PRId16 "\n", "siInitStatusV: ", si.siInitStatusV);

		fprintf(gOutFile, "%20s%" PRId8 "", "siState: ", si.siState);
		switch (si.siState) {
			case 0 : fprintf(gOutFile, " = stateNil\n"); break;
			case 1 : fprintf(gOutFile, " = stateSDMInit\n"); break;
			case 2 : fprintf(gOutFile, " = statePRAMInit\n"); break;
			case 3 : fprintf(gOutFile, " = statePInit\n"); break;
			case 4 : fprintf(gOutFile, " = stateSInit\n"); break;
			default: fprintf(gOutFile, "\n"); break;
		}

		fprintf(gOutFile, "%20s%02" PRIX8 " ", "siCPUByteLanes: ", (uint8_t)si.siCPUByteLanes);
		WriteByteLanes(si.siCPUByteLanes);

		fprintf(gOutFile, "%20s%02" PRIX8 "\n", "siTopOfROM: ", (uint8_t)si.siTopOfROM);
		fprintf(gOutFile, "%20s%02" PRIX8 "\n", "siStatusFlags: ", (uint8_t)si.siStatusFlags);

		fprintf(gOutFile, "%20s%" PRId16 "", "siTOConst: ", si.siTOConst);
		if (si.siTOConst == defaultTO)
			fprintf(gOutFile, " = defaultTO\n");
		else
			fprintf(gOutFile, "\n");

		fprintf(gOutFile, "%20s%02" PRIX8 "%02" PRIX8 "\n", "siReserved: ", (uint8_t)si.siReserved[0], (uint8_t)si.siReserved[1]);

		gByteLanes = int8_t(si.siCPUByteLanes & 0x0F);
		gTopOfRom = CalcAddr(si.siROMAddr, 1);
		fprintf(gOutFile, "%20s%08" PRIXPTR " ; Byte after ROM: %08" PRIXPTR "\n", "siROMAddr: ", OutAddr(si.siROMAddr), OutAddr(gTopOfRom));

		fprintf(gOutFile, "%20s%02" PRIX8 "\n", "siSlot: ", (uint8_t)si.siSlot);

		fprintf(gOutFile, "%20s%02" PRIX8 "%02" PRIX8 "%02" PRIX8 "\n", "siPadding: ", (uint8_t)si.siPadding[0], (uint8_t)si.siPadding[1], (uint8_t)si.siPadding[2]);
	}
	else
		fprintf(gOutFile, "%20s%s\n", "siInitStatusA: ", ErrS(si.siInitStatusA).c_str());
} /* WriteSInfoRecord */


void WriteSInfoRecordAndFHeaderRec(uint8_t slot, Ptr &sResDirFromInfo, Ptr &sResDirFromHead, Ptr &sRootDirFromXHead, bool printDivider)
{
	FHeaderRec fh;
	XFHeaderRec xfh;

	sResDirFromInfo = NULL;
	sResDirFromHead = NULL;
	sRootDirFromXHead = NULL;
	if (gCoveredPtr) {
		free(gCoveredPtr);
		gCoveredPtr = NULL;
		numToCover = 0;
	}

	memset(&fh, 0, sizeof(fh));
	memset(&xfh, 0, sizeof(xfh));

	SInfoRecPtr theSInfoRecPtr = NULL;

	SpBlock sp;
	memset(&sp, 0, sizeof(sp));
	sp.spSlot = (int8_t)slot;
	sp.spResult = int32_t(&fh);

	OSErr headErr = SReadFHeader(&sp);
	OSErr infoErr = SFindSInfoRecPtr(&sp);

	if (printDivider && (slot < 0x10 || headErr != smSlotOOBErr || infoErr != smSlotOOBErr))
	{
		fprintf(gOutFile, "\n");
		fprintf(gOutFile, "-----------------------------------------\n");
		fprintf(gOutFile, "slot %02" PRIX8 ":\n", slot);
	}

	if (slot < 0x10 || headErr != smSlotOOBErr)
		DoErr("SReadFHeader", headErr);

	if (slot < 0x10 || infoErr != smSlotOOBErr)
		DoErr("sFindInfoRecPtr", infoErr);

	if (infoErr == noErr)
	{
		theSInfoRecPtr = SInfoRecPtr(sp.spResult);
		fprintf(gOutFile, "\n");
		fprintf(gOutFile, "SInfoRecord:\n");
		WriteSInfoRecord(*theSInfoRecPtr, sResDirFromInfo);
	}

	if ((headErr == noErr) || (kIgnoreError && (fh.fhLength > 0)))
	{
		Ptr headerWhere = CalcAddr(gTopOfRom, -sizeof(fh));
		Ptr xHeaderWhere = CalcAddr(gTopOfRom, -sizeof(xfh));
		GetBytes(xHeaderWhere, &xfh, sizeof(xfh));

		gStartOfRom = CalcAddr(gTopOfRom, -fh.fhLength);
		numToCover = int32_t(gTopOfRom - gStartOfRom + 8) & ~7;
		gCoveredPtr = (Ptr)malloc(size_t(numToCover / 8));
		memset(gCoveredPtr, 0, size_t(numToCover / 8));

		if (xfh.fhXSTstPat == testPattern) {
			WriteXFHeaderRec(xfh, sRootDirFromXHead, xHeaderWhere);

			if (memcmp(&xfh.fhXDirOffset, &fh, sizeof(fh))) {
				// this should never happen
				fprintf(gOutFile, "\n");
				fprintf(gOutFile, "FHeaderRec from XFHeaderRec:\n");
				Ptr sResDirFromXHead;
				WriteFHeaderRec(*(FHeaderRecPtr)&xfh.fhXDirOffset, sResDirFromXHead, headerWhere);
			}
		}

		fprintf(gOutFile, "\n");
		fprintf(gOutFile, "FHeaderRec:\n");
		WriteFHeaderRec(fh, sResDirFromHead, headerWhere);
		if (headErr == noErr) {
			SetCovered(headerWhere, int32_t(gTopOfRom-headerWhere));
			DumpROM(slot, gStartOfRom, fh.fhLength);
		} else
			sResDirFromHead = NULL;
	}

	gLevel = -1;
} /* WriteSInfoRecordAndFHeaderRec */


void SlotsDumpMain()
{
	uint8_t slot;
	Ptr sResDirFromInfo;
	Ptr sResDirFromHead;
	Ptr sRootDirFromXHead;

	WriteTable();
	slot = 0;
	do {
		InitLoopingGlobals();

		gDoCovering = true;
		memset(&gSRsrcTypeData, -1, sizeof(gSRsrcTypeData));

		WriteSInfoRecordAndFHeaderRec(slot, sResDirFromInfo, sResDirFromHead, sRootDirFromXHead, true);

		if (sResDirFromHead || sResDirFromInfo || sRootDirFromXHead)
		{
			char buf[10];
			snprintf(buf, sizeof(buf), "%02" PRIX8 "", slot);

			if (sRootDirFromXHead)
			{
				fprintf(gOutFile, "\n");
				fprintf(gOutFile, "Root Directory from Extended Header:\n");
				WriteSResourceDirectory(sRootDirFromXHead, rootDir, std::string("(root dir) Slot:") + buf + " ");
				CheckAddrRange(sRootDirFromXHead);
			}

			if (sResDirFromInfo)
			{
				fprintf(gOutFile, "\n");
				fprintf(gOutFile, "Slot Resource Directory from Info:\n");
				WriteSResourceDirectory(sResDirFromInfo, rsrcDirDir, std::string("Slot:") + buf + " ");
			}

			if (sResDirFromHead)
			{
				fprintf(gOutFile, "\n");
				fprintf(gOutFile, "Slot Resource Directory from Header:\n");
				WriteSResourceDirectory(sResDirFromHead, rsrcDirDir, std::string("(from header) Slot:") + buf + " ");
			}

			fprintf(gOutFile, "\n");

			CheckAddrRange(sResDirFromInfo);
			CheckAddrRange(sResDirFromHead);

			fprintf(gOutFile, "minAddr = %08" PRIXPTR "   maxAddr = %08" PRIXPTR "\n", OutAddr(gMinAddr), OutAddr(gMaxAddr));
			fprintf(gOutFile, "\n");
			WriteUncoveredBytes();
		} /* if */
		slot++;
	} while (slot != 0);
	gDoCovering = false;
} /* SlotsDumpMain */


void SlotsDumpSecond()
{
	SpBlock sp;
	OSErr err;
	Ptr sResDirFromInfo;
	Ptr sResDirFromHead;
	Ptr sRootDirFromXHead;
	int8_t curSlot;

	memset(&sp, 0, sizeof(sp));
	curSlot = -1;

	/*set required values in parameter block*/
	sp.spParamData = (1 << fall) + (1 << fnext); /*fAll flag = 1: search all sResources*/
	sp.spSlot      = 0;     /*start search from slot 1*/
	sp.spID        = 0;     /*start search from sResource ID 1*/
	sp.spExtDev    = 0;     /*external device ID (card-specific)*/

	err = noErr;

	while (err == noErr)    /*loop to search sResources*/
	{
		InitLoopingGlobals();

		fprintf(gOutFile, "\n");
		fprintf(gOutFile, "-----------------------------------------\n");
		err = SGetSRsrc(&sp);
		DoErr("SGetSRsrc", err);

		if (err == noErr)
		{
			gSRsrcTypeData.category = sp.spCategory;
			gSRsrcTypeData.cType = sp.spCType;
			gSRsrcTypeData.DrSW = sp.spDrvrSW;
			gSRsrcTypeData.DrHW = sp.spDrvrHW;

			fprintf(gOutFile, "SpBlock: \n");
			fprintf(gOutFile, "%20s%08" PRIX32 "\n", "spsPointer: ", uint32_t(sp.spsPointer));
			fprintf(gOutFile, "%20s%" PRId32 "%s\n", "spParamData: ", sp.spParamData, GetspParamDataEnabledDisabledString(sp.spParamData));
			fprintf(gOutFile, "%20s%" PRId16 "\n", "spRefNum: ", sp.spRefNum);
			fprintf(gOutFile, "%20s%s\n", "spCategory: ", CategoryString(sp.spCategory).c_str());
			fprintf(gOutFile, "%20s%s\n", "spCType: ", cTypeString(gSRsrcTypeData).c_str());
			fprintf(gOutFile, "%20s%s\n", "spDrvrSW: ", DrSWString(gSRsrcTypeData).c_str());
			fprintf(gOutFile, "%20s%s\n", "spDrvrHW: ", DrHWString(gSRsrcTypeData).c_str());
			fprintf(gOutFile, "%20s%02" PRIX8 "\n", "spSlot: ", (uint8_t)sp.spSlot);
			fprintf(gOutFile, "%20s%02" PRIX8 "%s\n", "spID: ", (uint8_t)sp.spID, GetRsrcKind((uint8_t)sp.spID, rsrcDirDir));
			fprintf(gOutFile, "%20s%02" PRIX8 "\n", "spExtDev: ", (uint8_t)sp.spExtDev);
			fprintf(gOutFile, "%20s%02" PRIX8 "\n", "spHWDev: ", (uint8_t)sp.spHwDev);
			fprintf(gOutFile, "\n");

			err = SRsrcInfo(&sp);
			DoErr("SRsrcInfo", err);
			if (err == noErr)
			{
				fprintf(gOutFile, "%20s%04" PRIX16 "\n", "spIOReserved: ", sp.spIOReserved);

				if (sp.spSlot != curSlot)
					WriteSInfoRecordAndFHeaderRec(uint8_t(sp.spSlot), sResDirFromInfo, sResDirFromHead, sRootDirFromXHead, false);
				curSlot = sp.spSlot;

				fprintf(gOutFile, "\n");
				fprintf(gOutFile, "Slot Resource Directory from SpBlock.spsPointer:\n");
				WriteSResourceDirectory(sp.spsPointer, rsrcDir, std::string("from SpBlock.spsPointer "));
			}
		}
		sp.spParamData = (1 << fall) + (1 << fnext);
	}
} /* SlotsDumpSecond */


void InitIconWindow()
{
	Rect    bounds;
	int16_t genevaID;

	bounds = qd.screenBits.bounds;
	bounds.top = 20;
	bounds.bottom = bounds.bottom / 2;
	bounds = qd.screenBits.bounds;
	bounds.top = bounds.bottom / 2;
	gWind = NewCWindow(NULL, &bounds, "\p", true, plainDBox, WindowPtr(-1), false, 0);
	SetPort(gWind);
	GetFNum("\pGeneva", &genevaID);
	TextFont(genevaID);
	TextMode(srcOr);
	TextFace(0);
	TextSize(9);
	gCurWidth = defaultWidth;
	gCurTop = defaultTop;
	gCurLeft = defaultLeft;
} /* InitIconWindow */


int main(/*int argc, char **argv */)
{
	printf("-------------\n");

	gOutFile = fopen("SlotsDump.txt", "w");

	MoveWindow(FrontWindow(), 0, 20, false);
	InitIconWindow();

	SlotsDumpMain();
	SlotsDumpSecond();

	fclose(gOutFile);

	printf("done\n");
	//DisposeWindow(gWind);
} /* Main */

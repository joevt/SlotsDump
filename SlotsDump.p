PROGRAM SlotsDump;
USES
	Types, QuickDraw, TextUtils, ToolUtils, Fonts, Processes, Devices, Slots, Video, Icons, ROMDefs, PasLibIntf, Windows;


CONST
	kIgnoreError = TRUE;

	ltab1 = '                                        ';

	rsrcDirDirDir = 10;
	rsrcDirDir = 11;
	rsrcDir = 12;
	drvrDir = 13;
	loadDir = 14;
	vendorDir = 15;
	vidNamesDir = 16;
	GammaDir = 17;
	vidModeDir = 18;
	unknownSBlockDir = 19;

	keymapAddr = $174;

	numToCover = 100 * 1024;

TYPE
	long = RECORD
			CASE integer OF
				0: (
						l: LongInt
				);
				1: (
						w: ARRAY[0..1] OF Integer;
				);
				2: (
						b: PACKED ARRAY[0..3] OF SignedByte;
				)
		END;

	sRsrcTypeRec = RECORD
			category, cType, DrSW, DrHW: Integer;
		END;

	sPRAMRecord = RECORD
			CASE integer OF
				0: (
						allBytes: ARRAY[0..11] OF signedByte
				);
				1: (
						blockSize: LongInt;
						bytes: ARRAY[0..7] OF signedByte
				);
		END;

	KeyMapPtr = ^KeyMap;


	sExecBlock = RECORD
			blockSize: LongInt;
			revisionLevel: SignedByte;
			cpuId: SignedByte;
			reserved: Integer;
			codeOffset: LongInt;
		END;

	dirStr = STRING[49];
	dirListEntry = RECORD
			addr: Ptr;
			name: dirStr;
		END;
	dirListArr = ARRAY[0..100] OF dirListEntry;
	dirListHdl = ^dirListPtr;
	dirListPtr = ^dirListArr;
	dirListEntryPtr = ^dirListEntry;


	DriverHeaderRec = RECORD
			blockSize: LongInt;
			drvrFlags, drvrDelay, drvrEMask, drvrMenu, drvrOpen, drvrPrime, drvrCtl, drvrStatus, drvrClose: Integer;
			drvrName: Str255;
		END;

CONST
	defaultWidth = 64;
	defaultTop = 10;
	defaultLeft = 10;
	textLineHeight = 12;

VAR
	gDirList: Handle;
	gByteLanes: SignedByte;

	gTopOfRom: Ptr;
	gWind: WindowPtr;

	gCurWidth, gCurLeft, gCurTop: Integer;
	gSRsrcTypeData: sRsrcTypeRec;					{ current sRsrc type }

	gMinAddr, gMaxAddr: Ptr;

	gOutFile : Text;

	gCoveredPtr : Ptr;

	gDoCovering : Boolean;

	gLevel	:	Integer;

FUNCTION GetTab(i : Integer) : Str255;
VAR
	s	:	Str255;
	n	:	Integer;
BEGIN
	s[0] := chr(i);
	FOR n := 1 TO i DO
		s[n] := Chr(9);
	GetTab := s;
END;

PROCEDURE SetCovered( p : Ptr; len : LongInt );
	VAR
		bitNum	:	LongInt;
		numBits	:	LongInt;
	BEGIN
		IF gDoCovering THEN
			BEGIN
				numBits := UInt32(gTopOfRom) - UInt32(p);
				IF numBits < 0 THEN
					BEGIN
						WriteLn( StringOf( 'topOfRom:', gTopOfRom:0, ' p:', p:0, ' numBits:', numBits:0 ) );
						WriteLn( gOutFile, StringOf( 'topOfRom:', gTopOfRom:0, ' p:', p:0, ' numBits:', numBits:0 ) );
					END;
				FOR bitNum := numBits - len TO numBits - 1 DO
					IF bitNum >= 0 THEN
						IF bitNum < numToCover THEN
							BitSet( gCoveredPtr, bitNum );
			END;
	END;


PROCEDURE WriteCString (p: Ptr);
	VAR
		ch : CHAR;
	BEGIN
		WHILE p^ <> 0 DO
			BEGIN
				ch := Chr(Length(StringPtr(p)^));
				IF ch < ' ' THEN ch := '.';
				Write(gOutFile,ch);
				p := ptr(ord4(p) + 1);
			END;
	END;


FUNCTION Hex (hexPtr: Ptr;
								numBytes: Integer): Str255;
	CONST
		hexs = '0123456789ABCDEF';
	VAR
		i: Integer;
		s: Str255;
	BEGIN
		s := '';
		FOR i := 1 TO numBytes DO
			BEGIN
				s := concat(s, hexs[bsr(band(hexPtr^, $0F0), 4) + 1]);
				s := concat(s, hexs[band(hexPtr^, $0F) + 1]);
				hexPtr := Ptr(Ord4(hexPtr) + 1);
			END;
		hex := s;
	END;


FUNCTION HexLong( x : UNIV LongInt ) : Str255;
BEGIN
	HexLong := Hex(@x, 4);
END;


PROCEDURE WriteChars(p : UNIV LongINt; numChars : Longint );
VAR
	ch : CHAR;
BEGIN
	FOR numChars := numChars DOWNTO 1 DO
		BEGIN
			ch := Chr(length(StringPtr(p)^));
			IF ch < ' ' THEN ch := '.';
			Write(gOutFile,ch);
			p := p+1;
		END;
END;

PROCEDURE WriteUncoveredBytes;
VAR
	addr	:	Ptr;
	count	:	LongInt;
	bytes	:	ARRAY [0..31] OF signedByte;
	i		:	Integer;
	bitNum 	:	LongInt;
BEGIN
	gDoCovering := FALSE;
	bitNum := numToCover - 1;

	WHILE (bitNum >= 0) & NOT BitTst(gCoveredPtr,bitNum) DO
		bitNum := bitNum - 1;

	WHILE bitNum >= 0 DO
		BEGIN
			WHILE (bitNum >= 0) & BitTst(gCoveredPtr,bitNum) DO
				bitNum := bitNum - 1;

			IF bitNum >= 0 THEN
				BEGIN
					addr := Ptr(Ord4(gTopOfRom) - bitNum - 1);
					count := 0;
					Write(gOutFile, HexLong(addr),': ');
					WHILE (bitNum >= 0) & NOT BitTst(gCoveredPtr,bitNum) DO
						BEGIN
							bitNum := bitNum - 1;
							bytes[count] := addr^;
							count := count + 1;
							addr := Ptr(Ord4(addr)+1);
							IF count > 31 THEN
								BEGIN
									FOR i := 0 TO 15 DO
										Write(gOutFile, Hex(@bytes[i*2],2), ' ');
									Write(gOutFile,' “');
									WriteChars(@bytes,32);
									WriteLn(gOutFile,'”');
									count := 0;
									Write(gOutFile, HexLong(addr),': ');
								END;
						END; { WHILE }
					IF count <> 0 THEN
						BEGIN
							FOR i := 0 TO 31 DO
								BEGIN
									IF i < count THEN
										Write(gOutFile, Hex(@bytes[i],1))
									ELSE
										Write(gOutFile, '  ');
									IF i MOD 2 = 1 THEN
										Write(gOutFile, ' ');
								END;
							Write(gOutFile,' “');
							WriteChars(@bytes,count);
							WriteLn(gOutFile,'”');
							Write(gOutFile, HexLong(addr),': ');
						END;

				END; { IF }
			WriteLn(gOutFile);
			WriteLn(gOutFile);
		END; { WHILE }
	gDoCovering := TRUE;
END; { WriteUncoveredBytes }

FUNCTION StrOf (n: LongINt): Str255;
	VAR
		s: Str255;
	BEGIN
		NumToString(n, s);
		StrOf := s;
	END;


FUNCTION ErrS (err: OsErr): Str255;
	VAR
		s: Str255;
	BEGIN
		CASE err OF
			0:
				s := 'noErr - No error';
			-300:
				s := 'smEmptySlot - No card in slot';
			-305:
				s := 'smDisabledSlot - This slot is disabled';
			-309:
				s := 'smBLFieldBad - ByteLanes field was bad';
			-337:
				s := 'smSlotOOBErr - Slot out of bounds';
			-346:
				s := 'smBadsPtrErr - Bad pointer was passed to sCalcPointer';
			-344:
				s := 'smNoMoresRsrcs - No more sResources';
			OTHERWISE
				s := 'unknown err';
		END;
		ErrS := Concat(StrOf(err), ' = ', s);
	END;

PROCEDURE DoErr (msg: Str255;
								err: OSErr);
	BEGIN
		IF err <> noErr THEN
			BEGIN
				WriteLn('Error occured.', ErrS(err), ' in ', msg);
				WriteLn(gOutFile,'Error occured.', ErrS(err), ' in ', msg);
			END;
	END;

FUNCTION CalcAddr (base: Ptr;
								offset: LongInt): Ptr;
	VAR
		theSpBlock: spBlock;
		err: OsErr;
	BEGIN
		theSPBlock.spsPointer := base;
		theSPBlock.spOffsetData := offset;
		theSPBlock.spbyteLanes := gByteLanes;
		err := SCalcSpointer(@theSPBLock);
		DoErr('CalcAddr', err);
		IF offset = 0 THEN
			IF base <> theSPBlock.spsPointer THEN
				WriteLn( 'base ', base : 0, ' offset 0 ptr:', theSPBlock.spsPointer : 0 );
		CalcAddr := theSPBlock.spsPointer;
	END;

FUNCTION Get3 (n: LongInt): LongInt;
	VAR
		temp : long;
	BEGIN
		temp.l := n;
		IF BTst(n, 23) THEN
			temp.b[0] := -1
		ELSE
			temp.b[0] := $00;
		Get3 := temp.l;
	END;

PROCEDURE fillBytes (p: Ptr;
								n: LongInt);
	BEGIN
		WHILE n > 0 DO
			BEGIN
				p^ := 0;
				n := n - 1;
				p := ptr(ord4(p) + 1);
			END;
	END;

FUNCTION PtrOf( x : UNIV Ptr ) : Ptr;
INLINE $2E9F;


PROCEDURE GetBytes (source, dest: Ptr;
								numBytes: LongInt);
	VAR
		startPtr: Ptr;
	BEGIN
		startPtr := source;
		WHILE numBytes > 0 DO
			BEGIN
				IF UInt32(source) < UInt32(gTopOfRom) THEN
					BEGIN
						dest^ := source^;
						dest := ptr(Ord4(dest) + 1);
						source := CalcAddr(source, 1);
						numBytes := numBytes - 1;
					END
				ELSE
					BEGIN
						WriteLn( 'Bytes would exceed rom length' );
						WriteLn( gOutFile, 'Bytes would exceed rom length' );
						Leave;
					END
			END;
		SetCovered( startPtr, Ord4(source) - ORD4(startPtr) );
	END;


PROCEDURE GetRsrc (p: Ptr;
								VAR s: signedByte;
								VAR offset: LongInt);
	BEGIN
		GetBytes(p, @offset, 4);
		s := long(offset).b[0];
		offset := Get3(offset);
	END;


PROCEDURE GetSBlock (source: Ptr;
								VAR dest: Handle);
	VAR
		len: LongInt;
	BEGIN
		GetBytes(source, @len, 4);
		len := len - 4;
		IF dest = NIL THEN
			dest := NewHandle(len)
		ELSE
			SetHandleSize(dest, len);
		source := CalcAddr(source, 4);
		HLock(dest);
		GetBytes(source, dest^, len);
		HUnLock(dest);
	END;


PROCEDURE GetCString (source: Ptr;
								VAR dest: Handle);
	VAR
		len: LongInt;
		destPtr: Ptr;
		startPtr : Ptr;
	BEGIN
		startPtr := source;
		IF dest = NIL THEN
			dest := NewHandle(0);

		len := 0;
		REPEAT
			IF UInt32(source) < UInt32(gTopOfRom) THEN
				BEGIN
					len := len + 1;
					IF len > GetHandleSize(dest) THEN
						SetHandleSize(dest, len);
					destPtr := Ptr(Ord4(dest^) + len - 1);
					destPtr^ := source^;
					source := CalcAddr(source, 1);
				END
			ELSE
				BEGIN
					WriteLn( 'CString would exceed rom length' );
					WriteLn( gOutFile, 'CString would exceed rom length' );
					destPtr^ := 0;
					Leave;
				END
		UNTIL destPtr^ = 0;

		IF (BAND(Ord4(source) - Ord4(startPtr), 1) <> 0) THEN
			IF UInt32(source) < UInt32(gTopOfRom) THEN
				if source^ = 0 THEN
					source := CalcAddr(source, 1);

		SetCovered(startPtr, Ord4(source) - ORD4(startPtr));
	END;


PROCEDURE GetIconBytes (source: Ptr;
								VAR dest: Handle);
	BEGIN
		IF dest = NIL THEN
			dest := NewHandle(0);
		IF GetHandleSize(dest) < 128 THEN
			SetHandleSize(dest, 128);
		HLock(dest);
		GetBytes(source, dest^, 128);
		HUnLock(dest);
	END; { GetIconBytes }


PROCEDURE WriteIcon (icon: Handle;
								CONST title: Str255);
	VAR
		r: Rect;
		width: Integer;
	BEGIN
		SetRect(r, 0, 0, 32, 32);
		IF gCurTop > gWind^.portRect.bottom - 32 - textLineHeight THEN
			BEGIN
				gCurTop := defaultTop;
				gCurLeft := gCurLeft + gCurWidth;
				gCurWidth := defaultWidth;
			END;
		OffsetRect(r, gCurLeft, gCurTop);
		PlotIcon(r, icon);
		MoveTo(gCurLeft, gCurTop + 32 + textLineHeight);
		DrawString(title);
		width := StringWidth(title) + 10;
		IF gCurWidth < width THEN
			gCurWidth := width;
		gCurTop := gCurTop + 32 + textLineHeight + 4;
	END; { WriteIcon }


PROCEDURE WriteIcl (icon: Ptr;
								depth: Integer;
								CONST title: Str255);
	VAR
		pix: PixMap;
		r: Rect;
		width: Integer;
	BEGIN
		Pix := CGrafPtr(gWind)^.portPixMap^^;

		WITH Pix DO
			BEGIN
				baseAddr := icon;
				pixelType := 0;
				cmpCount := 1;
				SetRect(bounds, 0, 0, 32, 32);
				IF depth = 4 THEN
					BEGIN
						pixelSize := 4;
						cmpSize := 4;
						pmTable := GetCTable(4);
					END
				ELSE IF depth = 8 THEN
					BEGIN
						pixelSize := 8;
						cmpSize := 8;
						pmTable := GetCTable(8);
					END;
			END;

		SetRect(r, 0, 0, 32, 32);
		IF gCurTop > gWind^.portRect.bottom - 32 - textLineHeight THEN
			BEGIN
				gCurTop := defaultTop;
				gCurLeft := gCurLeft + gCurWidth;
				gCurWidth := defaultWidth;
			END;
		OffsetRect(r, gCurLeft, gCurTop);
		CopyBits(BitMapPtr(@Pix)^, gWind^.portBits, Pix.bounds, r, srcCopy, NIL);
		MoveTo(gCurLeft, gCurTop + 32 + textLineHeight);
		DrawString(title);
		width := StringWidth(title) + 10;
		IF gCurWidth < width THEN
			gCurWidth := width;
		gCurTop := gCurTop + 32 + textLineHeight + 4;
	END; { WriteIcl }


PROCEDURE WriteCIcon (icon: Handle;
								CONST title: Str255);
	VAR
		r: Rect;
		width: Integer;

		iconHeight: Integer;
		iconWidth: Integer;

		bmapSize: LongInt;
		maskSize: LongInt;
		colorSize: LongInt;
		pixSize: LongInt;


		pixData: Handle;
		colors: Handle;

		cIconPtr: Ptr;

		maskPtr: Ptr;
		bmapPtr: Ptr;
		colorPtr: Ptr;
		pixPtr: Ptr;
	BEGIN
		HLock(icon);
		cIconPtr := icon^;
		WITH cIconHandle(icon)^^ DO
			BEGIN
				WITH iconMask, bounds DO
					BEGIN
						maskSize := rowBytes * (bottom - top);
						maskPtr := Ptr(ORd4(cIconPtr) + sizeOf(PixMap) + 2 * SizeOf(BitMap) + 4);
						baseAddr := maskPtr;
					END;
				WITH iconBMap, bounds DO
					BEGIN
						bmapSize := rowBytes * (bottom - top);
						bmapPtr := Ptr(ORd4(maskPtr) + maskSize);
						baseAddr := bmapPtr;
					END;

				colorPtr := Ptr(ORd4(bmapPtr) + bmapSize);
				WITH CTabPtr(colorPtr)^ DO
					colorSize := 8 + 8 * (ctSize + 1);
				colors := NewHandle(colorSize);
				BlockMove(colorPtr, colors^, colorSize);

				pixPtr := Ptr(Ord4(colorPtr) + colorSize);
				WITH iconPMap, bounds DO
					BEGIN
						iconWidth := right - left;
						iconHeight := bottom - top;
						pixSize := Band(rowBytes, $7FFF) * iconHeight;
						pixData := NewHandle(pixSize);
						BlockMove(pixPtr, pixData^, pixSize);
						baseAddr := pixData^;
						pmTable := cTabHandle(colors);
					END;
				iconData := pixData;
			END;
		HUnlock(icon);

		SetRect(r, 0, 0, iconWidth, iconHeight);
		IF gCurTop > gWind^.portRect.bottom - iconHeight - textLineHeight THEN
			BEGIN
				gCurTop := defaultTop;
				gCurLeft := gCurLeft + gCurWidth;
				gCurWidth := defaultWidth;
			END;
		OffsetRect(r, gCurLeft, gCurTop);
		PlotCIcon(r, cIconHandle(icon));

		DisposeHandle(pixData);
		DisposeHandle(colors);
		MoveTo(gCurLeft, gCurTop + 32 + textLineHeight);

		DrawString(title);
		width := StringWidth(title) + 10;
		iconWidth := iconWidth + 10;
		IF width < iconWidth THEN
			width := iconWidth;
		IF gCurWidth < width THEN
			gCurWidth := width;
		gCurTop := gCurTop + iconHeight + textLineHeight + 4;
	END; { WriteCIcon }


PROCEDURE WriteByteLanes (theByteLanes: SignedByte);
	BEGIN
		Write(gOutFile,'= byte lanes: ');
		CASE theByteLanes OF
			$1:
				WriteLn(gOutFile,'0');
			$2:
				WriteLn(gOutFile,'1');
			$3:
				WriteLn(gOutFile,'0,1');
			$4:
				WriteLn(gOutFile,'2');
			$5:
				WriteLn(gOutFile,'0,2');
			$6:
				WriteLn(gOutFile,'1,2');
			$7:
				WriteLn(gOutFile,'0,1,2');
			$8:
				WriteLn(gOutFile,'3');
			$9:
				WriteLn(gOutFile,'0,3');
			$A:
				WriteLn(gOutFile,'1,3');
			$B:
				WriteLn(gOutFile,'0,1,3');
			$C:
				WriteLn(gOutFile,'2,3');
			$D:
				WriteLn(gOutFile,'0,2,3');
			$E:
				WriteLn(gOutFile,'1,2,3');
			$F:
				WriteLn(gOutFile,'0,1,2,3');
			OTHERWISE
				WriteLn(gOutFile,'illegal');
		END;
	END; { WriteByteLanes }


PROCEDURE WriteByteLanes2 (theByteLanes: SignedByte);
	BEGIN
		CASE Length( StringPtr( @theByteLanes )^ ) OF
			$E1,
			$D2,
			$C3,
			$B4,
			$A5,
			$96,
			$87,
			$78,
			$69,
			$5A,
			$4B,
			$3C,
			$2D,
			$1E,
			$0F: WriteByteLanes( BAND( theByteLanes, $0F ) );
			OTHERWISE
				WriteLn(gOutFile,'= byte lanes: illegal');
		END;
	END; { WriteByteLanes2 }


PROCEDURE WriteSInfoRecord (theSInfoRecPtr: SInfoRecPtr; VAR sResDirFromInfo : Ptr);
	VAR
		slot: SignedByte;
	BEGIN
		WITH theSInfoRecPtr^ DO
			IF ( siInitStatusA = 0 ) | ( kIgnoreError & ( siCPUByteLanes <> 0 ) ) THEN
				BEGIN
					WriteLn(gOutFile,'siDirPtr: ' : 20, Hex(@siDirPtr,4));

					sResDirFromInfo := siDirPtr;

					WriteLn(gOutFile,'siInitStatusA: ' : 20, errs(siInitStatusA));
					WriteLn(gOutFile,'siInitStatusV: ' : 20, siInitStatusV : 0);

					Write(gOutFile,'siState: ' : 20, siState : 0);
					CASE siState OF
						0:
							WriteLn(gOutFile,' = stateNil');
						1:
							WriteLn(gOutFile,' = stateSDMInit');
						2:
							WriteLn(gOutFile,' = statePRAMInit');
						3:
							WriteLn(gOutFile,' = statePInit');
						4:
							WriteLn(gOutFile,' = stateSInit');
						OTHERWISE
							WriteLn(gOutFile);
					END;

					Write(gOutFile,'siCPUByteLanes: ' : 20, Hex(@siCPUByteLanes, 1), ' ');
					WriteByteLanes(siCPUByteLanes);

					WriteLn(gOutFile,'siTopOfROM: ' : 20, Hex(@siTopOfROM, 1));
					WriteLn(gOutFile,'siStatusFlags: ' : 20, Hex(@siStatusFlags, 1));

					Write(gOutFile,'siTOConst: ' : 20, siTOConst : 0);
					IF siTOConst = defaultTO THEN
						WriteLn(gOutFile,' = defaultTO')
					ELSE
						WriteLn(gOutFile);

					WriteLn(gOutFile,'siReserved: ' : 20, hex(@siReserved, 2));
					WriteLn(gOutFile,'siROMAddr: ' : 20, Hex(@siROMAddr,4));

					slot := signedByte(siSlot);
					WriteLn(gOutFile,'siSlot: ' : 20, Hex(@slot, 1));

					WriteLn(gOutFile,'siPadding: ' : 20, Hex(@siPadding, 3));

					gByteLanes := BAND( siCPUByteLanes, $0F );
					gTopOfRom := CalcAddr(siRomAddr, 1);
				END
			ELSE
				WriteLn(gOutFile,'siInitStatusA: ' : 20, errs(siInitStatusA));
	END; { WriteSInfoRecord }


PROCEDURE WriteFHeaderRec (theFHeaderRecPtr: FHeaderRecPtr; VAR sResDirFromHead, HeaderWhere: Ptr );
	VAR
		theOffset: LongInt;
	BEGIN
		WITH theFHeaderRecPtr^ DO
			BEGIN
				theOffset := Get3(fhDirOffset);
				HeaderWhere := CalcAddr(gTopOfRom, -SizeOf(FHeaderRec));

				sResDirFromHead := CalcAddr(gTopOfRom, -SizeOf(FHeaderRec) + theOffset);

				WriteLn(gOutFile,Hex(@headerWhere,4));

				WriteLn(gOutFile,'fhDirOffset: ' : 20, Hex(@fhDirOffset, 1), ' ', Hex(Ptr(ord4(@theOffset) + 1), 3), ' = ', Hex(@sResDirFromHead,4));
				WriteLn(gOutFile,'fhLength: ' : 20, fhLength : 0);
				WriteLn(gOutFile,'fhCRC: ' : 20, Hex(@fhCRC, 4));

				Write(gOutFile,'fhROMRev: ' : 20, fhROMRev : 0);
				IF fhROMRev = romRevision THEN
					WriteLn(gOutFile,' = romRevision')
				ELSE
					WriteLn(gOutFile);

				Write(gOutFile,'fhFormat: ' : 20, fhFormat : 0);
				IF fhFormat = appleFormat THEN
					WriteLn(gOutFile,' = appleFormat')
				ELSE
					WriteLn(gOutFile);

				Write(gOutFile,'fhTstPat: ' : 20, Hex(@fhTstPat, 4));
				IF fhTstPat = testPattern THEN
					WriteLn(gOutFile,' = testPattern')
				ELSE
					WriteLn(gOutFile);

				WriteLn(gOutFile,'fhReserved: ' : 20, Hex(@fhReserved, 1));

				Write(gOutFile,'fhByteLanes: ' : 20, Hex(@fhByteLanes, 1), ' ');
				WriteByteLanes2(fhByteLanes);
			END;
	END; { WriteFHeaderRec }


PROCEDURE WriteTable;
	BEGIN
		WriteLn(gOutFile,'sResource Directory Ids:');
		WriteLn(gOutFile,'name' : 20, ' $id id  Description');
		WriteLn(gOutFile,'board' : 20, '  0   0  Board sResource - Required on all boards');
		WriteLn(gOutFile,'endOfList' : 20, ' FF 255 End of list');
		WriteLn(gOutFile);
		WriteLn(gOutFile,'sResource field Ids:');
		WriteLn(gOutFile,'name' : 20, ' $id id  Description');
		WriteLn(gOutFile,'sRsrcType' : 20, '  1   1  Type of the sResource');
		WriteLn(gOutFile,'sRsrcName' : 20, '  2   2  Name of the sResource');
		WriteLn(gOutFile,'sRsrcIcon' : 20, '  3   3  Icon for the sResource');
		WriteLn(gOutFile,'sRsrcDrvr' : 20, '  4   4  Driver directory for the sResource');
		WriteLn(gOutFile,'sRsrcLoadRec' : 20, '  5   5  Load record for the sResource');
		WriteLn(gOutFile,'sRsrcBootRec' : 20, '  6   6  Boot record');
		WriteLn(gOutFile,'sRsrcFlags' : 20, '  7   7  sResource flags');
		WriteLn(gOutFile,'sRsrcHWDevId' : 20, '  8   8  Hardware device ID');
		WriteLn(gOutFile,'minorBaseOS' : 20, '  A  10  Offset from dCtlDevBase to the sResource’s hardware base in standard slot space');
		WriteLn(gOutFile,'minorLength' : 20, '  B  11  Length of the sResource’s address space in standard slot space');
		WriteLn(gOutFile,'majorBaseOS' : 20, '  C  12  Offset from dCtlDevBase to the sResource’s address space in standard slot space');
		WriteLn(gOutFile,'majorLength' : 20, '  D  13  Length of the sResource in super slot space');
		WriteLn(gOutFile,'sRsrccicn' : 20, '  F  15  Color icon');
		WriteLn(gOutFile,'sRsrcicl8' : 20, ' 10  16  8-bit icon data');
		WriteLn(gOutFile,'sRsrcicl4' : 20, ' 11  17  4-bit icon data');
		WriteLn(gOutFile,'sGammaDir' : 20, ' 40  64  Gamma directory');
		WriteLn(gOutFile);
		WriteLn(gOutFile,'sDRVRDir' : 20, ' 10  16  sDriver directory');
		WriteLn(gOutFile);
		WriteLn(gOutFile,'endOfList' : 20, ' FF 255  End of list');
		WriteLn(gOutFile);
		WriteLn(gOutFile);
		WriteLn(gOutFile,'Format of sRsrcType: Category, cType, DrSW, DrHW');
		WriteLn(gOutFile);
	END;


FUNCTION GetRsrcKind (id: SignedByte;
								whichDir: Integer) : Str255;
VAR
	sRsrcId	:	Integer;
	s		:	Str255;
BEGIN
	sRsrcId := Length(StringPtr(@id)^);
	IF sRsrcId = 255 THEN
		s := ' ; endOfList       '
	ELSE

		CASE whichDir OF

			rsrcDirDir:
				IF SRsrcId = 0 THEN
					s := ' ; board           '
				ELSE
					s := '                   ';

			rsrcDir, gammaDir:
				CASE sRsrcId OF
					1:
						s := ' ; sRsrcType       ';
					2:
						s := ' ; sRsrcName       ';
					3:
						s := ' ; sRsrcIcon       ';
					4:
						s := ' ; sRsrcDrvrDir    ';
					5:
						s := ' ; sRsrcLoadRec    ';
					6:
						s := ' ; sRsrcBootRec    ';
					7:
						s := ' ; sRsrcFlags      ';
					8:
						s := ' ; sRsrcHWDevId    ';
					10:
						s := ' ; minorBaseOS     ';
					11:
						s := ' ; minorLength     ';
					12:
						s := ' ; majorBaseOS     ';
					13:
						s := ' ; majorLength     ';
					15:
						s := ' ; sRsrcCicn       ';
					16:
						s := ' ; sRsrcIcl8       ';
					17:
						s := ' ; sRsrcIcl4       ';
					64:
						s := ' ; sGammaDir       ';

					32:
						s := ' ; boardId         ';
					33:
						s := ' ; PRAMInitData    ';
					34:
						s := ' ; primaryInit     ';
					35:
						s := ' ; sTimeOut        ';
					36:
						s := ' ; vendorInfo      ';
					37:
						s := ' ; boardFlags      ';
					38:
						s := ' ; secondaryInit   ';
					65:
						s := ' ; sRsrcVidNames   ';
					OTHERWISE
						IF gSRsrcTypeData.category = CatDisplay THEN
							CASE sRsrcId OF
								128:
									s := ' ; 1:firstVidMode  ';
								129:
									s := ' ; 2:secondVidMode ';
								130:
									s := ' ; 4:thirdVidMode  ';
								131:
									s := ' ; 8:fourthVidMode ';
								132:
									s := ' ; 16:fifthVidMode ';
								133:
									s := ' ; 32:sixthVidMode ';
								OTHERWISE
									IF whichDir = RsrcDir THEN
										s := ' ; sRsrcUnknown    '
									ELSE
										s := ' ; unknownVidMode  ';
							END
						ELSE
							s := ' ; sRsrcUnknown    ';
				END;

			drvrDir:
				CASE SRsrcId OF
					1:
						s := ' ; sMacOS68000     ';
					2:
						s := ' ; sMacOS68020     ';
					3:
						s := ' ; sMacOS68030     ';
					4:
						s := ' ; sMacOS68040     ';
					OTHERWISE
						s := ' ; sMacOSUnknown   ';
				END;


			vendorDir:
				CASE SRsrcId OF
					1:
						s := ' ; vendorID        ';
					2:
						s := ' ; serialNum       ';
					3:
						s := ' ; revLevel        ';
					4:
						s := ' ; partNum         ';
					5:
						s := ' ; date            ';
					OTHERWISE
						s := ' ; unknownInfo     ';
				END;

			vidModeDir:
				CASE SRsrcId OF
					1:
						s := ' ; mVidParams      ';
					2:
						s := ' ; mTable          ';
					3:
						s := ' ; mPageCnt        ';
					4:
						s := ' ; mDevType        ';
					5:
						s := ' ; mHRes           ';
					6:
						s := ' ; mVRes           ';
					7:
						s := ' ; mPixelType      ';
					8:
						s := ' ; mPixelSize      ';
					9:
						s := ' ; mCmpCount       ';
					10:
						s := ' ; mCmpSize        ';
					11:
						s := ' ; mPlaneBytes     ';
					14:
						s := ' ; mVertRefRate    ';
					OTHERWISE
						s := ' ; mUnknown        ';
				END;
			OTHERWISE
				s := ' ;                 ';

		END; { CASE whichDir }
	GetRsrcKind := s;
END;


PROCEDURE WriteRsrcKind (id: SignedByte;
								whichDir: Integer);
BEGIN
	Write( gOutFile, GetRsrcKind( id, whichDir ) );
END; { WriteRsrcKind }


FUNCTION GetspParamDataEnabledDisabledString( spParamData : LongInt ) : Str255;
BEGIN
	CASE spParamData OF
		0:			GetspParamDataEnabledDisabledString := ' ; Enabled';
		1:			GetspParamDataEnabledDisabledString := ' ; Disabled';
		OTHERWISE	GetspParamDataEnabledDisabledString := ' ; ???';
	END;
END; { GetspParamDataEnabledDisabledString }


{$S SlotsDump2}
FUNCTION CategoryString (category: Integer): Str255;
	VAR
		s: Str255;
	BEGIN
		s := Hex(@category, 2);
		CASE category OF
			1:
				s := Concat(s, '=CatBoard');
			2:
				s := Concat(s, '=CatTest');
			3:
				s := Concat(s, '=CatDisplay');
			4:
				s := Concat(s, '=CatNetwork');
			10:
				s := Concat(s, '=CatCPU');
			OTHERWISE
				s := Concat(s, '=CatUnknown');
		END;
		CategoryString := s;
	END;

FUNCTION cTypeString (VAR theSRsrcTypeData: sRsrcTypeRec): Str255;
	VAR
		s: Str255;
	BEGIN
		WITH theSRsrcTypeData DO
			BEGIN
				s := Hex(@cType, 2);
				CASE cType OF
					0:
						s := Concat(s, '=TypBoard');
					1:
						IF category = CatDisplay THEN
							s := Concat(s, '=TypVideo')
						ELSE
							s := Concat(s, '=TypEthernet');
					2:
						s := Concat(s, '=TypLCD');
					4:
						s := Concat(s, '=Typ68030');
					5:
						s := Concat(s, '=Typ68040');
					21:
						s := Concat(s, '=TypAppleII');
					OTHERWISE
						s := Concat(s, '=TypUnknown');
				END;
			END;
		cTypeString := s;
	END;

FUNCTION DrSWString (VAR theSRsrcTypeData: sRsrcTypeRec): Str255;
	VAR
		s: Str255;
	BEGIN
		WITH theSRsrcTypeData DO
			BEGIN
				s := Hex(@DrSW, 2);
				CASE DrSW OF
					0:
						s := Concat(s, '=DrSWBoard');
					1:
						IF category = 10 THEN
							s := Concat(s, '=DrSWAppleIIe')
						ELSE
							s := Concat(s, '=DrSWApple');
					2:
						s := Concat(s, '=DrSWCompanyA');
					OTHERWISE
						s := Concat(s, '=DrSWUnknown');
				END;
			END;
		DrSWString := s;
	END;

FUNCTION DrHWString (VAR theSRsrcTypeData: sRsrcTypeRec): Str255;
	VAR
		s: Str255;
	BEGIN
		WITH theSRsrcTypeData DO
			BEGIN
				s := Hex(@DrHW, 2);
				CASE DrHW OF
					0:
						s := Concat(s, '=DrHWBoard');
					1:
						CASE category OF
							CatDisplay:
								s := Concat(s, '=DrHWTFB');
							10:
								s := Concat(s, '=DrHWFeatureCard');
							OTHERWISE
								s := Concat(s, '=DrHW3Com');
						END;
					2:
						s := Concat(s, '=DrHWproductA');
					3:
						IF category = CatDisplay THEN
							s := Concat(s, '=DrHWproductB')
						ELSE
							s := Concat(s, '=DrHWBSC');
					4:
						s := Concat(s, '=DrHWproductC');
					5:
						s := Concat(s, '=DrHWproductD');
					24:
						IF category = CatDisplay THEN
							s := Concat(s, '=DrHWRBV1')
						ELSE
							s := Concat(s, '=DrHWMacFamily');
					26:
						s := Concat(s, '=DrHWV8');
					27:
						s := Concat(s, '=DrHWTIM');
					28:
						s := Concat(s, '=DrHWDAFB');
					30:
						s := Concat(s, '=DrHWDBLite');
					34:
						s := Concat(s, '=DrHWSonora');
					272:
						s := Concat(s, '=DrHWSonic');
					OTHERWISE
						s := Concat(s, '=DrHWUnknown');
				END;
			END;
		DrHWString := s;
	END;


FUNCTION GetSRsrcTypeDataString( VAR theSRsrcTypeData: sRsrcTypeRec): Str255;
	BEGIN
		GetSRsrcTypeDataString := StringOf( categoryString(theSRsrcTypeData.category), ' ', cTypeString(gSRsrcTypeData), ' ', DrSWString(gSRsrcTypeData), ' ', DrHWString(gSRsrcTypeData) );
	END;


PROCEDURE WriteSExecBlock (tabStr: Str255;
								dataAddr: Ptr);
	VAR
		sExecBlockData: sExecBlock;
	BEGIN
		tabStr := concat(tabStr,ltab1);
		GetBytes(dataAddr, @sExecBlockData, SizeOf(sExecBlockData));
		WITH sExecBlockData DO
			BEGIN
				WriteLn(gOutFile,'block size = ' : 20, hex(@blockSize, 1), ' ', hex(Ptr(Ord4(@blockSize) + 1), 3));
				SetCovered(dataAddr, Ord4(CalcAddr(dataAddr,blockSize))-Ord4(CalcAddr(dataAddr,0)));


				Write(gOutFile,tabStr, 'revision level: ' : 20, revisionLevel : 0);
				IF revisionLevel = sCodeRev THEN
					WriteLn(gOutFile,' = sCodeRev')
				ELSE
					WriteLn(gOutFile);

				Write(gOutFile,tabStr, 'CPU ID: ' : 20, cpuID : 0);
				CASE cpuID OF
					1:
						WriteLn(gOutFile,' = sCPU68000');
					2:
						WriteLn(gOutFile,' = sCPU68020');
					3:
						WriteLn(gOutFile,' = sCPU68030');
					4:
						WriteLn(gOutFile,' = sCPU68040');
					OTHERWISE
						WriteLn(gOutFile);
				END; { CASE }

				WriteLn(gOutFile,tabStr, 'code offset: ' : 20, Hex(@codeOffset,4), ' = ', HexLong(CalcAddr(dataAddr, 8 + codeOffset)));
			END; { WITH }
	END; { WriteSExecBlock }


PROCEDURE WriteSResourceDirectory (dirPtr: Ptr;
								whichDir: Integer;
								CONST curStr: Str255);
FORWARD;


FUNCTION CStrLen (p: Ptr): LongInt;
	VAR
		i: LongInt;
	BEGIN
		i := 1;
		WHILE p^ <> 0 DO
			BEGIN
				i := i + 1;
				p := ptr(ord4(p) + 1);
			END;
		CStrLen := i;
	END;

PROCEDURE WritePRAMInitData (addr: Ptr);
	VAR
		paramData: sPRAMRecord;
	BEGIN
		GetBytes(addr, @paramData, sizeOf(sPRAMRecord));
		WriteLn(gOutFile);
		WITH paramData DO
			BEGIN
				WriteLn(gOutFile, GetTab(gLevel), 'block size = ' : 20, Hex(@allbytes[0], 1), ' ', Hex(@allbytes[1], 3));
				SetCovered(addr, Ord4(CalcAddr(addr,blockSize))-Ord4(CalcAddr(addr,0)));

				WriteLn(gOutFile, GetTab(gLevel), '0 0 byte1 byte2: ' : 20, Hex(@allbytes[4], 1), ' ', Hex(@allbytes[5], 1), ' ', Hex(@allbytes[6], 1), ' ', Hex(@allbytes[7], 1));
				WriteLn(gOutFile, GetTab(gLevel), 'byte3 to byte6: ' : 20, Hex(@allbytes[8], 1), ' ', Hex(@allbytes[9], 1), ' ', Hex(@allbytes[10], 1), ' ', Hex(@allbytes[11], 1));
			END;
	END;


FUNCTION FindDirEntry (findAddr: Ptr): Boolean;
	VAR
		i: LongInt;
	BEGIN
		FindDirEntry := FALSE;
		HLock(gDirList);
		FOR i := 0 TO GetHandleSize(gDirList) DIV SizeOf(dirListEntry) - 1 DO
			WITH dirListHdl(gDirList)^^[i] DO
				IF findAddr = addr THEN
					BEGIN
						WriteLn(gOutFile,'duplicate: ', name);
						findDirEntry := TRUE;
						Leave;
					END;
		HUnlock(gDirList);
	END;

PROCEDURE AddDirListEntry (newAddr: Ptr;
								CONST newName: dirStr);
	VAR
		curLength: LongInt;
	BEGIN
		curLength := GetHandleSize(gDirList);
		SetHandleSize(gDirList, curLength + SizeOf(dirListEntry));
		HLock(gDirList);
		WITH dirListEntryPtr(PtrOf(Ord4(gDirList^) + curLength))^ DO
			BEGIN
				addr := newAddr;
				name := newName;
			END;
		HUnlock(gDirList);
	END;

PROCEDURE CheckAddrRange(addr : Ptr );
	BEGIN
		IF (Ord4(addr) < Ord4(gMinAddr)) | (gMinAddr = NIL) THEN
			gMinAddr := addr
		ELSE IF (Ord4(addr) > Ord4(gMaxAddr)) | (gMaxAddr = NIL) THEN
			gMaxAddr := addr;
	END;


FUNCTION GetResult (sRsrcId: SignedByte;
								whichDir: Integer;
								addr: Ptr): Str255;
	BEGIN
		IF ((whichDir = rsrcDir) & (sRsrcId IN [sRsrcFlags, sRsrcHWDevId, boardId, timeOutConst, boardFlags])) | ((whichDir = vidModeDir) & (sRsrcId IN [mPageCnt, mDevType])) THEN
			GetResult := '        '
		ELSE
			BEGIN
				CheckAddrRange( addr );
				GetResult := Hex(@addr, 4);
			END;
	END;


PROCEDURE GetAndWriteCString (dataAddr: Ptr;
								VAR miscDataHdl: Handle);
	BEGIN
		GetCString(dataAddr, miscDataHdl);
		Write(gOutFile,'“');
		HLock(miscDataHdl);
		WriteCString(miscDataHdl^);
		HUnlock(miscDataHdl);
		WriteLn(gOutFile,'”');
	END;


VAR
	bytes: PACKED ARRAY[0..1023] OF SignedByte;
	driverHeader: DriverHeaderRec;

PROCEDURE WriteData (id: signedByte;
								offset: LongInt;
								dataAddr: Ptr;
								CONST curStr: Str255;
								whichDir: Integer);
	VAR
		miscDataHdl: Handle;
		longNum: LongInt;
		sRsrcId: Integer;
		tabStr : Str255;


		PROCEDURE DoRsrcDirDir;
		BEGIN
			WriteLn(gOutFile);
			IF sRsrcId <> endOfList THEN
				WriteSResourceDirectory(dataAddr, whichDir + 1, curStr);
		END;


		PROCEDURE DoRsrcDir;
		BEGIN
			IF sRsrcId IN [sRsrcDrvrDir, sRsrcLoadDir, sGammaDir, vendorInfo, sRsrcVidNames, firstVidMode..sixthVidMode, 126] THEN
				AddDirListEntry(dataAddr, curStr);

			CASE sRsrcId OF
				sRsrcType:
					BEGIN
						GetBytes(dataAddr, @gSRsrcTypeData, 8);
						WriteLn(gOutFile, GetSRsrcTypeDataString( gSRsrcTypeData ) );
					END;
				sRsrcName:
					GetAndWriteCString(dataAddr, miscDataHdl);
				sRsrcIcon:
					BEGIN
						GetIconBytes(dataAddr, miscDataHdl);
						WriteIcon(miscDataHdl, curStr);
						WriteLn(gOutFile);
					END;
				sRsrcDrvrDir:
					BEGIN
						WriteLn(gOutFile);
						WriteSResourceDirectory(dataAddr, drvrDir, curStr);
					END;
				sRsrcLoadDir:
					BEGIN
						WriteLn(gOutFile);
						WriteSResourceDirectory(dataAddr, loadDir, curStr);
					END;
				sRsrcBootRec, primaryInit, secondaryInit:
					WriteSExecBlock(GetTab(gLevel), dataAddr);
				sRsrcFlags:
					WriteLn(gOutFile,Hex(Ptr(Ord4(@Offset) + 2), 2), ' 2:f32BitMode=', ORD(BTst(offset, 2)) : 0, '  1:fOpenAtStart=', ORD(BTst(offset, 1)) : 0);
				sRsrcHWDevId:
					WriteLn(gOutFile,Hex(Ptr(Ord4(@Offset) + 3), 1));
				MinorBaseOS:
					BEGIN
						GetBytes(dataAddr, @longNum, 4);
						WriteLn(gOutFile,'$Fs00 0000 + ', Hex(@longNum,4));
					END;
				MajorBaseOS:
					BEGIN
						GetBytes(dataAddr, @longNum, 4);
						WriteLn(gOutFile,'$s000 0000 + ', Hex(@longNum,4));
					END;
				MinorLength, MajorLength:
					BEGIN
						GetBytes(dataAddr, @longNum, 4);
						WriteLn(gOutFile,Hex(@longNum,4));
					END;
				sRsrcCicn:
					BEGIN
						GetSBlock(dataAddr, miscDataHdl);
						WriteCIcon(miscDataHdl, curStr);
						WriteLn(gOutFile);
					END;

				sRsrcIcl8:
					BEGIN
						GetBytes(dataAddr, @bytes, 1024);
						WriteIcl(@bytes, 8, curStr);
						WriteLn(gOutFile);
					END;
				sRsrcIcl4:
					BEGIN
						GetBytes(dataAddr, @bytes, 512);
						WriteIcl(@bytes, 4, curStr);
						WriteLn(gOutFile);
					END;

				sGammaDir:
					BEGIN
						WriteLn(gOutFile);
						WriteSResourceDirectory(dataAddr, gammaDir, curStr);
					END;

				boardId, timeOutConst, boardFlags:
					WriteLn(gOutFile,Hex(Ptr(Ord4(@Offset) + 2), 2));

				PRAMInitData:
					WritePRAMInitData(dataAddr);

				vendorInfo:
					BEGIN
						WriteLn(gOutFile);
						WriteSResourceDirectory(dataAddr, vendorDir, curStr);
					END;

				sRsrcVidNames:
					BEGIN
						WriteLn(gOutFile);
						WriteSResourceDirectory(dataAddr, vidNamesDir, curStr);
					END;

				firstVidMode..sixthVidMode:
					IF gSRsrcTypeData.category = CatDisplay THEN
						BEGIN
							WriteLn(gOutFile);
							WriteSResourceDirectory(dataAddr, vidModeDir, curStr);
						END
					ELSE IF sRsrcId = 128 THEN
						WriteLn(gOutFile,offset : 0)
					ELSE
						BEGIN
							GetBytes(dataAddr, @longNum, 4);
							WriteLn(gOutFile,Hex(@longnum, 4));
						END;


				126:
					BEGIN
						WriteLn(gOutFile);
						WriteSResourceDirectory(dataAddr, unknownSBlockDir, curStr);
					END;

				125:
					WriteLn(gOutFile,offset : 0);


				OTHERWISE
					WriteLn(gOutFile);
			END; { CASE sRsrcId }
		END; { rsrcDir }


		PROCEDURE DoVidModeDir;
		BEGIN
			CASE sRsrcId OF
				mVidParams:
					BEGIN
						tabStr := Concat(GetTab(gLevel),ltab1);

						GetSBlock(dataAddr, miscDataHdl);
						HLock(miscDataHdl);
						longNum := GetHandleSize(miscDataHdl) + 4;
						WriteLn(gOutFile,'block size = ', hex(@longNum, 1), ' ', hex(Ptr(Ord4(@longNum) + 1), 3));
						WITH VPBlockPtr(miscDataHdl^)^ DO
							BEGIN
								WriteLn(gOutFile, tabStr, 'vpBaseOffset: ' : 20, Hex(@vpBaseOffset, 4));
								WriteLn(gOutFile, tabStr, 'vpRowBytes: ' : 20, Hex(@vpRowBytes, 2));
								WriteLn(gOutFile, tabStr, 'vpBounds (t,l,b,r): ' : 20, vpBounds.top : 0, ',', vpBounds.left : 0, ',', vpBounds.bottom : 0, ',', vpBounds.right : 0);

								Write(gOutFile, tabStr, 'vpVersion: ' : 20, vpVersion : 0);
								IF vpVersion = 4 THEN
									WriteLn(gOutFile,' = baseAddr32')
								ELSE
									WriteLn(gOutFile);

								WriteLn(gOutFile, tabStr, 'vpPackType: ' : 20, vpPackType : 0);
								WriteLn(gOutFile, tabStr, 'vpPackSize: ' : 20, vpPackSize : 0);
								WriteLn(gOutFile, tabStr, 'vpHRes: ' : 20, vpHRes / 65536.0 : 0 : 2);
								WriteLn(gOutFile, tabStr, 'vpVRes: ' : 20, vpVRes / 65536.0 : 0 : 2);

								Write(gOutFile, tabStr, 'vpPixelType: ' : 20, vpPixelType : 0);
								CASE Band(vpPixelType, $0F) OF
									0:
										Write(gOutFile,' = chunky');
									1:
										Write(gOutFile,' = chunkyPlanar');
									2:
										Write(gOutFile,' = planar');
								END;
								IF BTst(vpPixelType, 4) THEN
									WriteLn(gOutFile,', RGBDirect')
								ELSE
									WriteLn(gOutFile,', indexed');

								WriteLn(gOutFile, tabStr, 'vpPixelSize: ' : 20, vpPixelSize : 0);
								WriteLn(gOutFile, tabStr, 'vpCmpCount: ' : 20, vpCmpCount : 0);
								WriteLn(gOutFile, tabStr, 'vpCmpSize: ' : 20, vpCmpSize : 0);
								WriteLn(gOutFile, tabStr, 'vpPlaneBytes: ' : 20, Hex(@vpPlaneBytes, 4));
							END;
						HUnlock(miscDataHdl);
					END;
				mTable:
					BEGIN
						GetSBlock(dataAddr, miscDataHdl);
						HLock(miscDataHdl);
						longNum := GetHandleSize(miscDataHdl) + 4;
						Write(gOutFile,'block size = ', hex(@longNum, 1), ' ', hex(Ptr(Ord4(@longNum) + 1), 3));
						WITH CTabHandle(miscDataHdl)^^ DO
							WriteLn(gOutFile,'  ctSeed:', Hex(@ctSeed, 4), '  ctFlags: ', Hex(@ctFlags, 2), '  ctSize: ', ctSize : 0);
						HUnlock(miscDataHdl);
					END;
				mPageCnt:
					WriteLn(gOutFile,offset : 0);
				mDevType:
					BEGIN
						Write(gOutFile,offset : 0);
						CASE offset OF
							0:
								WriteLn(gOutFile,' = indexed CLUT device');
							1:
								WriteLn(gOutFile,' = indexed fixed device');
							2:
								WriteLn(gOutFile,' = direct device');
							OTHERWISE
								WriteLn(gOutFile,' = unknown device');
						END;
					END;
				endOfList:
					WriteLn(gOutFile);

			END; {CASE sRsrcId OF }
		END; { vidModeDir }


		PROCEDURE DoDrvrDir;
		BEGIN
			IF sRsrcId <> endOfList THEN
				WITH driverHeader DO
					BEGIN
						tabStr := Concat(GetTab(gLevel),ltab1);
						GetBytes(dataAddr, @driverHeader, SizeOF(DriverHeaderRec));
						WriteLn(gOutFile,'block size = ', hex(@blockSize, 1), ' ', hex(Ptr(Ord4(@blockSize) + 1), 3));
						SetCovered(dataAddr, Ord4(CalcAddr(dataAddr,blockSize))-Ord4(CalcAddr(dataAddr,0)));

						WriteLn(gOutFile, tabStr, 'drvrFlags: ' : 20, Hex(@drvrFlags, 2), '  8:dReadEnable=', ORD(BTst(drvrFlags, 8)) : 0,
						' 9:dWriteEnable=', ORD(BTst(drvrFlags, 9)) : 0, ' 10:dCtlEnable=', ORD(BTst(drvrFlags, 10)) : 0,
						' 11:dStatEnable=', ORD(BTst(drvrFlags, 11)) : 0, ' 12:dNeedGoodBye=', ORD(BTst(drvrFlags, 12)) : 0,
						' 13:dNeedTime=', ORD(BTst(drvrFlags, 13)) : 0, ' 14:dNeedLock=', ORD(BTst(drvrFlags, 14)) : 0);

						WriteLn(gOutFile, tabStr, 'drvrDelay: ' : 20, drvrDelay : 0);
						Write(gOutFile, tabStr, 'drvrEMask: ' : 20, Hex(@drvrEMask, 2), ' ');
						IF BTst(drvrEMask, 1) THEN
							Write(gOutFile,' 1:mouseDown');
						IF BTst(drvrEMask, 2) THEN
							Write(gOutFile,' 2:mouseUp');
						IF BTst(drvrEMask, 3) THEN
							Write(gOutFile,' 3:keyDown');
						IF BTst(drvrEMask, 4) THEN
							Write(gOutFile,' 4:keyUp');
						IF BTst(drvrEMask, 5) THEN
							Write(gOutFile,' 5:autoKey');
						IF BTst(drvrEMask, 6) THEN
							Write(gOutFile,' 6:update');
						IF BTst(drvrEMask, 7) THEN
							Write(gOutFile,' 7:disk');
						IF BTst(drvrEMask, 8) THEN
							Write(gOutFile,' 8:activate');
						IF BTst(drvrEMask, 10) THEN
							Write(gOutFile,' 10:network');
						IF BTst(drvrEMask, 11) THEN
							Write(gOutFile,' 11:driver');
						IF BTst(drvrEMask, 12) THEN
							Write(gOutFile,' 12:app1');
						IF BTst(drvrEMask, 13) THEN
							Write(gOutFile,' 13:app2');
						IF BTst(drvrEMask, 14) THEN
							Write(gOutFile,' 14:app3');
						IF BTst(drvrEMask, 15) THEN
							Write(gOutFile,' 15:app4');
						WriteLn(gOutFile);

						WriteLn(gOutFile, tabStr, 'drvrMenu: ' : 20, drvrMenu : 0);
						Write(gOutFile, tabStr, 'drvrOpen: ' : 20, Hex(@drvrOpen, 2));
						IF drvrOpen <> 0 THEN
							Write(gOutFile,' = ', HexLong(CalcAddr(dataAddr, 4 + drvrOpen)));
						WriteLn(gOutFile);

						Write(gOutFile, tabStr, 'drvrPrime: ' : 20, Hex(@drvrPrime, 2));
						IF drvrPrime <> 0 THEN
							Write(gOutFile,' = ', HexLong(CalcAddr(dataAddr, 4 + drvrPrime)));
						WriteLn(gOutFile);

						Write(gOutFile, tabStr, 'drvrCtl: ' : 20, Hex(@drvrCtl, 2));
						IF drvrCtl <> 0 THEN
							Write(gOutFile,' = ', HexLong(CalcAddr(dataAddr, 4 + drvrCtl)));
						WriteLn(gOutFile);

						Write(gOutFile, tabStr, 'drvrStatus: ' : 20, Hex(@drvrStatus, 2));
						IF drvrStatus <> 0 THEN
							Write(gOutFile,' = ', HexLong(CalcAddr(dataAddr, 4 + drvrStatus)));
						WriteLn(gOutFile);

						Write(gOutFile, tabStr, 'drvrClose: ' : 20, Hex(@drvrClose, 2));
						IF drvrClose <> 0 THEN
							Write(gOutFile,' = ', HexLong(CalcAddr(dataAddr, 4 + drvrClose)));
						WriteLn(gOutFile);

						WriteLn(gOutFile, tabStr, 'drvrName: “' : 21, drvrName, '”');
					END
			ELSE
				WriteLn(gOutFile);
		END; { drvrDir }


		PROCEDURE DoGammaDirOrVidNamesDir;
		BEGIN
			IF sRsrcId <> endOfList THEN
				BEGIN
					tabStr := Concat(GetTab(gLevel),ltab1);
					GetSBlock(dataAddr, miscDataHdl);
					HLock(miscDataHdl);
					longNum := GetHandleSize(miscDataHdl) + 4;
					Write(gOutFile,'block size = ', hex(@longNum, 1), ' ', hex(Ptr(Ord4(@longNum) + 1), 3), '  ID: ', Hex(miscDataHdl^, 2), '  name: “');
					WriteCString(Ptr(Ord4(miscDataHdl^) + 2));
					WriteLn(gOutFile,'”');
					IF whichDir = GammaDir THEN
						BEGIN
							longNum := BAnd(CStrLen(Ptr(Ord4(miscDataHdl^) + 2)), $ffffFFFE);
							WITH GammaTblPtr(PtrOf(ORD4(miscDataHdl^) + 2 + longNum))^ DO
								BEGIN
									WriteLn(gOutFile, tabStr, 'gVersion: ' : 20, gVersion : 0);
									WriteLn(gOutFile, tabStr, 'gType: ' : 20, gType : 0);
									WriteLn(gOutFile, tabStr, 'gFormulaSize: ' : 20, gFormulaSize : 0);
									WriteLn(gOutFile, tabStr, 'gChanCnt: ' : 20, gChanCnt : 0);
									WriteLn(gOutFile, tabStr, 'gDataCnt: ' : 20, gDataCnt : 0);
									WriteLn(gOutFile, tabStr, 'gDataWidth: ' : 20, gDataWidth : 0);
									WriteLn(gOutFile, tabStr, 'gFormulaData len: ' : 20, GetHandleSize(miscDataHdl) - SizeOf(GammaTbl) - longNum : 0);
								END;
						END;
					HUnlock(miscDataHdl);
				END
			ELSE
				WriteLn(gOutFile);
		END; { GammaDir, vidNamesDir }


		PROCEDURE DoUnknownSBlockDir;
		VAR
			i: Integer;
		BEGIN
			IF sRsrcId <> endOfList THEN
				BEGIN
					GetSBlock(dataAddr, miscDataHdl);
					HLock(miscDataHdl);
					longNum := GetHandleSize(miscDataHdl) + 4;
					Write(gOutFile,'block size = ', hex(@longNum, 1), ' ', hex(Ptr(Ord4(@longNum) + 1), 3), '  Data:');
					longNum := (longNum - 4) DIV 2 - 1;
					IF longNum > 15 THEN
						longNum := 15;
					FOR i := 0 TO longNum DO
						Write(gOutFile,' ', Hex(Ptr(Ord4(miscDataHdl^) + i + i), 2));
					WriteLn(gOutFile);
					HUnlock(miscDataHdl);
				END
			ELSE
				WriteLn(gOutFile);
		END; { unknownSBlockDir }


	BEGIN { WriteData }
		miscDataHdl := NIL;
		IF NOT findDirEntry(dataAddr) THEN
			BEGIN
				sRsrcId := Length(StringPtr(@id)^);
				IF (sRsrcId <> endOfList) & (whichDir IN [rsrcDirDirDir, rsrcDirDir, GammaDir, loadDir, drvrDir]) THEN
					AddDirListEntry(dataAddr, curStr);

				CASE whichDir OF
					rsrcDirDirDir, rsrcDirDir:
						DoRsrcDirDir;

					rsrcDir:
						DoRsrcDir;

					vidModeDir:
						DoVidModeDir;

					drvrDir:
						DoDrvrDir;

					loadDir:
						IF sRsrcId <> endOfList THEN
							WriteSExecBlock( GetTab(gLevel), dataAddr)
						ELSE
							WriteLn(gOutFile);

					vendorDir:
						IF sRsrcId <> endOfList THEN
							GetAndWriteCString(dataAddr, miscDataHdl)
						ELSE
							WriteLn(gOutFile);

					GammaDir, vidNamesDir:
						DoGammaDirOrVidNamesDir;

					unknownSBlockDir:
						DoUnknownSBlockDir;

				END; { CASE whichDir }

				IF miscDataHdl <> NIL THEN
					DisposeHandle(miscDataHdl);
			END; { IF NOT findDir }
	END; { WriteData }


PROCEDURE WriteSResourceDirectory (dirPtr: Ptr;
								whichDir: Integer;
								CONST curStr: Str255);
	VAR
		sRsrcId: SignedByte;
		offset: LongInt;
		i: Integer;
		s: Str255;
		resultAddr: Ptr;
	BEGIN
		gLevel := gLevel + 1;
		IF gLevel > 5 THEN
			BEGIN
				WriteLn( gOutFile, 'too many levels' );
				gLevel := gLevel - 1;
				Exit( WriteSResourceDirectory );
			END;

		s := GetTab(gLevel);

		i := 0;
		IF gLevel = 0 THEN
			BEGIN
				WriteLn(gOutFile,Hex(@dirPtr,4));
{				WriteLn(gOutFile,'        sResource Directory                |            sResource Fields        ');
				WriteLn(gOutFile,' #  sRsrcId  offset  result   note         |  #     id     offset  result   note');
}			END;
		REPEAT
			IF KeyMapPtr(KeyMapAddr)^[$3B] THEN
				debugger;
			REPEAT UNTIL NOT KeyMapPtr(KeyMapAddr)^[$39];
			GetRsrc(dirPtr, sRsrcID, offset);
			resultAddr := CalcAddr(dirPtr, offset);
			Write(gOutFile,s, i : 3, '    ', Hex(@sRsrcId, 1), '    ', Hex(Ptr(Ord4(@offset) + 1), 3), ' ', GetResult(sRsrcId, whichDir, resultAddr));
			WriteRsrcKind(sRsrcId, whichDir);
			IF (sRsrcId = endOfList) | KeyMapPtr(KeyMapAddr)^[$3A] THEN
				WriteLn
			ELSE
				WriteData(sRsrcId, offset, resultAddr, Concat(curStr, 'id:', Hex(@sRsrcId, 1), ' '), whichDir);


			dirPtr := CalcAddr(dirPtr,4);
			i := i + 1;
		UNTIL Length(StringPtr(@sRsrcId)^) = endOfList;
		gLevel := gLevel - 1;
	END; { WriteSResourceDirectory }


PROCEDURE WriteSInfoRecordAndFHeaderRec( slot : Integer; VAR headErr: OSErr; VAR sResDirFromInfo, sResDirFromHead: Ptr);
	VAR
		theSpBlock: spBlock;
		theSInfoRecPtr: SInfoRecPtr;
		theFHeaderRec: FHeaderRec;
		infoErr: OSErr;
		HeaderWhere: Ptr;
	BEGIN
		FillBytes( @theSpBlock, SIZEOF(theSPBlock ) );
		FillBytes( @theFHeaderRec, SIZEOF( theFHeaderRec ) );
		theSInfoRecPtr := NIL;

		theSpBlock.spSlot := slot;
		theSpBlock.spResult := LongInt(@theFHeaderRec);

		headErr := SReadFHeader(@theSpBlock);
		if (slot < $10) | (headErr <> smSlotOOBErr) THEN
			DoErr('SReadFHeader', headErr);

		infoErr := SFindSInfoRecPtr(@theSpBlock);
		if (slot < $10) | (infoErr <> smSlotOOBErr) THEN
			DoErr('sFindInfoRecPtr', infoErr);

		IF ( infoErr = noErr ) | ( kIgnoreError & ( theSInfoRecPtr <> NIL ) ) THEN
			BEGIN
				theSInfoRecPtr := SInfoRecPtr(theSpBlock.spResult);
				WriteLn(gOutFile);
				WriteLn(gOutFile,'SInfoRecord:');
				WriteSInfoRecord(theSInfoRecPtr, sResDirFromInfo);
			END;

		IF ( headErr = noErr ) | ( kIgnoreError & ( theFHeaderRec.fhLength <> 0 ) ) THEN
			BEGIN
				WriteLn(gOutFile);
				WriteLn(gOutFile,'Byte After ROM:');
				WriteLn(gOutFile,Hex(@gTopOfRom, 4));
				WriteLn(gOutFile);
				WriteLn(gOutFile,'FHeaderRec:');
				WriteFHeaderRec(@theFHeaderRec, sResDirFromHead, HeaderWhere);
			END;

		IF ( headErr = noErr ) THEN
			BEGIN
				gLevel := -1;

				FillBytes(gCoveredPtr, GetPtrSize(gCoveredPtr));
				SetCovered(HeaderWhere, Ord4(gTopOfRom)-Ord4(HeaderWhere));
			END;

	END; { WriteSInfoRecordAndFHeaderRec }

PROCEDURE InitLoopingGlobals;
BEGIN
			gMinAddr := NIL;
			gMaxAddr := NIL;
			SetHandleSize(gDirList, 0);
END;


PROCEDURE SlotsDumpMain;
	VAR
		slot: SignedByte;
		slotInt: Integer;
		headErr: OSErr;
		sResDirFromInfo: Ptr;
		sResDirFromHead: Ptr;

	BEGIN
		WriteTable;
		FOR slotInt := 0 TO 255 DO
			BEGIN
				slot := slotInt;
				InitLoopingGlobals;

				gDoCovering := TRUE;

				WriteLn(gOutFile);
				WriteLn(gOutFile,'-----------------------------------------');
				WriteLn(gOutFile,'slot ', Hex(@slot, 1), ':');

				WriteSInfoRecordAndFHeaderRec( slotInt, headErr, sResDirFromInfo, sResDirFromHead);

				IF ( headErr = noErr ) THEN
					BEGIN
						IF (slot = 0) & (gTopOfRom = Ptr($40900000)) & NOT button THEN
							BEGIN
								WriteLn(gOutFile);
								WriteLn(gOutFile,'Slot 0 Resource Directory of Resource Directories:');
								CheckAddrRange( Ptr($408F2C94) );
								WriteSResourceDirectory(Ptr($408F2C94), rsrcDirDirDir, Concat('(dir 0) Slot:', Hex(@slot, 1), ' '));
							END
						ELSE
							BEGIN
								WriteLn(gOutFile);
								WriteLn(gOutFile,'Slot Resource Directory from Info:');
								WriteSResourceDirectory(sResDirFromInfo, rsrcDirDir, Concat('Slot:', Hex(@slot, 1), ' '));
								IF sResDirFromInfo <> sResDirFromHead THEN
									BEGIN
										WriteLn(gOutFile);
										WriteLn(gOutFile,'Slot Resource Directory from Header:');
										WriteSResourceDirectory(sResDirFromHead, rsrcDirDir, Concat('(from header) Slot:', Hex(@slot, 1), ' '));
									END; { IF }
							END;

						WriteLn(gOutFile);

						CheckAddrRange( sResDirFromInfo );
						CheckAddrRange( sResDirFromHead );

						WriteLn(gOutFile,'minAddr = ', Hex(@gMinAddr, 4), '   maxAddr = ', Hex(@gMaxAddr, 4));
						WriteLn(gOutFile);
						WriteUncoveredBytes;
					END; { IF }
			END;
		gDoCovering := FALSE;
	END; { SlotsDumpMain }



PROCEDURE SlotsDumpSecond;
VAR
	theSpBlock: spBlock;
	err: OSErr;
	headErr: OSErr;
	sResDirFromInfo: Ptr;
	sResDirFromHead: Ptr;
	curSlot: SignedByte;


BEGIN
	FillBytes( @theSpBlock, SizeOF( theSPBlock ) );
	curSlot := -1;

	WITH theSpBlock DO 				{set required values in parameter block}
		BEGIN
			spParamData	:= BSL(1, fAll) + BSL( 1, fnext );	{fAll flag = 1: search all sResources}
			spSlot		:= 0;		{start search from slot 1}
			spID		:= 0;		{start search from sResource ID 1}
			spExtDev	:= 0;		{external device ID (card-specific)}

			err := noErr;

			WHILE err = noErr DO			{loop to search sResources}
				BEGIN
					InitLoopingGlobals;

					WriteLn(gOutFile);
					WriteLn(gOutFile,'-----------------------------------------');
					err := SGetSRsrc(@theSpBlock);
					DoErr( 'SGetSRsrc', err );

					IF err = noErr THEN
						BEGIN
							gSRsrcTypeData.category := spCategory;
							gSRsrcTypeData.cType := spCType;
							gSRsrcTypeData.DrSW := spDrvrSW;
							gSRsrcTypeData.DrHW := spDrvrHW;

							WriteLn( gOutFile, 'SpBlock: ');
							WriteLn( gOutFile, 'spsPointer: ' : 20, Hex(@spsPointer, 4) );
							WriteLn( gOutFile, 'spParamData: ' : 20, spParamData : 0, GetspParamDataEnabledDisabledString( spParamData ) );
							WriteLn( gOutFile, 'spRefNum: ' : 20, spRefNum : 0 );
							WriteLn( gOutFile, 'spCategory: ' : 20, categoryString(spCategory) );
							WriteLn( gOutFile, 'spCType: ' : 20, cTypeString(gSRsrcTypeData) );
							WriteLn( gOutFile, 'spDrvrSW: ' : 20, DrSWString(gSRsrcTypeData) );
							WriteLn( gOutFile, 'spDrvrHW: ' : 20, DrHWString(gSRsrcTypeData) );
							WriteLn( gOutFile, 'spSlot: ' : 20, Hex(@spSlot, 1) );
							WriteLn( gOutFile, 'spID: ' : 20, Hex(@spID, 1), GetRsrcKind( spID, rsrcDirDir )  );
							WriteLn( gOutFile, 'spExtDev: ' : 20, Hex(@spExtDev, 1) );
							WriteLn( gOutFile, 'spHWDev: ' : 20, Hex(@spHWDev, 1) );
							WriteLn( gOutFile );

							err := SRsrcInfo( @theSpBlock );
							DoErr( 'SRsrcInfo', err );
							IF err = noErr THEN
								BEGIN
									WriteLn( gOutFile, 'spIOReserved: ' : 20, Hex( @spIOReserved, 2 ) );

									IF spSlot <> curSlot THEN
										WriteSInfoRecordAndFHeaderRec( Length(StringPtr(@spSlot)^), headErr, sResDirFromInfo, sResDirFromHead);
									curSlot := spSlot;

									WriteLn( gOutFile );
									WriteLn(gOutFile,'Slot Resource Directory from SpBlock.spsPointer:');
									WriteSResourceDirectory( spsPointer, rsrcDir, Concat('from SpBlock.spsPointer '));
								END;
						END;
					spParamData	:= BSL(1, fAll) + BSL( 1, fnext );
				END;
		END; { WITH }

(*
	mainDev := GetMainDevice;
	myAuxDCEHandle := AuxDCEHandle( GetDCtlEntry(mainDev^^.gdRefNum) );
	WITH theSpBlock DO
		BEGIN
			spSlot := myAuxDCEHandle^^.dCtlSlot;
			spID := myAuxDCEHandle^^.dCtlSlotId;
			spExtDev := myAuxDCEHandle^^.dCtlExtDev;
			spHwDev := 0;
			spParamData := BSL( 1, foneslot );
			spTBMask := 3; { 3 }

			s := StringOf( spSlot, spExtDev, spID, spParamData );

			err := noerr;
			WHILE err = noErr DO
				BEGIN
					WriteLn( 'before ', s );
					err := SGetSRsrc(@thespBlock);
					WriteLn( 'after ', s, err );

					IF err = noErr THEN
						BEGIN
							WITH theSpBlock DO
								s := StringOf( spSlot, spExtDev, spID, spParamData );
							WriteLn( s );
							{ReadLn;}
							IF err <> noErr THEN
								WriteLn( err );
						END;
					spParamData := BSL( 1, fOneSlot ) + BSL( 1, fNext );
				END;
		END;
*)


	WriteLn( 'done' );
	ReadLn;
END; { SlotsDumpSecond }


PROCEDURE InitIconWindow;
	VAR
		bounds:		Rect;
		genevaID:	Integer;

	BEGIN
		bounds := qd.screenBits.bounds;
		bounds.top := 20;
		bounds.bottom := bounds.bottom DIV 2;
{		SetTextRect(bounds);}
{		ShowText;}
		bounds := qd.screenBits.bounds;
		bounds.top := bounds.bottom DIV 2;
		gWind := NewCWindow(NIL, bounds, '', TRUE, plainDBox, WindowPtr(-1), FALSE, 0);
		SetPort(gWind);
		GetFNum('Geneva', genevaID);
		TextFont(genevaID);
		TextMode(srcOr);
		TextFace([]);
		TextSize(9);
		gCurWidth := defaultWidth;
		gCurTop := defaultTop;
		gCurLeft := defaultLeft;
	END; { InitIconWindow }

BEGIN
	gDoCovering := FALSE;

	gDirList := NewHandle(0);

	gCoveredPtr := NewPtr(numToCover DIV 8);

	WriteLn(output,'-------------');


	PLFlush(output);

	ReWrite(gOutFile,'SlotsDumpOld.txt');

	MoveWindow(FrontWindow, 0, 20, FALSE);
	InitIconWindow;

	SlotsDumpMain;
	SlotsDumpSecond;

	DisposeWindow(gWind);
	ExitToShell;
END. { Main }
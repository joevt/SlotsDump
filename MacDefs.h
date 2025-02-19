
#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#else
	#warning missing pack/align pragmas
#endif

typedef int8_t *Ptr;

struct FHeaderRec {
	int32_t 						fhDirOffset;				/*offset to directory*/
	int32_t 						fhLength;					/*length of ROM*/
	int32_t							fhCRC;						/*CRC*/
	int8_t 							fhROMRev;					/*revision of ROM*/
	int8_t 							fhFormat;					/*format - 2*/
	int32_t							fhTstPat;					/*test pattern*/
	int8_t 							fhReserved;					/*reserved*/
	int8_t 							fhByteLanes;				/*ByteLanes*/
};
typedef struct FHeaderRec				FHeaderRec;
typedef FHeaderRec *					FHeaderRecPtr;

/*
   
   	Extended Format header block  -  extended declaration ROM format header for super sRsrc directories.	<H2><SM0>
   
*/


struct XFHeaderRec {
	int32_t 							fhXSuperInit;				/*Offset to SuperInit SExecBlock	<fhFormat,offset>*/
	int32_t 							fhXSDirOffset;				/*Offset to SuperDirectory			<$FE,offset>*/
	int32_t 							fhXEOL;						/*Psuedo end-of-list				<$FF,nil>*/
	int32_t 							fhXSTstPat;					/*TestPattern*/
	int32_t 							fhXDirOffset;				/*Offset to (minimal) directory*/
	int32_t 							fhXLength;					/*Length of ROM*/
	int32_t 							fhXCRC;						/*CRC*/
	int8_t 								fhXROMRev;					/*Revision of ROM*/
	int8_t 								fhXFormat;					/*Format-2*/
	int32_t 							fhXTstPat;					/*TestPattern*/
	int8_t 								fhXReserved;				/*Reserved*/
	int8_t 								fhXByteLanes;				/*ByteLanes*/
};
typedef struct XFHeaderRec				XFHeaderRec;
typedef XFHeaderRec *					XFHeaderRecPtr;


enum {
	appleFormat					= 1,							/*Format of Declaration Data (IEEE will assign real value)*/
	romRevision					= 1,							/*Revision of Declaration Data Format*/
	romRevRange					= 9,							/*Revision of Declaration Data Format [1..9]*/
	testPattern					= 1519594439L,					/*FHeader long word test pattern*/
	sCodeRev					= 2,							/*Revision of code (For sExec)*/
	sExec2						= 2,
	sCPU68000					= 1,							/*CPU type = 68000*/
	sCPU68020					= 2,							/*CPU type = 68020*/
	sCPU68030					= 3,							/*CPU type = 68030*/
	sCPU68040					= 4,							/*CPU type = 68040*/
	sMacOS68000					= 1,							/*Mac OS, CPU type = 68000*/
	sMacOS68020					= 2,							/*Mac OS, CPU type = 68020*/
	sMacOS68030					= 3,							/*Mac OS, CPU type = 68030*/
	sMacOS68040					= 4,							/*Mac OS, CPU type = 68040*/
	board						= 0,							/*Board sResource - Required on all boards*/
	displayVideoAppleTFB		= 16843009L,					/*Video with Apple parameters for TFB card.*/
	displayVideoAppleGM			= 16843010L,					/*Video with Apple parameters for GM card.*/
	networkEtherNetApple3Com	= 33620225L,					/*Ethernet with apple parameters for 3-Comm card.*/
	testSimpleAppleAny			= -2147417856L,					/*A simple test sResource.*/
	endOfList					= 255,							/*End of list*/
	defaultTO					= 100							/*100 retries.*/
};



enum {
																/* sResource flags for sRsrc_Flags */
	fOpenAtStart				= 1,							/* set => open the driver at start time, else do not */
	f32BitMode					= 2								/* set => a 32-bit address will be put into dctlDevBase (IM Devices 2-54) */
};


enum {
	sRsrcType					= 1,							/*Type of sResource*/
	sRsrcName					= 2,							/*Name of sResource*/
	sRsrcIcon					= 3,							/*Icon*/
	sRsrcDrvrDir				= 4,							/*Driver directory*/
	sRsrcLoadDir				= 5,							/*Load directory*/
	sRsrcBootRec				= 6,							/*sBoot record*/
	sRsrcFlags					= 7,							/*sResource Flags*/
	sRsrcHWDevId				= 8,							/*Hardware Device Id*/
	minorBaseOS					= 10,							/*Offset to base of sResource in minor space.*/
	minorLength					= 11,							/*Length of sResource’s address space in standard slot space.*/
	majorBaseOS					= 12,							/*Offset to base of sResource in Major space.*/
	majorLength					= 13,							/*Length of sResource in super slot space.*/
	sRsrcTest					= 14,							/*sBlock diagnostic code*/
	sRsrccicn					= 15,							/*Color icon*/
	sRsrcicl8					= 16,							/*8-bit (indexed) icon*/
	sRsrcicl4					= 17,							/*4-bit (indexed) icon*/
	sDRVRDir					= 16,							/*sDriver directory*/
	sGammaDir					= 64,							/*sGamma directory*/
	sRsrcVidNames				= 65,							/*Video mode name directory*/
	sRsrcDock					= 80,							/*spID for Docking Handlers*/
	sDiagRec					= 85,							/*spID for board diagnostics*/
	sVidAuxParams				= 123,							/*more video info for Display Manager -- timing information*/
	sDebugger					= 124,							/*DatLstEntry for debuggers indicating video anamolies*/
	sVidAttributes				= 125,							/*video attributes data field (optional,word)*/
	fLCDScreen					= 0,							/* bit 0 - when set is LCD, else is CRT*/
	fBuiltInDisplay				= 1,							/*	  1 - when set is built-in (in the box) display, else not*/
	fDefaultColor				= 2,							/*	  2 - when set display prefers multi-bit color, else gray*/
	fActiveBlack				= 3,							/*	  3 - when set black on display must be written, else display is naturally black*/
	fDimMinAt1					= 4,							/*	  4 - when set should dim backlight to level 1 instead of 0*/
	fBuiltInDetach				= 4,							/*	  4 - when set is built-in (in the box), but detaches*/
	sVidParmDir					= 126,
	sBkltParmDir				= 140,							/*directory of backlight tables*/
	stdBkltTblSize				= 36,							/*size of “standard” 0..31-entry backlight table*/
	sSuperDir					= 254
};

/* =======================================================================	*/
/* sResource types															*/
/* =======================================================================	*/

enum {
	catBoard					= 0x0001,						/*Category for board types.*/
	catTest						= 0x0002,						/*Category for test types -- not used much.*/
	catDisplay					= 0x0003,						/*Category for display (video) cards.*/
	catNetwork					= 0x0004,						/*Category for Networking cards.*/
	catScanner					= 0x0008,						/*scanners bring in data somehow*/
	catCPU						= 0x000A,
	catIntBus					= 0x000C,
	catProto					= 0x0011,
	catDock						= 0x0020,						/*Type*/
	typeBoard					= 0x0000,
	typeApple					= 0x0001,
	typeVideo					= 0x0001,
	typeEtherNet				= 0x0001,
	typeStation					= 0x0001,
	typeDesk					= 0x0002,
	typeTravel					= 0x0003,
	typeDSP						= 0x0004,
	typeXPT						= 0x000B,
	typeSIM						= 0x000C,
	typeDebugger				= 0x0100,
	type68000					= 0x0002,
	type68020					= 0x0003,
	type68030					= 0x0004,
	type68040					= 0x0005,
	type601						= 0x0025,
	type603						= 0x002E,
	typeAppleII					= 0x0015,						/*Driver Interface : <id.SW>*/
	drSwMacCPU					= 0,
	drSwAppleIIe				= 0x0001,
	drSwApple					= 1,							/*To ask for or define an Apple-compatible SW device.*/
	drSwMacsBug					= 0x0104,
	drSwDepewEngineering		= 0x0101,						/*Driver Interface : <id.SW><id.HW>*/
	drHwTFB						= 1,							/*HW ID for the TFB (original Mac II) video card.*/
	drHw3Com					= 1,							/*HW ID for the Apple EtherTalk card.*/
	drHwBSC						= 3,
	drHwGemini					= 1,
	drHwDeskBar					= 1,
	drHwHooperDock				= 2,							/*Hooper’s CatDock,TypeDesk,DrSwApple ID; registered with DTS.*/
	drHwATT3210					= 0x0001,
	drHwBootBug					= 0x0100,
	drHwMicroDock				= 0x0100,						/* video hardware id's  - <catDisplay><typVideo>*/
	drHwSTB3					= 0x0002,						/* Assigned by Kevin Mellander for STB-3 hardware. */
	drHwSTB						= drHwSTB3,						/* (Both STB-3 and STB-4 share the same video hardware.) */
	drHwRBV						= 0x0018,						/* IIci Aurora25/16 hw ID */
	drHwJMFB					= 0x0019,						/* 4•8/8•24 NuBus card */
	drHwElsie					= 0x001A,
	drHwTim						= 0x001B,
	drHwDAFB					= 0x001C,
	drHwDolphin					= 0x001D,						/* 8•24GC NuBus card */
	drHwGSC						= 0x001E,						/* (Renamed from GSC drHWDBLite) */
	drHwDAFBPDS					= 0x001F,
	drHWVSC						= 0x0020,
	drHwApollo					= 0x0021,
	drHwSonora					= 0x0022,
	drHwReserved2				= 0x0023,
	drHwColumbia				= 0x0024,
	drHwCivic					= 0x0025,
	drHwBrazil					= 0x0026,
	drHWPBLCD					= 0x0027,
	drHWCSC						= 0x0028,
	drHwJET						= 0x0029,
	drHWMEMCjr					= 0x002A,
	drHwBoogie					= 0x002B,						/* 8•24AC nuBus video card (built by Radius) */
	drHwHPV						= 0x002C,						/* High performance Video (HPV) PDS card for original PowerMacs */
	drHwPlanaria				= 0x002D,						/*PowerMac 6100/7100/8100 PDS AV video*/
	drHwValkyrie				= 0x002E,
	drHwKeystone				= 0x002F,
	drHWATI						= 0x0055,
	drHwGammaFormula			= 0x0056,						/* Use for gType of display mgr gamma tables */
																/* other drHW id's for built-in functions*/
	drHwSonic					= 0x0110,
	drHwMace					= 0x0114,
	drHwDblExp					= 0x0001,						/* CPU board IDs - <catBoard> <typBoard> <0000> <0000>*/
	MIIBoardId					= 0x0010,						/*Mac II Board ID*/
	ciVidBoardID				= 0x001F,						/*Aurora25 board ID*/
	CX16VidBoardID				= 0x0020,						/*Aurora16 board ID*/
	MIIxBoardId					= 0x0021,						/*Mac IIx Board ID*/
	SE30BoardID					= 0x0022,						/*Mac SE/30 Board ID*/
	MIIcxBoardId				= 0x0023,						/*Mac IIcx Board ID*/
	MIIfxBoardId				= 0x0024,						/*F19 board ID*/
	EricksonBoardID				= 0x0028,
	ElsieBoardID				= 0x0029,
	TIMBoardID					= 0x002A,
	EclipseBoardID				= 0x002B,
	SpikeBoardID				= 0x0033,
	DBLiteBoardID				= 0x0035,
	ZydecoBrdID					= 0x0036,
	ApolloBoardID				= 0x0038,
	PDMBrdID					= 0x0039,
	VailBoardID					= 0x003A,
	WombatBrdID					= 0x003B,
	ColumbiaBrdID				= 0x003C,
	CycloneBrdID				= 0x003D,
	CompanionBrdID				= 0x003E,
	DartanianBoardID			= 0x0040,
	DartExtVidBoardID			= 0x0046,
	HookBoardID					= 0x0047,						/*Hook internal video board ID*/
	EscherBoardID				= 0x004A,						/*Board ID for Escher (CSC)*/
	POBoardID					= 0x004D,						/*Board ID for Primus/Optimus/Aladdin*/
	TempestBrdID				= 0x0050,						/*Non-official Board ID for Tempest*/
	BlackBirdBdID				= 0x0058,						/*Board ID for BlackBird*/
	BBExtVidBdID				= 0x0059,						/*Board ID for BlackBird built-in external video*/
	YeagerBoardID				= 0x005A,						/*Board ID for Yeager*/
	BBEtherNetBdID				= 0x005E,						/*Board ID for BlackBird Ethernet board*/
	TELLBoardID					= 0x0065,						/*Board ID for TELL (Valkyrie)*/
	MalcolmBoardID				= 0x065E,						/*Board ID for Malcolm*/
	AJBoardID					= 0x065F,						/*Board ID for AJ*/
	M2BoardID					= 0x0660,						/*Board ID for M2*/
	OmegaBoardID				= 0x0661,						/*Board ID for Omega*/
	TNTBoardID					= 0x0670,						/*Board ID for TNT/Alchemy/Hipclipper CPUs (did Nano just make this up?)*/
	HooperBoardID				= 0x06CD,						/*Board ID for Hooper*/
																/* other board IDs*/
	BoardIDDblExp				= 0x002F,
	DAFBPDSBoardID				= 0x0037,
	MonetBoardID				= 0x0048,
	SacSONIC16BoardID			= 0x004E,
	SacSONIC32BoardID			= 0x004F,						/* CPU board types - <CatCPU> <Typ680x0> <DrSwMacCPU>*/
	drHWMacII					= 0x0001,						/*Mac II hw ID*/
	drHwMacIIx					= 0x0002,						/*Mac IIx hw ID*/
	drHWSE30					= 0x0003,						/*Mac SE/30 hw ID*/
	drHwMacIIcx					= 0x0004,						/*Mac IIcx hw ID*/
	drHWMacIIfx					= 0x0005,						/*Mac IIfx hw ID*/
	drHWF19						= 0x0005,						/*F19 hw ID*/
	sBlockTransferInfo			= 20,							/*general slot block xfer info*/
	sMaxLockedTransferCount		= 21,							/*slot max. locked xfer count*/
	boardId						= 32,							/*Board Id*/
	pRAMInitData				= 33,							/*sPRAM init data*/
	primaryInit					= 34,							/*Primary init record*/
	timeOutConst				= 35,							/*Time out constant*/
	vendorInfo					= 36,							/*Vendor information List. See Vendor List, below*/
	boardFlags					= 37,							/*Board Flags*/
	secondaryInit				= 38,							/*Secondary init record/code*/
																/* The following Id's are associated with all CPU sResources.*/
	MajRAMSp					= 129,							/*ID of Major RAM space.*/
	MinROMSp					= 130,							/*ID of Minor ROM space.*/
	vendorId					= 1,							/*Vendor Id*/
	serialNum					= 2,							/*Serial number*/
	revLevel					= 3,							/*Revision level*/
	partNum						= 4,							/*Part number*/
	date						= 5								/*Last revision date of the card*/
};

enum {
	mBaseOffset					= 1,							/*Id of mBaseOffset.*/
	mRowBytes					= 2,							/*Video sResource parameter Id's */
	mBounds						= 3,							/*Video sResource parameter Id's */
	mVersion					= 4,							/*Video sResource parameter Id's */
	mHRes						= 5,							/*Video sResource parameter Id's */
	mVRes						= 6,							/*Video sResource parameter Id's */
	mPixelType					= 7,							/*Video sResource parameter Id's */
	mPixelSize					= 8,							/*Video sResource parameter Id's */
	mCmpCount					= 9,							/*Video sResource parameter Id's */
	mCmpSize					= 10,							/*Video sResource parameter Id's */
	mPlaneBytes					= 11,							/*Video sResource parameter Id's */
	mVertRefRate				= 14,							/*Video sResource parameter Id's */
	mVidParams					= 1,							/*Video parameter block id.*/
	mTable						= 2,							/*Offset to the table.*/
	mPageCnt					= 3,							/*Number of pages*/
	mDevType					= 4,							/*Device Type*/
	oneBitMode					= 128,							/*Id of OneBitMode Parameter list.*/
	twoBitMode					= 129,							/*Id of TwoBitMode Parameter list.*/
	fourBitMode					= 130,							/*Id of FourBitMode Parameter list.*/
	eightBitMode				= 131							/*Id of EightBitMode Parameter list.*/
};

enum {
	sixteenBitMode				= 132,							/*Id of SixteenBitMode Parameter list.*/
	thirtyTwoBitMode			= 133,							/*Id of ThirtyTwoBitMode Parameter list.*/
	firstVidMode				= 128,							/*The new, better way to do the above. */
	secondVidMode				= 129,							/* QuickDraw only supports six video */
	thirdVidMode				= 130,							/* at this time.      */
	fourthVidMode				= 131,
	fifthVidMode				= 132,
	sixthVidMode				= 133,
	spGammaDir					= 64,
	spVidNamesDir				= 65
};


typedef struct {
	int16_t top;
	int16_t left;
	int16_t bottom;
	int16_t right;
} Rect;

struct VPBlock {
	int32_t 						vpBaseOffset;				/*Offset to page zero of video RAM (From minorBaseOS).*/
	int16_t							vpRowBytes;					/*Width of each row of video memory.*/
	Rect 							vpBounds;					/*BoundsRect for the video display (gives dimensions).*/
	int16_t							vpVersion;					/*PixelMap version number.*/
	int16_t							vpPackType;
	int32_t							vpPackSize;
	int32_t							vpHRes;						/*Horizontal resolution of the device (pixels per inch).*/
	int32_t							vpVRes;						/*Vertical resolution of the device (pixels per inch).*/
	int16_t							vpPixelType;				/*Defines the pixel type.*/
	int16_t							vpPixelSize;				/*Number of bits in pixel.*/
	int16_t							vpCmpCount;					/*Number of components in pixel.*/
	int16_t							vpCmpSize;					/*Number of bits per component*/
	int32_t							vpPlaneBytes;				/*Offset from one plane to the next.*/
};
typedef struct VPBlock					VPBlock;
typedef VPBlock *						VPBlockPtr;

struct RGBColor {
	uint16_t 					red;						/*magnitude of red component*/
	uint16_t 					green;						/*magnitude of green component*/
	uint16_t 					blue;						/*magnitude of blue component*/
};
typedef struct RGBColor					RGBColor;

struct ColorSpec {
	int16_t 						value;						/*index or other value*/
	RGBColor 						rgb;						/*true color*/
};
typedef struct ColorSpec				ColorSpec;
typedef ColorSpec *						ColorSpecPtr;

typedef ColorSpec 						CSpecArray[1];

struct ColorTable {
	int32_t							ctSeed;						/*unique identifier for table*/
	int16_t							ctFlags;					/*high bit: 0 = PixMap; 1 = device*/
	int16_t							ctSize;						/*number of entries in CTTable*/
	CSpecArray 						ctTable;					/*array [0..0] of ColorSpec*/
};
typedef struct ColorTable				ColorTable;

typedef ColorTable *					CTabPtr;
typedef CTabPtr *						CTabHandle;

struct GammaTbl {
	int16_t 							gVersion;					/*gamma version number*/
	int16_t 							gType;						/*gamma data type*/
	int16_t 							gFormulaSize;				/*Formula data size*/
	int16_t 							gChanCnt;					/*number of channels of data*/
	int16_t 							gDataCnt;					/*number of values/channel*/
	int16_t 							gDataWidth;					/*bits/corrected value (data packed to next larger byte size)*/
	int16_t 							gFormulaData[1];			/*data for formulas followed by gamma values*/
};
typedef struct GammaTbl					GammaTbl;
typedef GammaTbl *						GammaTblPtr;

enum {
	timingInvalid				= 0,							/*	Unknown timing… force user to confirm. */
	timingInvalid_SM_T24		= 8,							/*	Work around bug in SM Thunder24 card.*/
	timingApple_FixedRateLCD	= 42,							/*	Lump all fixed-rate LCDs into one category.*/
	timingApple_512x384_60hz	= 130,							/*  512x384  (60 Hz) Rubik timing. */
	timingApple_560x384_60hz	= 135,							/*  560x384  (60 Hz) Rubik-560 timing. */
	timingApple_640x480_67hz	= 140,							/*  640x480  (67 Hz) HR timing. */
	timingApple_640x400_67hz	= 145,							/*  640x400  (67 Hz) HR-400 timing. */
	timingVESA_640x480_60hz		= 150,							/*  640x480  (60 Hz) VGA timing. */
	timingVESA_640x480_72hz		= 152,							/*  640x480  (72 Hz) VGA timing. */
	timingVESA_640x480_75hz		= 154,							/*  640x480  (75 Hz) VGA timing. */
	timingVESA_640x480_85hz		= 158,							/*  640x480  (85 Hz) VGA timing. */
	timingGTF_640x480_120hz		= 159,							/*  640x480  (120 Hz) VESA Generalized Timing Formula */
	timingApple_640x870_75hz	= 160,							/*  640x870  (75 Hz) FPD timing.*/
	timingApple_640x818_75hz	= 165,							/*  640x818  (75 Hz) FPD-818 timing.*/
	timingApple_832x624_75hz	= 170,							/*  832x624  (75 Hz) GoldFish timing.*/
	timingVESA_800x600_56hz		= 180,							/*  800x600  (56 Hz) SVGA timing. */
	timingVESA_800x600_60hz		= 182,							/*  800x600  (60 Hz) SVGA timing. */
	timingVESA_800x600_72hz		= 184,							/*  800x600  (72 Hz) SVGA timing. */
	timingVESA_800x600_75hz		= 186,							/*  800x600  (75 Hz) SVGA timing. */
	timingVESA_800x600_85hz		= 188,							/*  800x600  (85 Hz) SVGA timing. */
	timingVESA_1024x768_60hz	= 190,							/* 1024x768  (60 Hz) VESA 1K-60Hz timing. */
	timingVESA_1024x768_70hz	= 200,							/* 1024x768  (70 Hz) VESA 1K-70Hz timing. */
	timingVESA_1024x768_75hz	= 204,							/* 1024x768  (75 Hz) VESA 1K-75Hz timing (very similar to timingApple_1024x768_75hz). */
	timingVESA_1024x768_85hz	= 208,							/* 1024x768  (85 Hz) VESA timing. */
	timingApple_1024x768_75hz	= 210,							/* 1024x768  (75 Hz) Apple 19" RGB. */
	timingApple_1152x870_75hz	= 220,							/* 1152x870  (75 Hz) Apple 21" RGB. */
	timingAppleNTSC_ST			= 230,							/*  512x384  (60 Hz, interlaced, non-convolved). */
	timingAppleNTSC_FF			= 232,							/*  640x480  (60 Hz, interlaced, non-convolved). */
	timingAppleNTSC_STconv		= 234,							/*  512x384  (60 Hz, interlaced, convolved). */
	timingAppleNTSC_FFconv		= 236,							/*  640x480  (60 Hz, interlaced, convolved). */
	timingApplePAL_ST			= 238,							/*  640x480  (50 Hz, interlaced, non-convolved). */
	timingApplePAL_FF			= 240,							/*  768x576  (50 Hz, interlaced, non-convolved). */
	timingApplePAL_STconv		= 242,							/*  640x480  (50 Hz, interlaced, convolved). */
	timingApplePAL_FFconv		= 244,							/*  768x576  (50 Hz, interlaced, convolved). */
	timingVESA_1280x960_75hz	= 250,							/* 1280x960  (75 Hz) */
	timingVESA_1280x960_60hz	= 252,							/* 1280x960  (60 Hz) */
	timingVESA_1280x960_85hz	= 254,							/* 1280x960  (85 Hz) */
	timingVESA_1280x1024_60hz	= 260,							/* 1280x1024 (60 Hz) */
	timingVESA_1280x1024_75hz	= 262,							/* 1280x1024 (75 Hz) */
	timingVESA_1280x1024_85hz	= 268,							/* 1280x1024 (85 Hz) */
	timingVESA_1600x1200_60hz	= 280,							/* 1600x1200 (60 Hz) VESA timing. */
	timingVESA_1600x1200_65hz	= 282,							/* 1600x1200 (65 Hz) VESA timing. */
	timingVESA_1600x1200_70hz	= 284,							/* 1600x1200 (70 Hz) VESA timing. */
	timingVESA_1600x1200_75hz	= 286,							/* 1600x1200 (75 Hz) VESA timing (pixel clock is 189.2 Mhz dot clock). */
	timingVESA_1600x1200_80hz	= 288,							/* 1600x1200 (80 Hz) VESA timing (pixel clock is 216>? Mhz dot clock) - proposed only. */
	timingVESA_1600x1200_85hz	= 289,							/* 1600x1200 (85 Hz) VESA timing (pixel clock is 229.5 Mhz dot clock). */
	timingSMPTE240M_60hz		= 400,							/* 60Hz V, 33.75KHz H, interlaced timing, 16:9 aspect, typical resolution of 1920x1035. */
	timingFilmRate_48hz			= 410,							/* 48Hz V, 25.20KHz H, non-interlaced timing, typical resolution of 640x480. */
	timingSony_1600x1024_76hz	= 500,							/* 1600x1024 (76 Hz) Sony timing (pixel clock is 170.447 Mhz dot clock). */
	timingSony_1920x1080_60hz	= 510,							/* 1920x1080 (60 Hz) Sony timing (pixel clock is 159.84 Mhz dot clock). */
	timingSony_1920x1080_72hz	= 520,							/* 1920x1080 (72 Hz) Sony timing (pixel clock is 216.023 Mhz dot clock). */
	timingSony_1900x1200_74hz	= 530,							/* 1900x1200 (74 Hz) Sony timing (pixel clock is 236.25 Mhz dot clock). */
	timingSony_1900x1200_76hz	= 540							/* 1900x1200 (76 Hz) Sony timing (pixel clock is 245.48 Mhz dot clock). */
};

inline void BitSet(int8_t *bits, uint32_t bit) {
	bits[bit >> 3] |= (1 << (bit & 7));
}

inline bool BitTst(int8_t *bits, uint32_t bit) {
	return bits[bit >> 3] & (1 << (bit & 7));
}

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif

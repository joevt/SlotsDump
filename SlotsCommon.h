#define __STDC_FORMAT_MACROS

#include <ConditionalMacros.h>

#include <string>
#include <cinttypes>
#ifdef __SLOTSDUMP__
}
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef __SLOTSDUMP__
#include "Types.h"
#include "QuickDraw.h"
#include "TextUtils.h"
#include "ToolUtils.h"
#include "Fonts.h"
#include "Processes.h"
#include "Devices.h"
#include "Slots.h"
#include "Video.h"
#include "Icons.h"
#include "ROMDefs.h"
#include "Windows.h"
#endif

#ifdef __SLOTSPARSE__
#include "MacDefs.h"
#endif


/* read an unaligned big-endian WORD (16bit) */
#define READ_WORD_BE_U( addr) ((((uint8_t*)(addr))[0] << 8) | ((uint8_t*)(addr))[1])

/* read an unaligned big-endian DWORD (32bit) */
#define READ_DWORD_BE_U(addr)                                                  \
    ((((uint8_t*)(addr))[0] << 24) | (((uint8_t*)(addr))[1] << 16) |           \
     (((uint8_t*)(addr))[2] <<  8) |  ((uint8_t*)(addr))[3]         )

#if __LITTLE_ENDIAN__
#define BE_TO_HOST_16(x) x = READ_WORD_BE_U(&x)
#define BE_TO_HOST_32(x) x = READ_DWORD_BE_U(&x)
#else
#define BE_TO_HOST_16(x)
#define BE_TO_HOST_32(x)
#endif

#define kOpenQuote "\""
#define kCloseQuote "\""
#define kApostrophe "'"


const bool kIgnoreError = true;

const char ltab1[] = "                                        ";

typedef enum {
	rootDir,
		superDir,
			rsrcUnknownDir, // child -> rsrcDir
		rsrcDirDir,
			rsrcDir,
				drvrDir,
				loadDir,
				vendorDir,
				vidNamesDir,
				gammaDir,
				vidModeDir,
				vidAuxParamsDir,
				vidParmDir
} DirType;

// sResource: sBlockTransferInfo, mBlockTransferInfo bit defines
#define fIsMaster			31
#define fMstrLockedXfer		30
#define fMstrXferSz16		19
#define fMstrXferSz8 		18
#define fMstrXferSz4		17
#define fMstrXferSz2		16
#define fIsSlave			15
#define fSlvXferSz16		3
#define fSlvXferSz8 		2
#define fSlvXferSz4			1
#define fSlvXferSz2			0

#define CPU_FlagsID                      0x80
#define hasPixelClock                    0               // bit indicating that card provides a pixel clock
#define hasIWM                           1               // card has an IWM
#define hasJoystick                      2               // card has a joystick port
#define hasDERegs                        15              // card uses the initial DE register set


typedef struct {
	int16_t category, cType, DrSW, DrHW;
} sRsrcTypeRec;

typedef struct {
	uint32_t blockSize;
	uint16_t boardID;
	uint8_t  vendorUse1;
	uint8_t  vendorUse2;
	uint8_t  vendorUse3;
	uint8_t  vendorUse4;
	uint8_t  vendorUse5;
	uint8_t  vendorUse6;
} SPRAMRecord;

typedef struct {
	uint32_t blockSize;
	int8_t revisionLevel;
	int8_t cpuID;
	int16_t reserved;
	int32_t codeOffset;
} sExecBlock;

typedef struct {
	Ptr  addr;
	char name[100];
} dirListEntry;
typedef dirListEntry* dirListEntryPtr;


typedef struct {
	uint32_t blockSize;
	int16_t drvrFlags, drvrDelay, drvrEMask, drvrMenu, drvrOpen, drvrPrime, drvrCtl, drvrStatus, drvrClose;
	char drvrName[255];
} DriverHeaderRec;


extern dirListEntryPtr gDirList;
extern int32_t gDirListSize;

extern Ptr gTopOfRom;
extern Ptr gStartOfRom;
extern size_t gAddrAfterRom;
extern int8_t gByteLanes;	// 0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F

extern sRsrcTypeRec gSRsrcTypeData;               /* current sRsrc type */

extern Ptr gMinAddr, gMaxAddr;

extern FILE * gOutFile;

extern int32_t numToCover;
extern Ptr gCoveredPtr;
extern bool gDoCovering;

extern int16_t gLevel;


std::string GetTab(int i);
void SetCovered(void * p, int32_t len);
void WriteCString(char *p);
std::string Hex(const void *hexPtr, int numBytes);
void WriteChars(uint8_t *p, int32_t numChars);
void WriteUncoveredBytes();
int32_t Get3(int32_t n);
bool CheckDataAddr(void * dataAddr);
void GetBytes(void* source, void* dest, int32_t numBytes, bool doSetCovered = true);
uint32_t CalcChecksum(Ptr start, int32_t len);
void RsrcToIdAndOffset(int32_t rsrc, uint8_t &s, int32_t &offset);
void GetRsrc(Ptr p, uint8_t &s, int32_t &offset);
int32_t GetSBlock(Ptr source, Ptr &dest);
void GetCString(Ptr source, Ptr &dest);
void WriteByteLanes(int8_t theByteLanes);
void WriteByteLanes2(uint8_t theByteLanes);
void WriteFHeaderRec(FHeaderRec &fh, Ptr &sResDirFromHead, Ptr headerWhere);
void WriteXFHeaderRec(XFHeaderRec &xfh, Ptr &sRootDirFromXHead, Ptr xHeaderWhere);
void WriteTable();
const char * GetRsrcKind(uint8_t sRsrcId, DirType whichDir);
void WriteRsrcKind(uint8_t id, DirType whichDir);
const char * GetspParamDataEnabledDisabledString(int32_t spParamData);
std::string CategoryString(int16_t category);
std::string cTypeString(sRsrcTypeRec &theSRsrcTypeData);
std::string DrSWString(sRsrcTypeRec &theSRsrcTypeData);
std::string DrHWString(sRsrcTypeRec &theSRsrcTypeData);
std::string GetSRsrcTypeDataString(sRsrcTypeRec &theSRsrcTypeData);
const char* BoardIDString(uint16_t boardID);
void WriteSExecBlock(std::string tabStr, Ptr dataAddr);
void WriteSResourceDirectory(Ptr dirPtr, DirType whichDir, const std::string curStr);
void WritePRAMInitData(Ptr dataAddr);
void WriteSBlock(Ptr dataAddr, Ptr &miscData, const char *description);
bool FindDirEntry(Ptr findAddr);
void AddDirListEntry(Ptr newAddr, const char *newName);
void CheckAddrRange(Ptr addr);
bool GetAddrResult(uint8_t sRsrcId, DirType whichDir, Ptr addr, char *buf);
void GetAndWriteCString(Ptr dataAddr, Ptr &miscData);
void DoRsrcDirDir(uint8_t sRsrcId, DirType whichDir, Ptr dataAddr, const std::string curStr);
void DoRsrcDir(uint8_t sRsrcId, int32_t offset, Ptr dataAddr, Ptr &miscData, const std::string curStr);
void DoVidModeDir(uint8_t sRsrcId, int32_t offset, Ptr dataAddr, Ptr &miscData);
void DoDrvrDir(uint8_t sRsrcId, Ptr dataAddr);
void DoGammaDirOrVidNamesDir(uint8_t sRsrcId, DirType whichDir, Ptr dataAddr, Ptr &miscData);
void DoVidParmDir(uint8_t sRsrcId, Ptr dataAddr, Ptr &miscData);
void DoVidAuxParamsDir(uint8_t sRsrcId, Ptr dataAddr);
void WriteData (
	uint8_t sRsrcId,
	int32_t offset,
	Ptr dataAddr,
	const std::string curStr,
	DirType whichDir
);
void WriteSResourceDirectory(Ptr dirPtr, DirType whichDir, const std::string curStr);
void InitLoopingGlobals();

size_t OutAddr(void *base);
Ptr CalcAddr(void* base, int32_t offset);
void WriteIcon(Ptr iconp, const char * title);
void WriteIcl(void * icon, int16_t depth, const char * title);
void WriteCIcon(Ptr miscData, const char * title);

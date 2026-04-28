#include <dolphin.h>
#include <dolphin/os.h>
#include <dolphin/dvd.h>

#include <dolphin/internal/__dvd.h>

// .bss
static u8 bb2Buf[63]; // size: 0x3F, address: 0x0

// .sbss
static u32 status; // size: 0x4, address: 0x0
static struct DVDBB2 * bb2; // size: 0x4, address: 0x4
static struct DVDDiskID * idTmp; // size: 0x4, address: 0x8

// functions
static void cb(s32 result, DVDCommandBlock * block);
void __fstLoad();

static void cb(s32 result, DVDCommandBlock * block) {
    if (result > 0) {
        switch(status) {
            case 0:
                status = 1;
                DVDReadAbsAsyncForBS(block, bb2, 0x20, 0x420, cb);
                return;
            case 1:
                status = 2;
                DVDReadAbsAsyncForBS(block, bb2->FSTAddress, (bb2->FSTLength + 0x1F) & 0xFFFFFFE0, bb2->FSTPosition, cb);
            default:
                return;
        }
    }
    if (result == -1) {
        return;
    } else if (result == -4) {
        status = 0;
        DVDReset();
        DVDReadDiskID(block, idTmp, cb);
    }
}

void __fstLoad() {
    struct OSBootInfo_s * bootInfo;
    DVDDiskID * id;
    u8 idTmpBuf[63];
    s32 state;
    void * arenaHi;
    static DVDCommandBlock block;

    arenaHi = OSGetArenaHi();
    bootInfo = (void*)OSPhysicalToCached(0);
    idTmp = (void*)OSRoundUp32B(idTmpBuf);
    bb2 = (void*)OSRoundUp32B(bb2Buf);
    DVDReset();
    DVDReadDiskID(&block, idTmp, cb);
    while (1) {
        state = DVDGetDriveStatus();
        if (state == 0) {
            break;
        }

        // weird switch that seemingly wont do anything but break out of its own switch. What was this for? Early DVD development pre-hardware?
        switch(state) {
            case DVD_STATE_FATAL_ERROR: break;
            case DVD_STATE_BUSY: break;
            case DVD_STATE_WAITING: break;
            case DVD_STATE_COVER_CLOSED: break;
            case DVD_STATE_NO_DISK: break;
            case DVD_STATE_COVER_OPEN: break;
            case DVD_STATE_MOTOR_STOPPED: break;
        }
    }
    bootInfo->FSTLocation = (void*)bb2->FSTAddress;
    bootInfo->FSTMaxLength = bb2->FSTMaxLength;
    id = &bootInfo->DVDDiskID;
    memcpy(id, idTmp, 0x20);
    OSReport("\n");
    //TODO: Are gameName and company u8 arrays in this version?
    OSReport("  Game Name ... %c%c%c%c\n", (u8)id->gameName[0], (u8)id->gameName[1], (u8)id->gameName[2], (u8)id->gameName[3]);
    OSReport("  Company ..... %c%c\n", (u8)id->company[0], (u8)id->company[1]);
    OSReport("  Disk # ...... %d\n", id->diskNumber);
    OSReport("  Game ver .... %d\n", id->gameVersion);
    OSReport("  Streaming ... %s\n", !(id->streaming) ? "OFF" : "ON");
    OSReport("\n");
    OSSetArenaHi(bb2->FSTAddress);
}

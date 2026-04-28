#ifndef _DOLPHIN_DVD_INTERNAL_H_
#define _DOLPHIN_DVD_INTERNAL_H_

#include <dolphin/dvd.h>

// dvd.c
void __DVDAudioBufferConfig(struct DVDCommandBlock * block, u32 enable, u32 size, void (* callback)(long, struct DVDCommandBlock *));
void __DVDPrepareResetAsync(DVDCBCallback callbac);

// dvdfs.c
extern struct OSThreadQueue __DVDThreadQueue;
extern unsigned long __DVDLongFileNameFlag;

void __DVDFSInit();

// dvdlow.c
void __DVDInterruptHandler(short unused, struct OSContext * context);

// dvdqueue.c
void __DVDClearWaitingQueue();
int __DVDPushWaitingQueue(s32 prio, DVDCommandBlock * block);
struct DVDCommandBlock * __DVDPopWaitingQueue();
BOOL __DVDCheckWaitingQueue();
BOOL __DVDDequeueWaitingQueue(DVDCommandBlock * block);
BOOL __DVDIsBlockInWaitingQueue(DVDCommandBlock * block);

//dvderror.c
void __DVDStoreErrorCode(u32 error);

// fstload.c
void __fstLoad();

#endif // _DOLPHIN_DVD_INTERNAL_H_

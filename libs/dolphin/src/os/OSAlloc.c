#include <dolphin.h>
#include <dolphin/os.h>

#define ALIGNMENT 32

#define InRange(cell, arenaStart, arenaEnd) \
    ((u32) arenaStart <= (u32) cell) && ((u32) cell < (u32) arenaEnd)

#define HEADERSIZE 32u
#define MINOBJSIZE 64u

#ifdef DEBUG
#define ENABLE_HEAPDESC
#endif

struct Cell {
    struct Cell * prev;
    struct Cell * next;
    long size;
#ifdef ENABLE_HEAPDESC
    struct HeapDesc * hd;
    long requested;
#endif
};

struct HeapDesc {
    long size;
    struct Cell * free;
    struct Cell * allocated;
#ifdef ENABLE_HEAPDESC
    unsigned long paddingBytes;
    unsigned long headerBytes;
    unsigned long payloadBytes;
#endif
};

volatile int __OSCurrHeap = -1;

static struct HeapDesc * HeapArray;
static int NumHeaps;
static void * ArenaStart;
static void * ArenaEnd;

// functions
static struct Cell * DLAddFront(struct Cell * list, struct Cell * cell);
static struct Cell * DLLookup(struct Cell * list, struct Cell * cell);
static struct Cell * DLExtract(struct Cell * list, struct Cell * cell);
static struct Cell * DLInsert(struct Cell * list, struct Cell * cell);
static int DLOverlap(struct Cell * list, void * start, void * end);
static long DLSize(struct Cell * list);

static struct Cell * DLAddFront(struct Cell * list, struct Cell * cell) {
    cell->next = list;
    cell->prev = 0;
    if (list) {
        list->prev = cell;
    }
    return cell;
}

static struct Cell * DLLookup(struct Cell * list, struct Cell * cell) {
    for(; list; list = list->next) {
        if (list == cell) {
            return list;
        }
    }
    return NULL;
}

static struct Cell * DLExtract(struct Cell * list, struct Cell * cell) {
    if (cell->next) {
        cell->next->prev = cell->prev;
    }
    if (cell->prev == NULL) {
        return cell->next;
    }
    cell->prev->next = cell->next;
    return list;
}

static struct Cell * DLInsert(struct Cell * list, struct Cell * cell) {
    struct Cell * prev;
    struct Cell * next;

    for(next = list, prev = NULL; next != 0; prev = next, next = next->next) {
        if (cell <= next) {
            break;
        }
    }

    cell->next = next;
    cell->prev = prev;
    if (next) {
        next->prev = cell;
        if ((u8*)cell + cell->size == (u8*)next) {
            cell->size += next->size;
            next = next->next;
            cell->next = next;
            if (next) {
                next->prev = cell;
            }
        }
    }
    if (prev) {
        prev->next = cell;
        if ((u8*)prev + prev->size == (u8*)cell) {
            prev->size += cell->size;
            prev->next = next;
            if (next) {
                next->prev = prev;
            }
        }
        return list;
    }
    return cell;
}

static int DLOverlap(struct Cell * list, void * start, void * end) {
    struct Cell * cell = list;

    while(cell) {
        if (((start <= cell) 
            && (cell < end)) 
            || ((start < (void* ) ((u8*)cell + cell->size)) 
            && ((void* ) ((u8*)cell + cell->size) <= end))) {
            return 1;
        }
        cell = cell->next;
    }
    return 0;
}

static long DLSize(struct Cell * list) {
    struct Cell * cell;
    long size;

    size = 0;
    cell = list;

    while(cell) {
        size += cell->size;
        cell = cell->next;
    }

    return size;
}

void * OSAllocFromHeap(int heap, unsigned long size) {
    struct HeapDesc * hd;
    struct Cell * cell;
    struct Cell * newCell;
    long leftoverSize;
    long requested;

    requested = size;
    ASSERTMSGLINE(0x14D, HeapArray, "OSAllocFromHeap(): heap is not initialized.");
    ASSERTMSGLINE(0x14E, (signed long)size > 0, "OSAllocFromHeap(): invalid size.");
    ASSERTMSGLINE(0x14F, heap >= 0 && heap < NumHeaps, "OSAllocFromHeap(): invalid heap handle.");
    ASSERTMSGLINE(0x150, HeapArray[heap].size >= 0, "OSAllocFromHeap(): invalid heap handle.");

    hd = &HeapArray[heap];
    size += 0x20;
    size = (size + 0x1F) & 0xFFFFFFE0;

    for(cell = hd->free; cell != NULL; cell = cell->next) {
        if ((signed)size <= (signed)cell->size) {
            break;
        }
    }

    if (cell == NULL) {
#if DEBUG
        OSReport("OSAllocFromHeap: Warning- failed to allocate %d bytes\n", size);
#endif
        return NULL;
    }
    ASSERTMSGLINE(0x168, !((s32)cell & 0x1F), "OSAllocFromHeap(): heap is broken.");
    ASSERTMSGLINE(0x169, cell->hd == NULL, "OSAllocFromHeap(): heap is broken.");

    leftoverSize = cell->size - size;
    if (leftoverSize < 0x40U) {
        hd->free = DLExtract(hd->free, cell);
    } else {
        cell->size = size;
        newCell = (void*)((u8*)cell + size);
        newCell->size = leftoverSize;
#ifdef ENABLE_HEAPDESC
        newCell->hd = 0;
#endif
        newCell->prev = cell->prev;
        newCell->next = cell->next;
        if (newCell->next != NULL) {
            newCell->next->prev = newCell; 
        }
        if (newCell->prev != NULL) {
            newCell->prev->next = newCell; 
        } else {
            ASSERTMSGLINE(0x186, hd->free == cell, "OSAllocFromHeap(): heap is broken.");
            hd->free = newCell;
        }
    }

    hd->allocated = DLAddFront(hd->allocated, cell);
#ifdef ENABLE_HEAPDESC
    cell->hd = hd;
    cell->requested = requested;
    hd->headerBytes += 0x20;
    hd->paddingBytes += (cell->size - (requested + 0x20));
    hd->payloadBytes += requested;
#endif
    return (u8*)cell + 0x20;
}

void * OSInitAlloc(void * arenaStart, void * arenaEnd, int maxHeaps) {
    unsigned long arraySize;
    int i;
    struct HeapDesc * hd;

    ASSERTMSGLINE(0x283, maxHeaps > 0, "OSInitAlloc(): invalid number of heaps.");
    ASSERTMSGLINE(0x285, (u32)arenaStart < (u32)arenaEnd, "OSInitAlloc(): invalid range.");
    ASSERTMSGLINE(0x288, maxHeaps <= (((u32)arenaEnd - (u32)arenaStart) / 24U), "OSInitAlloc(): too small range.");
    arraySize = maxHeaps * sizeof(struct HeapDesc);
    HeapArray = arenaStart;
    NumHeaps = maxHeaps;

    for(i = 0; i < NumHeaps; i++) {
        hd = &HeapArray[i];
        hd->size = -1;
        hd->free = hd->allocated = 0;
#ifdef ENABLE_HEAPDESC
        hd->paddingBytes = hd->headerBytes = hd->payloadBytes = 0;
#endif
    }
    __OSCurrHeap = -1;
    arenaStart = (void*) ((u32)((char*)HeapArray + arraySize));
    arenaStart = (void*) (((u32)arenaStart + 0x1F) & 0xFFFFFFE0);
    ArenaStart = arenaStart;
    ArenaEnd = (void*) ((u32)arenaEnd & 0xFFFFFFE0);
    ASSERTMSGLINE(0x2A4, ((u32)ArenaEnd - (u32)ArenaStart) >= 0x40U, "OSInitAlloc(): too small range.");
    return arenaStart;
}

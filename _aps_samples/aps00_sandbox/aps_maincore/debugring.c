#include <sdk/config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <debug.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>

#include <debugring.h>

/* static members */
sDebugRing *pDRing;
int myCpuId;
int lockID;

/** ------------------------------------- *
 * PROTOTYPE Definitions
 * -------------------------------------- */
static void copymemory(char *dst, const char*src, int size);

/** ------------------------------------- *
 * MACRO
 * -------------------------------------- */
/* RING CONTROL */
#define NEXT_RING_POS(h) (((h+1) >= NUM_DEBUGRING_ITEMS)? 0 : h+1)

/** lock Wrapper */
static inline void __mutex_lock(void)
{
    /* NOP */
}

/** unlock Wrapper */
static inline void __mutex_unlock(void)
{
    /* NOP */
}

/** ------------------------------------- *
 * Public Functions
 * -------------------------------------- */
int init_debugring(void* buf)
{
    int ret = 0;
    myCpuId = 0;
    pDRing = (sDebugRing*)buf; 

#if 0
sDebugRingHead *pRingHeader;
sDebugRingItem *pRing;
sDebugMemPool *pMemPool;
    pRingHeader = mshr + OFFSET_DEBUGRING_HEAD;
    pRing = mshr + OFFSET_DEBUGRING_RING;
    pMemPool = mshr + OFFSET_DEBUGRING_MEMPOOL;
#endif 
    printf("ret = %d\n", ret);

    pDRing->header.head = 0;
    pDRing->header.tail = 0;
    return ret;
}

int enqueue_debugring(int param, void *buf, int len)
{
    int h;
    int t;
    int put_pos = -1;
    __mutex_lock();
    h = pDRing->header.head;
    t = pDRing->header.tail;
    if (h == NEXT_RING_POS(t)) {
        /* overflow */
    } else {
        /* get pos */
        put_pos = t;
        t = NEXT_RING_POS(t);
        pDRing->ring[put_pos].cpuid = myCpuId;
        pDRing->ring[put_pos].param = param;
        if ((buf != NULL) && (len != 0)) {
            pDRing->ring[put_pos].addr = &pDRing->memPool[put_pos];
            len = (len > SIZE_DEBUGRING_MEMPOOL)? SIZE_DEBUGRING_MEMPOOL : len;
            pDRing->ring[put_pos].size = len;
            copymemory((int *)&pDRing->memPool[put_pos], (const int *)buf, len);
        } else {
            pDRing->ring[put_pos].addr = NULL;
            pDRing->ring[put_pos].size = 0;
        }
    }
    pDRing->header.tail = t;
    __mutex_unlock();
    return put_pos;
}

int dequeue_debugring(sDebugRingItem *getItem, void *buf, int len)
{
    int h;
    int t;
    int get_pos = -1;
    __mutex_lock();
    h = pDRing->header.head;
    t = pDRing->header.tail;
    if (h == t) {
        /* empty */
    } else {
        /* get pos */
        get_pos = h;
        h = NEXT_RING_POS(h);
        *getItem = pDRing->ring[get_pos];
        if (buf != NULL) {
            if ((pDRing->ring[get_pos].addr == NULL) || (pDRing->ring[get_pos].size == 0)) {
                /* buffer had been not unused */
            } else {
                len = (len > SIZE_DEBUGRING_MEMPOOL)? SIZE_DEBUGRING_MEMPOOL : len;
                copymemory((int *)buf, (const int *)getItem->addr, len);
            }
        }
    }
    pDRing->header.head = h;
    __mutex_unlock();
    return get_pos;
}

/** ------------------------------------- *
 * Static Functions
 * -------------------------------------- */
static void copymemory(char *dst, const char*src, int size)
{
    int pos;
    for (pos = 0; pos < size; pos++) {
        dst[pos] = src[pos];
    }
    return;
}

#ifndef _DEBUGRING_H_
#define _DEBUGRING_H_

#include <asmp/asmp.h>
#include <asmp/mptask.h>
#include <asmp/mpshm.h>
#include <asmp/mpmq.h>
#include <asmp/mpmutex.h>

/* Ring Size */
#define SIZE_DEBUGRING_MEMPOOL      (1024)      /* 1KB */
#define NUM_DEBUGRING_ITEMS         (48)        /* 48 entries */
#define TOTALSIZE_DEBUGRING_USED    (64*1024)   /* 64KB */

/** Ring Buffer Structs Definitions */
typedef struct _s_debugring_head {
    int head;
    int tail;
    mpmutex_t *pmutex;
    int _reserved_1;    
} sDebugRingHead;

typedef struct _s_debugring_item {
    int cpuid;
    int param;
    void *addr;
    int size;    
} sDebugRingItem;

/* pool buf */
typedef struct s_mempool_item {
    char buf[SIZE_DEBUGRING_MEMPOOL];
} sDebugMemPool;

/* Memory Layout */
typedef struct s_debugring {
    sDebugMemPool memPool[NUM_DEBUGRING_ITEMS];
    sDebugRingItem ring[NUM_DEBUGRING_ITEMS];
    sDebugRingHead header;
} sDebugRing;

typedef enum _e_initmode_for_ring {
    DQUEUE_MASTER,
    DQUEUE_SLAVE,
} eInitMode;

/* Public Method Definitions */
int init_debugring(void* buf, mpmutex_t *pmutex, eInitMode master);
sDebugRing *get_debugring_addr(void);
int enqueue_debugring(int param, void *buf, int len);
int dequeue_debugring(sDebugRingItem *getItem, void *buf, int len);

#endif /* _DEBUGRING_H_ */
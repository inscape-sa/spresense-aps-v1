#ifndef _DEBUGRING_H_
#define _DEBUGRING_H_

/* Ring Size */
#define SIZE_DEBUGRING_MEMPOOL      (1024)  /* 1KB */
#define NUM_DEBUGRING_ITEMS         (48)    /* 48 entries */

/** Ring Buffer Structs Definitions */
typedef struct _s_debugring_head {
    int head;
    int tail;
    int _reserved_0;
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

/* Public Method Definitions */
int init_debugring(void *buf);
int enqueue_debugring(int param, void *buf, int len);
int dequeue_debugring(sDebugRingItem *getItem, void *buf, int len);

#endif /* _DEBUGRING_H_ */
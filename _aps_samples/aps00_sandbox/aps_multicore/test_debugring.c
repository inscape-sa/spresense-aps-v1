#include <debugring.h>

/* DEBUG = ON/OFF */
#define _DEBUG_PRINTF_
#ifdef _DEBUG_PRINTF_
#define dbgprintf(f_, ...) printf((f_), ##__VA_ARGS__)
#else
#define dbgprintf(f_, ...) while(0){}
#endif /* _DEBUG_PRINTF_ */

/** ------------------------------------- *
 * PROTOTYPE Definitions
 * -------------------------------------- */
static void copymemory(char *dst, const char*src, int size);
static void setmemory(char *dst, char setchar, int size);
static int _local_strlen(char *str);


/** ------------------------------------- *
 * Public Functions 
 * - but, these function have to be defined in user-code.
 * -------------------------------------- */
/** Ring Buffer Test */
void test_debugring(void *buf)
{
    int ret;
    char set_buf[] = "Test Test Test";
    const char get_buf_len = 64;
    char get_buf[get_buf_len];
    sDebugRingItem getitem;
    int set_len;
    
    int loop;
    const int num_test = 100;

    ret = init_debugring(buf);
    dbgprintf("ret(%d) = init_debugring(on 0x%08x)\n", ret, buf);
    set_len = _local_strlen(set_buf) + 1;

    for (loop = 0; loop < num_test; loop++)   
    {
        enqueue_debugring(set_len, set_buf, set_len);

        setmemory(get_buf, 0, get_buf_len);
        ret = dequeue_debugring(&getitem, get_buf, get_buf_len);
        dbgprintf("dequeue(RET=%d, CPUID=%d, PARAM=0x%x, buf[%s][%s])\n", ret, getitem.cpuid, getitem.param, get_buf, set_buf);

        enqueue_debugring(set_len, set_buf, set_len);
        
        setmemory(get_buf, 0, get_buf_len);
        ret = dequeue_debugring(&getitem, get_buf, get_buf_len);
        dbgprintf("dequeue(RET=%d, CPUID=%d, PARAM=0x%x, buf[%s][%s])\n", ret, getitem.cpuid, getitem.param, get_buf, set_buf);

        enqueue_debugring(set_len, set_buf, set_len);

        setmemory(get_buf, 0, get_buf_len);
        ret = dequeue_debugring(&getitem, get_buf, get_buf_len);
        dbgprintf("dequeue(RET=%d, CPUID=%d, PARAM=0x%x, buf[%s][%s])\n", ret, getitem.cpuid, getitem.param, get_buf, set_buf);

        setmemory(get_buf, 0, get_buf_len);
        ret = dequeue_debugring(&getitem, get_buf, get_buf_len);
        dbgprintf("dequeue(RET=%d, CPUID=%d, PARAM=0x%x, buf[%s][%s])\n", ret, getitem.cpuid, getitem.param, get_buf, set_buf);
    }

    return;
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

static void setmemory(char *dst, char setchar, int size)
{
    int pos;
    for (pos = 0; pos < size; pos++) {
        dst[pos] = setchar;
    }
    return;
}

static int _local_strlen(char *str)
{
    int pos = 0;
    do {
        //dbgprintf("%d-", str[pos]);
        if (str[pos] != '\0') {
            pos++;
            continue;
        }
        break;
    } while(1);
    //dbgprintf("\n");
    return pos;
}
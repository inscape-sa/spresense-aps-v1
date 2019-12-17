#ifndef __WORKER_CTRL_H__
#define __WORKER_CTRL_H__

#include <sdk/config.h>
#include <stdio.h>
#include <string.h>

#include <asmp/asmp.h>
#include <asmp/mptask.h>
#include <asmp/mpshm.h>
#include <asmp/mpmq.h>
#include <asmp/mpmutex.h>

class WorkerCtrl {
public:
  typedef enum e_task_status {
    TASK_EMPTY,
    TASK_READY,
  } TASK_STAT;
private:
  const char *filename;
  TASK_STAT status;
  mptask_t mptask;
  mpmq_t mq;
  mpmutex_t mutex;
  mpshm_t shm;
  void *buf;
  int retcode;

private:
  WorkerCtrl();
public:
  WorkerCtrl(const char *filename);
  ~WorkerCtrl(void);
  bool getReady(void);
  int initMutex(int id);
  int initMq(int id);
  void *initShm(int id, ssize_t size);
  void *getAddrShm(void);
  int execTask(void);
  int destroyTask(void);
  void lock(void);
  void unlock(void);
  int send(int8_t msgid, uint32_t value);
  int receive(uint32_t *pvalue, bool nonblocking = false);
};

#endif /* __WORKER_CTRL_H__ */
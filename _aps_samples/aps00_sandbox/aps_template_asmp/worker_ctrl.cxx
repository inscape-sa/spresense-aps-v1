#include "worker_ctrl.h"

WorkerCtrl::WorkerCtrl(const char *setFileName)
{
  int ret;
  
  this->retcode = -1;
  this->status = TASK_EMPTY;
  this->filename = setFileName;
  ret = mptask_init(&this->mptask, this->filename);
  if (ret != 0) {
    return;
  }
  ret = mptask_assign(&this->mptask);
  if (ret != 0) {
    return;
  }
  this->status = TASK_READY;
  return;
}

WorkerCtrl::~WorkerCtrl(void)
{
  /* Finalize all of MP objects */
  mpshm_detach(&this->shm);
  mpshm_destroy(&this->shm);
  mpmutex_destroy(&this->mutex);
  mpmq_destroy(&this->mq);
}

bool WorkerCtrl::getReady(void)
{
  return (this->status == TASK_READY)? true : false;
}

int WorkerCtrl::initMutex(int id)
{
  int ret;
  ret = mpmutex_init(&this->mutex, id);
  if (ret < 0) {
    return ret;
  }
  ret = mptask_bindobj(&this->mptask, &this->mutex);
  if (ret < 0) {
    return ret;
  } 
  return 0; 
}

int WorkerCtrl::initMq(int id)
{
  int ret;
  ret = mpmq_init(&this->mq, id, mptask_getcpuid(&this->mptask));
  if (ret < 0) {
    return ret;
  }
  ret = mptask_bindobj(&this->mptask, &this->mq);
  if (ret < 0) {
    return ret;
  }
  return 0; 
}

void *WorkerCtrl::initShm(int id, ssize_t size)
{
  int ret;
  ret = mpshm_init(&this->shm, id, size);
  if (ret < 0) {
    return NULL;
  }
  ret = mptask_bindobj(&this->mptask, &this->shm);
  if (ret < 0) {
    return NULL;
  }
  /* Map shared memory to virtual space */
  this->buf = mpshm_attach(&this->shm, size);
  if (!this->buf) {
    return NULL;
  }
  return buf; 
}

int WorkerCtrl::execTask(void)
{
  return mptask_exec(&this->mptask);
}

int WorkerCtrl::destroyTask(void)
{
  mptask_destroy(&this->mptask, false, &this->retcode);
  return this->retcode;
}

void WorkerCtrl::lock(void)
{
  mpmutex_lock(&this->mutex);
}

void WorkerCtrl::unlock(void)
{
  mpmutex_unlock(&this->mutex);
}

int WorkerCtrl::send(int8_t msgid, uint32_t value)
{
  return mpmq_send(&this->mq, msgid, value);
}

int WorkerCtrl::receive(uint32_t *pvalue, bool nonblocking)
{
  int ret;
  uint32_t gabage;
  if (pvalue == NULL) {
    pvalue = &gabage;
  }
  if (nonblocking) {
    ret = mpmq_timedreceive(&this->mq, pvalue, MPMQ_NONBLOCK);
  } else {
    ret = mpmq_receive(&this->mq, pvalue);
  }
  return ret;
}
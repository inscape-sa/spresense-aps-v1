#include "worker_ctrl.h"

/** WorkerCtrl (constructor)
 * - arg = program on sub-core
 * - geneate task, and assign at a CPU. 
 */
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

/** WorkerCtrl (destructor)
 * - delete shm, mutex, mq
 */
WorkerCtrl::~WorkerCtrl(void)
{
  /* Finalize all of MP objects */
  mpshm_detach(&this->shm);
  mpshm_destroy(&this->shm);
  mpmutex_destroy(&this->mutex);
  mpmq_destroy(&this->mq);
}

/** getReady
 * - check already init all task-resources
 */
bool WorkerCtrl::getReady(void)
{
  return (this->status == TASK_READY)? true : false;
}

/** initMutex
 * - init mutex in TaskResource as ID
 */
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

/** initMq
 * - init Message-Queue in TaskResource as ID
 */
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

/** initShm
 * - init Shared-memory in TaskResource as ID
 */
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

/** getAddrShm
 * - get Address for Shared-Memory
 */
void *WorkerCtrl::getAddrShm(void)
{
  return this->buf;
}

/** execTask
 * - start Task on sub-core
 */
int WorkerCtrl::execTask(void)
{
  return mptask_exec(&this->mptask);
}

/** destroyTask
 * - force Exit and Destroy Task on sub-core
 */
int WorkerCtrl::destroyTask(void)
{
  mptask_destroy(&this->mptask, false, &this->retcode);
  return this->retcode;
}

/** lock
 * - Entering Critical-Section with mutex
 */
void WorkerCtrl::lock(void)
{
  mpmutex_lock(&this->mutex);
}

/** unlock
 * - Exit Critical-Section with mutex
 */
void WorkerCtrl::unlock(void)
{
  mpmutex_unlock(&this->mutex);
}

/** send
 * - send message with (id,value) via MQ
 */
int WorkerCtrl::send(int8_t msgid, uint32_t value)
{
  return mpmq_send(&this->mq, msgid, value);
}

/** receive
 * - receive message (id,value) via MQ
 * - it can be used for nonblocking receive.
 */
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

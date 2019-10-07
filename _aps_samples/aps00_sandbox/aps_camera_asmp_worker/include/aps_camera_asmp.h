#ifndef __APPS_EXAMPLES_CAMERA_ASMP_WORKER_H
#define __APPS_EXAMPLES_CAMERA_ASMP_WORKER_H

/* MP object keys. Must be synchronized with supervisor. */


#define APS00_SANDBOX_APS_CAMERA_ASMPKEY_STRIDE         (4)
#define APS00_SANDBOX_APS_CAMERA_ASMPKEY_SHM(cpuid)     ((cpuid*4) + 1)
#define APS00_SANDBOX_APS_CAMERA_ASMPKEY_MQ(cpuid)      ((cpuid*4) + 2)
#define APS00_SANDBOX_APS_CAMERA_ASMPKEY_MUTEX(cpuid)   ((cpuid*4) + 3)

#define MSG_ID_APS00_SANDBOX_APS_CAMERA_ASMP 1
#define MSG_ID_APS00_SANDBOX_APS_CAMERA_EXIT 3

#define IMAGE_YUV_SIZE  (320*240*2)
#define SHMSIZE         (IMAGE_YUV_SIZE)


#endif /* __APPS_EXAMPLES_CAMERA_ASMP_WORKER_H */
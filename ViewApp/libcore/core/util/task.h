#pragma once

#include "c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct os_task_queue;
typedef struct os_task_queue os_task_queue_t;

typedef void (*os_task_t)(void *param);

EXPORT os_task_queue_t *os_task_queue_create(void);
EXPORT bool os_task_queue_queue_task(os_task_queue_t *tt, os_task_t task,
				     void *param);

//结束当前线程
EXPORT void os_task_queue_destroy(os_task_queue_t *tt);
//等待当前线程队列结束
EXPORT bool os_task_queue_wait(os_task_queue_t *tt);

//队列线程是否已经开启
EXPORT bool os_task_queue_inside(os_task_queue_t *tt);

#ifdef __cplusplus
}
#endif

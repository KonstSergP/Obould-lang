#ifndef THREADS_H
#define THREADS_H

#include <stdint.h>

typedef struct ob_7Threads_ThreadHandle ob_7Threads_ThreadHandle;
typedef ob_7Threads_ThreadHandle* ob_7Threads_Thread;

typedef void (*ob_Routine)(void);
typedef void (*ob_RoutineI64)(int64_t);

ob_7Threads_Thread ob_7Threads_Start(ob_Routine routine);
ob_7Threads_Thread ob_7Threads_StartI64(ob_RoutineI64 routine, int64_t arg);
void ob_7Threads_Join(ob_7Threads_Thread t);
void ob_7Threads_Threads(void);

#endif /* THREADS_H */

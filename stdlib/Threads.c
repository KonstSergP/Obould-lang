#define GC_THREADS
#include <stdint.h>
#include "gc/gc.h"
#include "Threads.h"

#if defined(__APPLE__)
#define OB_SYMBOL(name) __asm__("_ob_" name)
#else
#define OB_SYMBOL(name) __asm__("ob_" name)
#endif


typedef struct ob_7Threads_ThreadHandle {
    pthread_t handle;
} ThreadHandle;

typedef struct ob_7Threads_RoutineArg {
    ob_RoutineI64 routine;
    int64_t arg;
} RoutineI64;

ob_7Threads_Thread Start(ob_Routine routine) OB_SYMBOL("7Threads_Start");
ob_7Threads_Thread StartI64(ob_RoutineI64 routine, int64_t arg) OB_SYMBOL("7Threads_StartI64");
void Join(ob_7Threads_Thread t) OB_SYMBOL("7Threads_Join");
void Threads(void) OB_SYMBOL("Threads");


static void* thread_wrapper(void* arg) {
    ob_Routine routine = arg;
    if (routine) {
        routine();
    }
    return NULL;
}

static void* thread_wrapper_i64(void* arg) {
    RoutineI64* payload = arg;
    if (payload && payload->routine) {
        payload->routine(payload->arg);
    }
    return NULL;
}

ob_7Threads_Thread Start(ob_Routine routine) {
    if (!routine) return NULL;

    ob_7Threads_Thread t = GC_MALLOC(sizeof(ThreadHandle));
    if (!t) return NULL;

    if (pthread_create(&t->handle, NULL, thread_wrapper, (void*)routine) != 0) {
        return NULL;
    }
    return t;
}

ob_7Threads_Thread StartI64(ob_RoutineI64 routine, int64_t arg) {
    if (!routine) return NULL;

    RoutineI64* payload = GC_MALLOC(sizeof(RoutineI64));
    if (!payload) return NULL;
    payload->routine = routine;
    payload->arg = arg;

    ob_7Threads_Thread t = GC_MALLOC(sizeof(ThreadHandle));
    if (!t) return NULL;

    if (pthread_create(&t->handle, NULL, thread_wrapper_i64, payload) != 0) {
        return NULL;
    }
    return t;
}

void Join(ob_7Threads_Thread t) {
    if (!t) return;
    pthread_join(t->handle, NULL);
}

void Threads(void) {}
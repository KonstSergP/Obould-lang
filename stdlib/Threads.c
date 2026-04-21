#include <stdlib.h>
#include <pthread.h>
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

ob_7Threads_Thread Start(ob_Routine routine) OB_SYMBOL("7Threads_Start");
void Join(ob_7Threads_Thread t) OB_SYMBOL("7Threads_Join");
void Threads(void) OB_SYMBOL("Threads");

static void* thread_wrapper(void* arg) {
    ob_Routine routine = arg;
    if (routine) {
        routine();
    }
    return NULL;
}

ob_7Threads_Thread Start(ob_Routine routine) {
    if (!routine) return NULL;

    ob_7Threads_Thread t = GC_MALLOC(sizeof(ThreadHandle));
    if (!t) return NULL;

    if (pthread_create(&t->handle, NULL, thread_wrapper, (void*)routine) != 0) {
        GC_FREE(t);
        return NULL;
    }
    return t;
}

void Join(ob_7Threads_Thread t) {
    if (!t) return;
    pthread_join(t->handle, NULL);
}

void Threads(void) {}

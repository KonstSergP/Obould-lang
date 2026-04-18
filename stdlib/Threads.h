#ifndef THREADS_H
#define THREADS_H


typedef struct ob_7Threads_ThreadHandle ob_7Threads_ThreadHandle;
typedef ob_7Threads_ThreadHandle* ob_7Threads_Thread;

typedef void (*ob_Routine)(void);

ob_7Threads_Thread ob_7Threads_Start(ob_Routine routine);
void ob_7Threads_Join(ob_7Threads_Thread t);
void ob_7Threads_Threads(void);

#endif /* THREADS_H */

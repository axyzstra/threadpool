#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_
typedef struct ThreadPool ThreadPool;
// 创建线程池
ThreadPool* threadpoolCreate(int min, int max, int queueSize);

void* worker(void* arg);
void* manager(void* arg);


#endif  // _THREADPOOL_H_
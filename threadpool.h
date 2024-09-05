#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_
typedef struct ThreadPool ThreadPool;
// 创建线程池
ThreadPool* threadpoolCreate(int min, int max, int queueSize);

// 销毁线程池
int threadPoolDestroy(ThreadPool* pool);

// 添加任务
void threadPoolAdd(ThreadPool* pool, void(*func)(void*), void* arg);

// 获取线程池中工作的线程数
int threadPoolBusyNum(ThreadPool* pool);

// 获取线程池中活着的线程数
int threadPoolAliveNum(ThreadPool* pool);



void* worker(void* arg);
void* manager(void* arg);

void threadExit(ThreadPool* pool);


#endif  // _THREADPOOL_H_
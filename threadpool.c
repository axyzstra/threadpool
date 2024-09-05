#include "threadpool.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

// 任务
typedef struct Task {
    void (*function)(void*);
    void* arg;
} Task;

// 线程池
typedef struct ThreadPool {
    //  任务队列，数组的每个元素是一个函数及其参数
    Task* taskQ;
    int queueCapacity;  // 容量
    int queueSize;       // 任务个数
    int queueFront;     // 队头
    int queueRear;      // 队尾

    // 线程
    pthread_t managerID;    // 管理者线程 ID
    pthread_t *threadIDs;   // 工作的线程 ID
    int minNum;             // 线程池最小线程数
    int maxNum;             // 线程池最大线程数
    int busyNum;            // 忙的线程数
    int liveNum;            // 存活的线程数
    int exitNum;            // 需要销毁的线程数
    pthread_mutex_t mutexPool;  // 锁整个线程池
    pthread_mutex_t mutexBusy;  // 锁 busyNum 变量
    pthread_cond_t notFull;     // 条件变量判断队列是否满了
    pthread_cond_t notEmpty;    // 任务队列是否为空

    int shutdown;               // 判断是否需要销毁线程池，销毁为 1，反之为 0
} ThreadPool;

ThreadPool *threadpoolCreate(int min, int max, int queueSize)
{
    // 创建线程池
    ThreadPool* pool = (ThreadPool*) malloc(sizeof(ThreadPool));
    do {
        if (pool == NULL) {
            printf("malloc threadpool fail ... \n");
            break;
        }
        
        // 创建工作线程 id 数组
        pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max);
        if (pool->threadIDs == NULL) {
            printf("malloc threadIDs fail ...\n");
            break;
        }
        memset(pool->threadIDs, 0, sizeof(pthread_t) * max);

        pool->minNum = min;
        pool->maxNum = max;
        pool->busyNum = 0;
        pool->liveNum = min;   // 目前存活的线程数，初始化时将在后面进行创建
        pool->exitNum = 0;     // 需要退出的线程数，即管理线程计算后得出需要退出的线程数量


        // 初始化锁和条件变量
        if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
            pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
            pthread_cond_init(&pool->notFull, NULL) != 0 ||
            pthread_cond_init(&pool->notEmpty, NULL) != 0) 
        {
            printf("mutex or condition init fail ...\n");
            break;
        }
        // 创建任务队列
        pool->taskQ = (Task*)malloc(sizeof(Task) * queueSize);
        if (pool->taskQ == NULL) {
            printf("malloc taskQ fail...\n");
            break;
        }
        pool->queueCapacity = queueSize;
        pool->queueSize = 0;
        pool->queueFront = 0;
        pool->queueRear = 0;

        pool->shutdown = 0;

        // 创建管理者线程
        pthread_create(&pool->managerID, NULL, manager, pool);

        // 工作者线程
        for (int i = 0; i < min; i++) {
            pthread_create(pool->threadIDs[i], NULL, worker, pool);
        }
        return pool;
    } while (0);

    if (pool && pool->taskQ) free(pool->taskQ);
    if (pool && pool->threadIDs) free(pool->threadIDs);
    if (pool) free(pool);
    return NULL;
}

void *worker(void *arg)
{
    return NULL;
}

void *manager(void *arg)
{
    return NULL;
}

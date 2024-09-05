#include "threadpool.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

// 每次添加的线程数
#define NUMBER 2

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
    ThreadPool* pool = (ThreadPool*) arg;
    while (1) {
        pthread_mutex_lock(&pool->mutexPool);
        // 任务队列为空则工作线程阻塞
        while (pool->queueSize == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
            // 若线程唤醒后需要被销毁，则进行销毁，注意销毁时数量必须多于最小值
            if (pool->exitNum > 0) {
                pool->exitNum--;
                if (pool->liveNum > pool->minNum) {
                    pool->liveNum--;
                    // 销毁前需要释放锁
                    pthread_mutex_unlock(&pool->mutexPool);
                    threadExit(pool);
                }
            }
        }
        // 若线程池关闭，则销毁当前线程
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->mutexPool);
            threadExit(pool);
        }

        // 以下为正常情况，即从任务队列中取出任务进行处理
        Task task;
        task.function = pool->taskQ[pool->queueFront].function;
        task.arg = pool->taskQ[pool->queueFront].arg;
        // 取出任务后，头节点后移，此处为循环队列
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;

        // notFull 表示任务队列已满的条件变量，此处处理了一个任务队列
        // 即可唤醒生产任务的线程继续生产任务
        pthread_cond_signal(&pool->notFull);
        pthread_mutex_unlock(&pool->mutexPool);

        printf("thread %ld start working...\n", pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);
        // 处理任务
        task.function(task.arg);
        free(task.arg);
        task.arg = NULL;
        printf("thread %ld end working...\n", pthread_self());

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    return NULL;
}

void *manager(void *arg)
{
    ThreadPool* pool = (ThreadPool*) arg;
    // 若线程池不关闭，开始循环
    while (!pool->shutdown) {
        sleep(3);
        // 读取线程池信息，作为管理的依据
        pthread_mutex_lock(&pool->mutexPool);
        // 任务个数
        int queueSize = pool->queueSize;
        // 存活的线程数
        int liveNum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);

        pthread_mutex_lock(&pool->mutexBusy);
        // 正在忙的线程数
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);
        // 现在得到了任务数，正在忙的线程，以及目前总线程数
        // 管理者线程将根据这三个数据对线程进行管理

        // 若任务个数 > 存活线程个数 && 存活线程数 < 最大线程数 则添加线程
        if (queueSize > liveNum && liveNum < pool->maxNum) {
            pthread_mutex_lock(&pool->mutexPool);
            int count = 0;
            for (int i = 0; i < pool->maxNum && 
                            count < NUMBER && 
                            liveNum < pool->maxNum; i++)
            {
                if (pool->threadIDs[i] == 0) {
                    pthread_create(&pool->threadIDs[i], NULL, worker, pool);
                    count++;
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }

        // 忙线程 * 2 < 存活的线程数 && 存活线程数 > 最小线程数 需要销毁多余的线程
        if (pool->busyNum * 2 < pool->liveNum && pool->liveNum > pool->minNum) {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&pool->mutexPool);
            // 唤醒阻塞的线程，让其自动销毁
            for (int i = 0; i < NUMBER; i++) {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }
    return NULL;
}

// 销毁当前线程，并在线程池中注销
void threadExit(ThreadPool *pool)
{
    pthread_t tid = pthread_self();
    for (int i = 0; i < pool->maxNum; i++) {
        if (pool->threadIDs[i] == tid) {
            pool->threadIDs[i] = 0;
            printf("threadExit() called, %ld exiting...\n", tid);
            break;
        }
    }
    pthread_exit(NULL);
}

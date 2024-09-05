#include "threadpool.h"
#include <pthread.h>

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
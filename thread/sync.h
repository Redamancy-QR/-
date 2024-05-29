
#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H
#include "list.h"
#include "stdint.h"
#include "thread.h"

/**
 * struct semaphore - 定义信号量
 * @value: 信号量的当前值
 * @waiters: 等待此信号量的线程列表
 *
 * 表示具有指示可用性的信号量。
 * waiters 列表跟踪正在等待此信号量的线程。
 */
struct semaphore {
    uint8_t value;
    struct list waiters;
};

/**
 * struct lock - 定义锁
 * @holder: 指向当前持有锁的任务的指针
 * @sema: 控制锁的信号量
 * @holder_repeat_nr: 持有者重新获取锁的累积计数
 *
 * 使用信号量进行同步的锁机制。
 * holder 字段指向当前拥有锁的任务。
 * holder_repeat_nr 用于防止持有者多次释放锁（详见P.449）。
 */
struct lock {
    struct task_struct *holder;
    struct semaphore sema;
    uint32_t holder_repeat_nr;
};

void lock_init(struct lock *plock);
void lock_acquire(struct lock *plock);
void lock_release(struct lock *plock);
void sema_init(struct semaphore *psema, uint8_t _value);
void sema_down(struct semaphore *psema);
void sema_up(struct semaphore *psema);
#endif

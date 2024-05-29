#include "sync.h"
#include "debug.h"
#include "interrupt.h"
#include "list.h"
#include "stdint.h"
//#include "stdio_kernel.h"
#include "thread.h"

/**
 * sema_init - 初始化信号量
 * @psema: 指向要初始化的信号量的指针
 * @value: 信号量的初始值
 *
 * 使用给定的值和空的等待列表初始化信号量。
 */
void sema_init(struct semaphore *psema, uint8_t _value) {
    psema->value = _value;
    list_init(&psema->waiters);
}

/**
 * lock_init - 初始化锁
 * @plock: 指向要初始化的锁的指针
 *
 * 设置一个带有空持有者的锁，重置持有者的重复计数，
 * 并将关联的信号量初始化为1（二进制信号量）。
 */
void lock_init(struct lock *plock) {
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    /* 二进制信号量具有两个状态（0/1） */
    sema_init(&plock->sema, 1);
}

/**
 * sema_down - 减少信号量的值
 * @psema: 指向信号量的指针
 *
 * 减少信号量的值。如果值达到零，则当前线程被阻塞并添加到信号量的等待列表中。
 */
void sema_down(struct semaphore *psema) {
    enum intr_status old_status = intr_disable();

    struct task_struct *cur_thread = running_thread();
    /** if (psema == 0) { */
    while (psema->value == 0) {
        if (list_elem_find(&psema->waiters, &running_thread()->general_tag)) {
            PANIC("The thread blocked has been in waiters list\n");
        }
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED);
    }

    psema->value--;
    ASSERT(psema->value == 0);

    intr_set_status(old_status);
}

/**
 * sema_up - 增加信号量的值
 * @psema: 指向信号量的指针
 *
 * 增加信号量的值。如果有任何线程被阻塞并在此信号量上等待，则解除其中一个线程的阻塞状态。
 */
void sema_up(struct semaphore *psema) {
    enum intr_status old_status = intr_disable();
    ASSERT(psema->value == 0);

    if (!list_empty(&psema->waiters)) {
        struct list_elem *blocked_thread_tag = list_pop(&psema->waiters);
        struct task_struct *blocked_thread =
            elem2entry(struct task_struct, general_tag, blocked_thread_tag);
        thread_unblock(blocked_thread);
    }
    psema->value++;
    ASSERT(psema->value == 1);
    intr_set_status(old_status);
}

/**
 * lock_acquire - 获取指定的锁
 * @plock: 指向锁的指针
 *
 * 尝试为当前运行的线程获取锁。如果锁已被另一个线程持有，则当前线程将被阻塞，直到锁被释放。
 */
void lock_acquire(struct lock *plock) {
    if (plock->holder != running_thread()) {
        sema_down(&plock->sema);
        plock->holder = running_thread();
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    } else {
        plock->holder_repeat_nr++;
    }
}

/**
 * lock_release - 释放指定的锁
 * @plock: 指向锁的指针
 *
 * 释放当前运行的线程持有的锁。如果当前线程多次获取了锁，则递减计数器，并仅当计数器达到零时才释放锁。
 */
void lock_release(struct lock *plock) {
    ASSERT(plock->holder == running_thread());
    if (plock->holder_repeat_nr > 1) {
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);

    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_up(&plock->sema);
}
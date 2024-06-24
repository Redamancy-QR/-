#include "ioqueue.h"
#include "debug.h"
#include "global.h"
#include "interrupt.h"
//#include "stdio_kernel.h"
#include "sync.h"
#include "thread.h"

/**
 * ioqueue_init - 初始化一个I/O队列
 * @ioq: 指向要初始化的ioqueue的指针
 *
 * 通过初始化锁、将生产者和消费者设置为NULL，并重置头部和尾部索引，来设置一个I/O队列。
 */
void ioqueue_init(struct ioqueue *ioq) {
    put_str("    ioqueue init start\n");
    lock_init(&ioq->_lock);
    ioq->consumer = ioq->producer = NULL;
    ioq->head = ioq->tail = 0;
    put_str("    ioqueue init start\n");
}

/**
 * next_pos - 计算缓冲区中的下一个位置
 * @pos: 当前缓冲区中的位置
 * Return: 缓冲区中的下一个位置
 *
 * 当到达末尾时，绕回缓冲区以保持循环特性。
 */
static int32_t next_pos(int32_t pos) { return (pos + 1) % BUF_SIZE; }

/**
 * ioq_is_full - 检查I/O队列是否已满
 * @ioq: 指向ioqueue的指针
 * Return: 如果队列已满则返回true，否则返回false
 *
 * 通过比较头部的下一个位置与当前尾部位置来确定队列是否已满。因此队列的总容量是BUF_SIZE-1。
 */
bool ioq_is_full(struct ioqueue *ioq) {
    ASSERT(intr_get_status() == INTR_OFF);
    return next_pos(ioq->head) == ioq->tail;
}

/**
 * ioq_is_empty - 检查I/O队列是否为空
 * @ioq: 指向ioqueue的指针
 * Return: 如果队列为空则返回true，否则返回false
 *
 * 如果头部和尾部位置相同，则队列被视为空。
 */
bool ioq_is_empty(struct ioqueue *ioq) {
    ASSERT(intr_get_status() == INTR_OFF);
    return ioq->tail == ioq->head;
}

/**
 * ioq_wait() - 阻塞当前的生产者或消费者在这个缓冲区上。
 * @waiter: 指向等待任务的task_struct指针。
 *
 * 此函数用于阻塞当前操作共享缓冲区的线程（生产者或消费者）。当前线程被设置为等待者然后被阻塞。
 * 这在需要等待某些条件变为真的情况下使用（例如等待写入缓冲区的空间或从缓冲区读取数据）。
 *
 * 上下文: 通常在需要线程间同步的生产者-消费者场景中使用。
 */
static void ioq_wait(struct task_struct **waiter) {
    ASSERT(waiter != NULL && *waiter == NULL);
    *waiter = running_thread();
    thread_block(TASK_BLOCKED);
}

/**
 * wakeup() - 唤醒被阻塞的等待者。
 * @waiter: 指向等待任务的task_struct指针。
 *
 * 此函数解除之前由ioq_wait()阻塞的线程。它接受等待者的task_struct指针，并将其解除阻塞，
 * 然后将指针设置为NULL。这是生产者-消费者问题中同步的一部分，当被阻塞的线程（等待者）等待的条件变为真时，
 * 需要唤醒它。
 *
 * 上下文: 与ioq_wait()一起使用，用于管理在生产者-消费者等同步场景中线程的阻塞和解除阻塞。
 */
static void ioq_wakeup(struct task_struct **waiter) {
    ASSERT(*waiter != NULL);
    thread_unblock(*waiter);
    *waiter = NULL;
}

/**
 * ioq_getchar - 从I/O队列中获取一个字符
 * @ioq: 指向ioqueue的指针
 * Return: 从队列中检索到的字符
 *
 * 等待队列非空（必要时阻塞），然后从尾部获取字符并调整尾部位置。
 */
char ioq_getchar(struct ioqueue *ioq) {
    ASSERT(intr_get_status() == INTR_OFF);
    while (ioq_is_empty(ioq)) {
        lock_acquire(&ioq->_lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->_lock);
    }

    char ret_char = ioq->buf[ioq->tail];
    ioq->tail = next_pos(ioq->tail);

    if (ioq->producer != NULL)
        ioq_wakeup(&ioq->producer);

    return ret_char;
}

/**
 * ioq_putchar - 将一个字符放入I/O队列
 * @ioq: 指向ioqueue的指针
 * @ch: 要放入队列的字符
 *
 * 等待队列非满（必要时阻塞），然后将字符放在头部并调整头部位置。
 */
void ioq_putchar(struct ioqueue *ioq, char ch) {
    ASSERT(intr_get_status() == INTR_OFF);
    while (ioq_is_full(ioq)) {
        lock_acquire(&ioq->_lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->_lock);
    }
    ioq->buf[ioq->head] = ch;
    ioq->head = next_pos(ioq->head);

    if (ioq->consumer != NULL)
        ioq_wakeup(&ioq->consumer);
}
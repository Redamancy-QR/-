#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "stdint.h"
#include "sync.h"
#include "thread.h"

#define BUF_SIZE 64

/**
 * struct ioqueue - 定义用于I/O操作的循环缓冲区
 * @_lock: 用于确保互斥访问缓冲区的锁
 * @producer: 指向表示缓冲区生产者的task_struct的指针
 * @consumer: 指向表示缓冲区消费者的task_struct的指针
 * @buf: 作为循环缓冲区的数组
 * @head: 缓冲区头部的索引，表示下一个读取位置
 * @tail: 缓冲区尾部的索引，表示下一个写入位置
 *
 * 表示用于生产者-消费者I/O操作的循环缓冲区。包括同步机制（锁）和指向生产者和消费者task_struct的指针。
 * 缓冲区实现为一个数组，使用头部和尾部索引来有效管理数据流。
 */
struct ioqueue {
    struct lock _lock;
    struct task_struct *producer;
    struct task_struct *consumer;
    char buf[BUF_SIZE];
    int32_t head;
    int32_t tail;
};

void ioqueue_init(struct ioqueue *ioq);
bool ioq_is_full(struct ioqueue *ioq);
bool ioq_is_empty(struct ioqueue *ioq);
char ioq_getchar(struct ioqueue *ioq);
void ioq_putchar(struct ioqueue *ioq, char ch);
#endif
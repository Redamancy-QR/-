#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "list.h"
#include "memory.h"
#include "stdint.h"

#define MAX_FILES_OPEN_PER_PROC 8
#define TASK_NAME_LEN 16

typedef void thread_func(void *);
typedef int16_t pid_t;

/* 线程生命周期内可能的状态 */
enum task_status {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

/**
 * struct intr_stack - 表示中断期间使用的堆栈。
 * @vec_no: 中断的向量编号。
 * @edi, esi, ebp, ebx, edx, ecx, eax: 保存的寄存器。
 * @esp_dummy: 虚拟堆栈指针（未使用）。
 * @gs, fs, es, ds: 段寄存器。
 * @error_code: 中断的错误码。
 * @eip: 指令指针。
 * @cs: 代码段。
 * @eflags: CPU 标志。
 * @esp: 堆栈指针。
 * @ss: 堆栈段。
 *
 * 此结构表示中断期间的堆栈状态，并用于保存被中断的线程的上下文。
 */
struct intr_stack {
    uint32_t vec_no;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t error_code;
    void (*eip)(void);
    uint32_t cs;
    uint32_t eflags;
    void *esp;
    uint32_t ss;
};

/**
 * struct thread_stack - 表示用于函数执行的线程堆栈。
 * @ebp, ebx, edi, esi: 调用者函数的保存寄存器。
 * @eip: 指令指针。
 * @unused_retaddr: 未使用的返回地址占位符。
 * @function: 线程将要执行的函数。
 * @func_arg: 函数的参数。
 *
 * 此结构表示为即将开始执行给定函数的线程设置的堆栈。
 */
struct thread_stack {
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;
    void (*eip)(thread_func *func, void *func_arg);
    /* 以下仅供第一次被调度上cpu使用 */
    void(*unused_retaddr);
    thread_func *function;
    void *func_arg;
};

/**
 * struct task_struct - 进程或线程的PCB，即程序控制块
 * @self_kstack: 各线程的内核栈顶指针。
 * @status: 线程状态。
 * @name: 任务（线程或进程）的名字。
 * @priority: 线程的优先级。
 * @ticks: 每次在处理器是执行的时间嘀嗒数。
 * @elapsed_ticks: 此任务自上CPU后一共执行了多少嘀嗒数。
 * @general_tag: 线程在一般的队列中的节点
 * @all_list_tag: 线程在线程队列 thread_all_list 中的节点
 * @pg_dir: 描述自己页表的虚拟地址，如果是TCB，则为NULL
 * @userprog_vaddr: 用户进程的虚拟内存池
 * @stack_magic: 魔数，用与栈的边界标记。
 */
struct task_struct {
    uint32_t *self_kstack;
    pid_t pid;
    enum task_status status;
    char name[TASK_NAME_LEN];
    uint8_t priority;
    uint8_t ticks;
    uint32_t elapsed_ticks;
    struct list_elem general_tag;
    struct list_elem all_list_tag;
    uint32_t *pg_dir; 
    struct virtual_addr userprog_vaddr; //
    uint32_t stack_magic;
};

void thread_init();
struct task_struct *running_thread();
void init_thread(struct task_struct *thread, char *name, int _priority);
void thread_create(struct task_struct *thread, thread_func function,void *func_arg);
struct task_struct *thread_start(char *name, int _priority,thread_func function, void *func_arg);
void schedule();
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct *pthread);
#endif
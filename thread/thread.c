#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"

#define PAGE_SIZE 4096

/* 关闭中断，执行函数function(func_arg) */
static void kernel_thread(thread_func *function, void *func_arg) {
    function(func_arg);
}

/**
 * thread_create - 初始化线程堆栈。
 * @thread: 指向表示线程的 task_struct 的指针。
 * @function: 线程将要执行的函数。
 * @func_arg: 要传递给线程函数的参数。
 *
 * 此函数为新线程设置线程堆栈，将函数及其参数放置在线程堆栈中的适当位置。
 * 它还准备了堆栈以在 `kernel_thread` 处开始执行。
 */
void thread_create(struct task_struct *thread, thread_func function,void *func_arg) {
    /* 让 self_kstack 指向线程堆栈的顶部 */
    thread->self_kstack -= sizeof(struct intr_stack);
    thread->self_kstack -= sizeof(struct thread_stack);

    struct thread_stack *kthread_stack = (struct thread_stack *)thread->self_kstack;

    kthread_stack->ebp = kthread_stack->ebx = 0;
    kthread_stack->esi = kthread_stack->edi = 0;

    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
}

/* 初始化 PCB  */
void init_thread(struct task_struct *thread, char *name, int _priority) {
    /* 将 PCB 清零 */
    memset(thread, 0, sizeof(*thread));
    strcpy(thread->name, name);
    thread->status = TASK_RUNNING;
    thread->priority = _priority;
    /* 让堆栈指针指向高地址 */
    thread->self_kstack = (uint32_t *)((uint32_t)thread + PAGE_SIZE);
    thread->stack_magic = 0x20030807;
}

/*
 * thread_start - 创建一个优先级为_priority的新线程，线程名字为name
 */
struct task_struct *thread_start(char *name, int _priority,thread_func function, void *func_arg) {
    struct task_struct *thread = get_kernel_pages(1);
    init_thread(thread, name, _priority);
    thread_create(thread, function, func_arg);

    asm volatile (" movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" : : "g" (thread->self_kstack) : "memory");

    return thread;
}
#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "interrupt.h"
#include "switch.h"
#include "print.h"
#include "debug.h"
#include "list.h"
#define PAGE_SIZE 4096

struct task_struct *main_thread;       //主线程PCB
struct list thread_ready_list;         //就绪队列
struct list thread_all_list;           //所有任务队列
//static struct list_elem* thread_tag;   //保存队列中的线程节点

//extern void switch_to(struct task_struct* cur,struct task_struct* next);

/* 获取当前线程的 PCB 指针 */
struct task_struct *running_thread() {
    uint32_t esp;
    asm("mov %%esp,%0" : "=g"(esp));
    return (struct task_struct *)(esp & 0xfffff000);
}

/* 关闭中断，执行函数function(func_arg) */
static void kernel_thread(thread_func *function, void *func_arg) {
    intr_enable();
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
    
    if(thread == main_thread){
        thread->status = TASK_RUNNING;
    } else {
        thread->status = TASK_READY;
    }

    /* 让堆栈指针指向高地址 */
    thread->self_kstack = (uint32_t *)((uint32_t)thread + PAGE_SIZE);
    thread->priority = _priority;
    /* 优先级越大，时间片越长 */
    thread->ticks = _priority;
    thread->elapsed_ticks = 0;
    thread->pg_dir = NULL;

    thread->stack_magic = 0x20030807;
}

/*
 * thread_start - 创建一个优先级为_priority的新线程，线程名字为name
 */
struct task_struct *thread_start(char *name, int _priority,thread_func function, void *func_arg) {
    struct task_struct *thread = get_kernel_pages(1);
    init_thread(thread, name, _priority);
    thread_create(thread, function, func_arg);

    ASSERT(!list_elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);
    ASSERT(!list_elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);
    
    return thread;
}

/*
 * make_main_thread - 初始化操作系统的主线程的PCB信息
 */
static void make_main_thread(void) {
    /* 在 loader.S 中 mov esp,0xc009f000 已经预留了 PCB，故不需要分配，PCB 地址为 0xc009e000*/
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);

    ASSERT(!list_elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

/**
 * schedule - 使用先进先出调度选择下一个要运行的线程
 *
 * 从就绪列表中选择下一个要运行的任务并切换上下文到它。如果当前运行的线程
 * 已经用完了它的时间片或者正在等待，则将其追加回就绪列表以供稍后调度。
 * 在弹出一个任务之前，该函数确保就绪列表不为空，并确保当前线程在运行时
 * 不在就绪列表中。
 *
 * 上下文切换到下一个任务，并相应地更新当前任务和下一个任务的状态。
 */
void schedule() {
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct *cur_thread = running_thread();

    if (cur_thread->status == TASK_RUNNING) {
        /* 当前线程的时间片已经用完，加到就绪队列队尾 */

        /* 确保 cur_thread 不在 thread_ready_list 中 */
        ASSERT(!list_elem_find(&thread_ready_list, &cur_thread->general_tag));

        list_append(&thread_ready_list, &cur_thread->general_tag);
        cur_thread->ticks = cur_thread->priority;
        cur_thread->status = TASK_READY;
    } else {
        /* 其他事件，如线程阻塞、线程让出 */
    }

    ASSERT(!list_empty(&thread_ready_list));

    static struct list_elem* thread_tag;
    //thread_tag = NULL;
    thread_tag = list_pop(&thread_ready_list);
    /* 根据 general_tag 获取 PCB 的起始地址 */
    struct task_struct *next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    switch_to(cur_thread, next);
}

/**
 * thread_block - 阻塞当前正在运行的线程
 * @stat: 要分配给线程的新状态（BLOCKED、HANGING、WAITING）
 *
 * 将当前线程的状态更改为给定的状态，并重新调度。
 */
void thread_block(enum task_status stat) {
    enum intr_status old_status = intr_disable();

    ASSERT(stat == TASK_BLOCKED || stat == TASK_HANGING || stat == TASK_WAITING);

    struct task_struct *cur_thread = running_thread();
    cur_thread->status = stat;
    schedule();
    intr_set_status(old_status);
}

/**
 * thread_unblock - 解除指定线程的阻塞状态
 * @pthread: 要解除阻塞的线程指针
 *
 * 将给定的线程从阻塞状态移动到就绪状态，使其重新具备调度资格。
 */
void thread_unblock(struct task_struct *pthread) {
    enum intr_status old_status = intr_disable();

    ASSERT(pthread->status == TASK_BLOCKED || pthread->status == TASK_HANGING ||
            pthread->status == TASK_WAITING);
    
    //if(pthread->status != TASK_READY){
        //ASSERT(!list_elem_find(&thread_ready_list, &pthread->general_tag));
        if (list_elem_find(&thread_ready_list, &pthread->general_tag))
            PANIC("blocked thread in ready_list\n");
        list_push(&thread_ready_list, &pthread->general_tag);
        pthread->status = TASK_READY;
    //}
    intr_set_status(old_status);
}

void thread_init() {
    put_str("  thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    make_main_thread();
    put_str("  thread_init done\n");
}
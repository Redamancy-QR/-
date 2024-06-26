#include "process.h"
#include "console.h"
#include "debug.h"
#include "global.h"
#include "interrupt.h"
#include "list.h"
#include "memory.h"
#include "stdint.h"
#include "string.h"
#include "thread.h"
#include "tss.h"
#include "userprog.h"

extern void intr_exit(void);
extern struct list thread_ready_list;
extern struct list thread_all_list;

/**
 * start_process - 构建用户进程的上下文。该函数是“中断返回”地址
 * @_filename: 用户进程的文件名，也是进程的名称
 *
 * 该函数初始化用户进程的intr_stack，这是用户进程的上下文
 */

void start_process(void *_filename) {
    void *function = _filename;
    struct task_struct *cur_thread = running_thread();
    /* 让 self_kstack 跳过 thread_stack 并指向 intr_stack */
    cur_thread->self_kstack += sizeof(struct thread_stack);
    struct intr_stack *proc_stack = (struct intr_stack *)cur_thread->self_kstack;

    /* 为进程初始化 intr_stack。至于通用寄存器，目前暂未使用，
    * 因此将它们初始化为 0 */
    proc_stack->edi = proc_stack->esi = 0;
    proc_stack->ebp = proc_stack->esp_dummy = 0;
    proc_stack->ebx = proc_stack->edx = 0;
    proc_stack->ecx = proc_stack->eax = 0;

    proc_stack->gs = 0;
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;

    proc_stack->cs = SELECTOR_U_CODE;
    proc_stack->eip = function;

    proc_stack->eflags = (EFLAGS_IF_1 | EFLAGS_IOPL_0 | EFLAGS_MBS);

    proc_stack->ss = SELECTOR_U_DATA;

    /* 为用户进程分配优先级为3的栈 */
    proc_stack->esp =
        (void *)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PAGE_SIZE);

    /* 跳转到中断退出，以便CPU通过中断从高权限级别（操作系统内核）完成到低权限级别（用户进程）的转换 */
    asm volatile("movl %0,%%esp; jmp intr_exit" ::"g"(proc_stack) : "memory");
}

/**
 * page_dir_activate() - 激活给定线程的页目录。
 * @pthread: 指向需要激活页目录的线程的指针。
 *
 * 此函数将适当的页目录地址加载到CR3寄存器中。
 * 如果线程是用户进程，则使用其自己的页目录；否则，使用内核的页目录。
 */
void page_dir_activate(struct task_struct *pthread) {
    /* 内核线程 */
    uint32_t page_dir_phy_addr = 0x100000;
    /* 线程还是进程？ */
    if (pthread->pg_dir != NULL) {
        /* 进程，切换页目录（PD） */
        page_dir_phy_addr = addr_v2p((uint32_t)pthread->pg_dir);
    }
    asm volatile("movl %0, %%cr3" ::"r"(page_dir_phy_addr) : "memory");
}

/**
 * process_activate() - 激活进程并更新TSS。
 * @pthread: 指向要激活的进程的指针。
 *
 * 此函数为给定进程更新TSS中的ESP0栈指针，并通过调用page_dir_activate激活其页目录。
 */
void process_activate(struct task_struct *pthread) {
    ASSERT(pthread != NULL);
    page_dir_activate(pthread);
    if (pthread->pg_dir) {
        update_tss_esp(pthread);
    }
}

/**
 * create_page_dir() - 为用户进程创建一个页目录。
 *
 * 为用户进程分配并初始化一个新的页目录。它将内核条目复制到新的页目录中，并设置一个自引用。
 * 返回一个指针（vaddr）指向新创建的页目录。
 */

uint32_t *create_page_dir(void) {
    /* 为用户进程创建页目录 */
    uint32_t *user_page_dir_vaddr = get_kernel_pages(1);
    if (user_page_dir_vaddr == NULL) {
        console_put_str("create_page_dir: get_kernel_pages failed!");
        return NULL;
    }

    /* 通过将上部1GB（PDE.768~PDE.1023）映射到内核物理地址空间（页目录中的1024/4=256
    * 条目，即1GB），在用户虚拟地址空间中创建内核入口 */
    memcpy((uint32_t *)((uint32_t)user_page_dir_vaddr + 0x300 * 4), (uint32_t *)(0xfffff000 + 0x300 * 4), 1024);

    /* 让用户PDE的最后一项指向页目录本身 */
    uint32_t user_page_dir_phy_addr = addr_v2p((uint32_t)user_page_dir_vaddr);
    user_page_dir_vaddr[1023] = user_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
    return user_page_dir_vaddr;
}

/**
 * create_user_vaddr_bitmap() - 创建用于管理用户进程虚拟地址的位图。
 * @user_prog: 指向用户进程任务结构的指针。
 *
 * 初始化用于管理用户进程虚拟地址的位图，从预定义的用户虚拟地址（0x8048000）开始。
 */
void create_user_vaddr_bitmap(struct task_struct *user_prog) {
    user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;

    /* 用户位图所需的物理页面数量 */
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8, PAGE_SIZE);
    user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
    user_prog->userprog_vaddr.vaddr_bitmap.bmap_bytes_len = (0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8;
    bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}

/**
 * process_execute() - 创建一个新的用户进程。
 * @filename: 指向要执行的进程文件名的指针。
 * @name: 进程的名称。
 *
 * 此函数创建一个新的用户进程，初始化其线程结构，并将其添加到就绪队列和所有线程列表中。
 * 它还为用户进程创建必要的结构，如用户地址空间位图和页目录。
 */
void process_execute(void *filename, char *name) {
    /* 为用户进程创建PCB（本质上是一个线程）*/
    struct task_struct *user_thread = get_kernel_pages(1);
    ASSERT(user_thread != NULL);
    /* 初始化用户进程的PCB */
    init_thread(user_thread, name, default_prio);
    /* 为虚拟地址空间创建位图 */
    create_user_vaddr_bitmap(user_thread);
    /* 初始化线程栈 */
    thread_create(user_thread, start_process, filename);
    /* 创建用户进程的页目录以进行地址映射 */
    user_thread->pg_dir = create_page_dir();

    //block_desc_init(user_thread->u_mb_desc_arr);

    /* 准备运行 */
    enum intr_status old_status = intr_disable();
    ASSERT(!list_elem_find(&thread_ready_list, &user_thread->general_tag));
    list_append(&thread_ready_list, &user_thread->general_tag);

    ASSERT(!list_elem_find(&thread_all_list, &user_thread->all_list_tag));
    list_append(&thread_all_list, &user_thread->all_list_tag);
    intr_set_status(old_status);
}
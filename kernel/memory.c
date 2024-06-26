#include "memory.h"
#include "print.h"
#include "stdint.h"
#include "bitmap.h"
#include "debug.h"
#include "global.h"
#include "string.h"
#include "list.h"
#include "interrupt.h"
#include "string.h"
#include "thread.h"
#include "sync.h"
#define PAGE_SIZE 4096

/* 内核位图的虚拟地址 */
#define MEM_BITMAP_BASE 0xc009a000

/* 内核的虚拟地址从3G开始，并且需要跨越起始和使用的1MB，即0xc0000000 + 0x00100000 = 0xc0100000 */
#define KERNEL_HEAP_START 0xc0100000

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

/**
 * struct pool - 表示一个物理内存池。
 * @pool_bitmap: 用于跟踪内存池中页面分配状态的位图。
 * @phy_addr_start: 内存池的起始物理地址。
 * @pool_size: 内存池的总大小。
 * @_lock: 用于申请内存时的互斥。
 * 此结构用于管理物理内存池，无论是用于内核还是用户空间。
 * 它包括一个位图，用于高效地跟踪哪些页面是空闲的或已分配的，以及内存池的起始地址和总大小。
 */
struct pool {
    struct bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
    struct lock _lock;
};
/* 内核、用户的物理内存池 */
struct pool kernel_pool, user_pool;

/* 内核的虚拟内存池 */
struct virtual_addr kernel_vaddr;


/**
 * mem_pool_init() - 初始化内核和用户的物理和虚拟内存池。
 * @all_mem: 总物理内存大小。
 *
 * 此函数初始化内核和用户的内存池。
 * 它计算并将可用内存分配给内核和用户空间，考虑到内核已使用的内存。
 * 它初始化这些内存池的位图，以跟踪空闲和已使用的页面。
 * 该函数还设置内核和用户内存池的起始地址和大小。
 * 此外，它初始化了内核的虚拟地址池。
 *
 * 上下文: 在系统初始化期间应调用此函数，为内核和用户空间设置内存池。
 */
static void mem_pool_init(uint32_t all_mem) {
    put_str("     mem_pool_init start\n");

    lock_init(&kernel_pool._lock);
    lock_init(&user_pool._lock);
    
    /* 1 页目录表（PDT）+ 255 页表（PT）= 4KB * 256 = 1024KB = 1MB（0x100000B） */
    uint32_t page_table_size = PAGE_SIZE * 256;

    /* 0x100000 是内核使用的低1MB物理空间 */
    /* 因此 used_mem 的值为 2MB（0x200000）*/
    uint32_t used_mem = page_table_size + 0x100000;
    uint32_t free_mem = all_mem - used_mem;

    /* 忽略小于页面大小的物理内存 */
    uint16_t all_free_pages = free_mem / PAGE_SIZE;

    /* 分配给内核和用户内存池的空闲物理页 */
    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    /* 内核和用户内存池位图的长度 */
    uint32_t kernel_bitmap_len = kernel_free_pages / 8;
    uint32_t user_bitmap_len = user_free_pages / 8;

    /* 内核和用户内存池的起始地址*/
    uint32_t kernel_pool_start = used_mem;
    uint32_t user_pool_start = kernel_pool_start + kernel_free_pages * PAGE_SIZE;

    kernel_pool.phy_addr_start = kernel_pool_start;
    kernel_pool.pool_size = kernel_free_pages * PAGE_SIZE;
    kernel_pool.pool_bitmap.bmap_bytes_len = kernel_bitmap_len;
    kernel_pool.pool_bitmap.bits = (void *)MEM_BITMAP_BASE;

    user_pool.phy_addr_start = user_pool_start;
    user_pool.pool_size = user_free_pages * PAGE_SIZE;
    user_pool.pool_bitmap.bmap_bytes_len = user_bitmap_len;
    user_pool.pool_bitmap.bits = (void *)(MEM_BITMAP_BASE + kernel_bitmap_len);

    put_str("      kernel_pool_bitmap_start: ");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str("\n");
    put_str("      kernel_pool_phy_start:    ");
    put_int(kernel_pool.phy_addr_start);
    put_str("\n");

    put_str("      user_pool_bitmap_start:   ");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str("\n");
    put_str("      user_pool_phy_start:      ");
    put_int(user_pool.phy_addr_start);
    put_str("\n");

    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    /* 初始化内核虚拟地址位图，并按实际物理内存大小生成数组*/
    kernel_vaddr.vaddr_bitmap.bmap_bytes_len = kernel_bitmap_len;
    kernel_vaddr.vaddr_bitmap.bits = (void *)(MEM_BITMAP_BASE + kernel_bitmap_len + user_bitmap_len);
    kernel_vaddr.vaddr_start = KERNEL_HEAP_START;

    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("    mem_pool_init done\n");
}

/*
 * mem_init() - 内存管理初始化的入口点。
 *
 * 此函数标记了内存初始化的开始。首先打印一条消息表示内存初始化的开始。
 * 然后读取总内存大小，并使用此大小初始化内存池。
 * 最后，初始化内存块描述符数组，这对于 malloc 函数是至关重要的，并打印完成消息。
 *
 * 上下文: 此函数对于设置内存管理系统至关重要。它初始化内存池并准备用于动态内存分配的内存块描述符。
 * 返回值: 此函数不返回值。
 */
void mem_init() {
    put_str("  mem_init start\n");
    uint32_t mem_bytes_total = (*(uint32_t *)(0xb00));
    mem_pool_init(mem_bytes_total);
    put_str("  mem_init done\n");
}

/**
 * vaddr_get - 从虚拟内存池pf请求pg_cnt个虚拟页面。
 * @pf: 虚拟内存池。
 * @pg_cnt: 要申请的虚拟页面数量。
 *
 * 返回值: 如果成功，则返回虚拟页面的起始地址，否则返回NULL。
 */
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
    int vaddr_start = 0, free_bit_idx_start = -1;
    uint32_t cnt = 0;

    if (pf == PF_KERNEL) {
        free_bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if (free_bit_idx_start == -1)
            return NULL;
        while (cnt < pg_cnt) {
            bitmap_set(&kernel_vaddr.vaddr_bitmap, free_bit_idx_start + cnt++, 1);
        }
        vaddr_start = kernel_vaddr.vaddr_start + free_bit_idx_start * PAGE_SIZE;
    } else {
        /* 用户进程 */
        struct task_struct *cur = running_thread();
        free_bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap, pg_cnt);
        if (free_bit_idx_start == -1){
            return NULL;
        }
            
        while (cnt < pg_cnt) {
            bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, free_bit_idx_start + cnt++,1);
        }
        vaddr_start = cur->userprog_vaddr.vaddr_start + free_bit_idx_start * PAGE_SIZE;
        ASSERT((uint32_t)vaddr_start < (0xc0000000 - PAGE_SIZE));
    }
    return (void *)vaddr_start;
}

/**
 * pte_ptr - 计算给定虚拟地址的页表项的虚拟地址。
 * @vaddr: 要查找相应页表项的虚拟地址。
 *
 * 此函数计算提供的虚拟地址（'vaddr'）对应的页表项（PTE）的虚拟地址。
 * 它利用页面中的最后一个条目（vaddr的高10位应为1以访问此PDE）
 * 来访问页面目录本身，从而使得可以计算出用于访问可分页内的适当PTE的虚拟地址。
 *
 * 返回值: 给定虚拟地址对应的页表项的虚拟地址。
 */
uint32_t *pte_ptr(uint32_t vaddr) {
    uint32_t *pte = (uint32_t *)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}

/**
 * pde_ptr - 计算给定虚拟地址的页目录项的虚拟地址。
 * @vaddr: 要查找相应页目录项的虚拟地址。
 *
 * 此函数计算提供的虚拟地址（'vaddr'）对应的页目录项（PDE）的虚拟地址。
 * 它使用页面目录的最后一个条目来访问目录本身，从而确定可以用于访问正确PDE的虚拟地址。
 * 计算是通过从固定的高内存位置（0xfffff000）开始，并根据'vaddr'应用偏移量来到达特定的PDE。
 *
 * 返回值: 给定虚拟地址对应的页目录项的虚拟地址。
 */
uint32_t *pde_ptr(uint32_t vaddr) {
    uint32_t *pde = (uint32_t *)((0xfffff000) + PDE_IDX(vaddr) * 4);
    return pde;
}

/**
 * palloc - 从给定的内存池分配一个物理页面。
 * @m_pool: 指向要分配页面的内存池的指针。
 *
 * 从指定的物理内存池中分配一个物理页面。
 * 函数搜索内存池位图中的空闲位，将其标记为已使用，并返回分配的页面的物理地址。
 *
 * 返回值: 分配的物理页面的起始指针，如果没有可用的空闲页面则返回NULL。
 */
static void *palloc(struct pool *m_pool) {
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if (bit_idx == -1)
        return NULL;
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phy_addr = m_pool->phy_addr_start + bit_idx * PAGE_SIZE;
    return (void *)page_phy_addr;
}

/**
 * page_table_Add() - 在虚拟地址和物理地址之间建立映射关系。
 * @_vaddr: 虚拟地址。
 * @_page_phy_addr: 物理地址。
 *
 * 此函数为给定的虚拟地址设置页表项（PTE）。
 * 它涉及检查给定虚拟地址的页目录项（PDE）是否存在。
 * 如果PDE存在，表示存在页表，则函数继续创建PTE。
 * 在设置PTE之前，它确保PTE不存在。
 * 如果PDE不存在，则在内核空间为页表分配一个物理页面，
 * 初始化这个新的页表（设置为0），然后创建PTE。
 * 函数使用位操作来设置PDE和PTE中的正确标志以进行映射。
 *
 * 上下文: 此函数应在可以安全修改页表的上下文中调用。
 * 它假定内核页池已经初始化。
 * 返回值: 此函数不返回值。
 *
 * 注意: 此函数使用ASSERT来确保在设置新的PTE时PTE不存在。
 * 它还执行必要的位操作来设置页表条目中的标志。
 */
static void page_table_add(void *_vaddr, void *_page_phy_addr) {
    uint32_t vaddr = (uint32_t)_vaddr;
    uint32_t page_phy_addr = (uint32_t)_page_phy_addr;
    uint32_t *pde = pde_ptr(vaddr);
    uint32_t *pte = pte_ptr(vaddr);

    /* 通过位是否存在检查PDE是否存在 */
    if (*pde & 0x00000001) {
        /* PDE 存在，这意味着页表存在，所以只需创建 PTE */

        /* 确保 PTE 不存在 */
        ASSERT(!(*pte & 0x00000001));
        
        if(!(*pte & 0x00000001)){
            *pte = (page_phy_addr | PG_US_U | PG_RW_W | PG_P_1);
        } else {
            PANIC("pte repeat");
            *pte = (page_phy_addr | PG_US_U | PG_RW_W | PG_P_1);
        }
        
    } else {
        /* PDE 不存在，这意味着页表不存在，因此在 kernel_pool 中申请一个物理页面作为页表 */
        uint32_t pde_phy_addr = (uint32_t)palloc(&kernel_pool);
        *pde = (pde_phy_addr | PG_US_U | PG_RW_W | PG_P_1);
        /* memset 需要一个虚拟地址。通过 pte 的值获取页表的虚拟地址 */
        memset((void *)((int)pte & 0xfffff000), 0, PAGE_SIZE);

        ASSERT(!(*pte & 0x00000001));
        *pte = (page_phy_addr | PG_US_U | PG_RW_W | PG_P_1);
    }
}

/**
 * malloc_page() - 分配指定数量的页面空间。
 * @pf: 指示要使用哪个内存池的标志。
 * @pg_cnt: 要分配的页面数量。
 *
 * 此函数分配 'pg_cnt' 个虚拟内存页面，并建立虚拟页面和物理页面之间的映射关系。
 * 它确保请求不会超过总物理内存大小。
 * 函数首先分配虚拟页面，然后对于每个虚拟页面，它分配一个相应的物理页面，并设置页表项（PTE）和可能的页目录项（PDE）。
 * 如果在任何时候分配失败，函数将返回NULL。
 *
 * 上下文: 根据池标志，用于在内核或用户空间中分配内存。
 * 返回值: 如果成功，则返回已分配虚拟页面的起始地址，否则返回NULL。
 */
void *malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
    /* 确保由 pg_cnt 表示的内存大小不超过总物理内存大小。15MB/4KB = 3840 */
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);
    /* 分配虚拟页面 */
    void *vaddr_start = vaddr_get(pf, pg_cnt);
    if (vaddr_start == NULL)
        return NULL;

    uint32_t vaddr = (uint32_t)vaddr_start;
    uint32_t cnt = pg_cnt;
    struct pool *mem_pool = (pf & PF_KERNEL) ? &kernel_pool : &user_pool;

    /* 在相应的池中分配物理页面，即建立虚拟页面和物理页面之间的映射关系，即创建PTE（可能还有PDE） */
    while (cnt-- > 0) {
        void *page_phy_addr = palloc(mem_pool);
        if (page_phy_addr == NULL)
            return NULL;
        page_table_add((void *)vaddr, page_phy_addr);
        vaddr += PAGE_SIZE;
    }
    return vaddr_start;
}

/**
 * get_kernel_pages() - 分配内核页面并将其初始化为零。
 * @pg_cnt: 要分配的页面数量。
 *
 * 此函数是 malloc_page 的一个特定于内核内存分配的包装器。
 * 它使用 malloc_page 在内核空间中分配 'pg_cnt' 个页面。
 * 如果成功，然后将分配的内存初始化为零。
 * 这特别用于内核空间内存分配，其中需要初始化。
 *
 * 上下文: 当需要内核空间内存，并且需要将分配的内存初始化为零时使用。
 * 返回值: 如果成功，则返回已分配和初始化的虚拟页面的起始地址，否则返回NULL。
 */
void *get_kernel_pages(uint32_t pg_cnt) {
    void *vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if (vaddr != NULL)
        memset(vaddr, 0, pg_cnt * PAGE_SIZE);
    return vaddr;
}

/**
 * get_user_page - 分配用户空间页面
 * @pg_cnt: 要分配的4K页面数量
 * 返回：分配的用户空间内存的虚拟地址
 *
 * 在用户空间分配'pg_cnt'数量的4K页面，初始化分配的空间为零，并返回分配空间的虚拟地址。
 * 通过在用户内存池上获取锁来确保分配期间的互斥。
 */

void *get_user_page(uint32_t pg_cnt) {
    lock_acquire(&user_pool._lock);
    void *vaddr = malloc_page(PF_USER, pg_cnt);
    if (vaddr != NULL){
        memset(vaddr, 0, pg_cnt * PAGE_SIZE);
    }
    lock_release(&user_pool._lock);
    return vaddr;
}

/**
 * get_a_page - 将虚拟地址映射到物理页面
 * @pf: 池标志，指示页面是为用户还是内核
 * @vaddr: 要映射的虚拟地址
 * 返回：成功时返回虚拟地址‘vaddr’，失败时返回NULL
 *
 * 将给定的虚拟地址‘vaddr’映射到指定池‘pf’（用户或内核）的物理页面。
 * 该函数首先在内存池上获取锁，计算来自虚拟地址的位图索引，设置位图中相应的位以指示页面被使用，
 * 分配一个物理页面，并添加虚拟地址与物理页面之间的映射。返回前释放锁。如果物理页面分配失败，则返回NULL。
 */

void *get_a_page(enum pool_flags pf, uint32_t vaddr) {
    struct pool *mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
    lock_acquire(&mem_pool->_lock);
    struct task_struct *cur_thread = running_thread();
    int32_t bit_idx = -1;

    if (cur_thread->pg_dir != NULL && pf == PF_USER) {
        bit_idx = (vaddr - cur_thread->userprog_vaddr.vaddr_start) / PAGE_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&cur_thread->userprog_vaddr.vaddr_bitmap, bit_idx, 1);
    } else if (cur_thread->pg_dir == NULL && pf == PF_KERNEL) {
        bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PAGE_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx, 1);
    } else {
        PANIC("Unable to establish mapping between pf and vaddr");
    }
    void *page_phy_addr = palloc(mem_pool);
    if (page_phy_addr == NULL) {
        //lock_release(&mem_pool->_lock);
        return NULL;
    }
    page_table_add((void *)vaddr, page_phy_addr);
    lock_release(&mem_pool->_lock);
    return (void *)vaddr;
}

/**
 * addr_v2p - 将虚拟地址转换为物理地址
 * @vaddr: 要转换的虚拟地址
 * 返回：相应的物理地址
 *
 * 该函数使用页表条目将给定的虚拟地址转换为其对应的物理地址。
 * 它首先定位给定虚拟地址的页表条目，然后从中提取物理地址。
 * 函数结合页表条目中提取的物理页面帧地址的高20位与原始虚拟地址的低12位，形成完整的物理地址。
 */
uint32_t addr_v2p(uint32_t vaddr) {
    uint32_t *pte_phy_addr = pte_ptr(vaddr);
    return ((*pte_phy_addr & 0xfffff000) + (vaddr & 0x00000fff));
}

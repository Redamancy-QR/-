#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "bitmap.h"
#include "list.h"
#include "stdint.h"

#define PG_P_1  1 //页表项或页目录项存在属性位
#define PG_P_0  0
#define PG_RW_R 0 //读、执行
#define PG_RW_W 2 //读写、执行
#define PG_US_S 0 //系统级
#define PG_US_U 4 //用户级

#define MB_DESC_CNT 7

enum pool_flags { PF_KERNEL = 1, PF_USER = 2 };

/*
 * struct virtual_addr - 管理虚拟内存池。
 * @vaddr_bitmap: 用于跟踪虚拟地址分配状态的位图。
 * @vaddr_start: 内存池的起始虚拟地址。
 *
 * 此结构用于管理虚拟内存池，对于操作系统中的虚拟内存管理至关重要。
 * 它包括一个位图，用于跟踪池内已分配和空闲的虚拟地址。
 * 'vaddr_start' 成员表示此池管理的虚拟地址空间的起始位置。
 * 此结构有助于动态和高效地分配虚拟地址，确保对虚拟内存空间的正确管理。
 */
struct virtual_addr {
    struct bitmap vaddr_bitmap;
    uint32_t vaddr_start;
};


extern struct pool kernel_pool, user_pool;
void mem_init();
void *get_kernel_pages(uint32_t pg_cnt);
void *get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);

#endif
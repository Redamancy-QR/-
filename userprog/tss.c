#include "tss.h"
#include "global.h"
#include "print.h"
#include "stdint.h"
#include "string.h"
#include "thread.h"

#define PAGE_SIZE 4096

struct tss {
    uint32_t backlink;
    uint32_t *esp0;
    uint32_t ss0;
    uint32_t *esp1;
    uint32_t ss1;
    uint32_t *esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t (*eip)(void);
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t io_base;
};
static struct tss tss;

/**
 * update_tss_esp() - 更新TSS中的ESP0字段。
 * @pthread: 指向task_struct的指针，其0级栈将用于ESP0。
 *
 * 该函数更新任务状态段（TSS）的ESP0字段，使其指向给定任务结构（pthread）的0级栈顶。
 * 这对于正确的上下文切换至关重要，特别是在从用户模式切换到内核模式时。
 */

void update_tss_esp(struct task_struct *pthread) {
    tss.esp0 = (uint32_t *)((uint32_t)pthread + PAGE_SIZE);
}

/**
 * make_gdt_desc() - 创建全局描述符表（GDT）的描述符。
 * @desc_addr: 描述符的基地址。
 * @limit: 段的限制。
 * @attr_low: 段属性的低8位。
 * @attr_high: 段属性的高8位。
 *
 * 返回一个表示GDT描述符的结构。该描述符包括段的基地址、限制和属性。
 * 此函数用于在GDT中设置段，例如代码段、数据段和TSS段。
 */
static struct gdt_desc make_gdt_desc(uint32_t *desc_addr, uint32_t limit,uint8_t attr_low, uint8_t attr_high) {
    uint32_t desc_base = (uint32_t)desc_addr;
    struct gdt_desc desc;
    desc.limit_low_word = limit & 0x0000ffff;
    desc.limit_high_attr_high = (((limit & 0x000f0000) >> 16) + (uint8_t)(attr_high));
    desc.base_low_word = desc_base & 0x0000ffff;
    desc.base_mid_byte = ((desc_base & 0x00ff0000) >> 16);
    desc.base_high_byte = desc_base >> 24;
    desc.attr_low_byte = (uint8_t)attr_low;
    return desc;
}

/**
 * tss_init() - 初始化任务状态段（TSS）并加载GDT。
 *
 * 该函数使用默认值初始化TSS，包括设置堆栈段和I/O位图。
 * 它还在GDT中为TSS以及DPL 3代码段和数据段创建描述符。
 * 最后，它加载新的GDT并设置任务寄存器以使用新的TSS。
 * 该函数对于建立任务切换和权限级别变更的工作环境至关重要。
 */
void tss_init() {
    put_str("  tss_init start\n");
    uint32_t tss_size = sizeof(tss);
    memset(&tss, 0, tss_size);
    tss.ss0 = SELECTOR_KERNEL_STACK;
    tss.io_base = tss_size;

    *((struct gdt_desc *)0xc0000920) = make_gdt_desc((uint32_t *)&tss, tss_size - 1, TSS_ARRT_LOW, TSS_ATTR_HIGH);
    /* 代码/数据段的大小为4GB（2^20 * 2^12 = 2^32） */
    *((struct gdt_desc *)0xc0000928) = make_gdt_desc((uint32_t *)0, 0xfffff, GDT_CODE_ATTR_LOW_WITH_DPL3, GDT_ATTR_HIGH);
    *((struct gdt_desc *)0xc0000930) = make_gdt_desc((uint32_t *)0, 0xfffff, GDT_DATA_ATTR_LOW_WITH_DPL3, GDT_ATTR_HIGH);

    uint64_t lgdt_operand = ((8 * 7 - 1) | ((uint64_t)(uint32_t)0xc0000900 << 16));
    asm volatile("lgdt %0" ::"m"(lgdt_operand));
    asm volatile("ltr %w0" ::"r"(SELECTOR_TSS));
    put_str("  tss_init done\n");
}

#include "interrupt.h"
#include "global.h"
#include "io.h"
#include "print.h"
#include "stdint.h"

/* 主 8259A 芯片的控制（CTRL）端口和数据（DATA）端口 */
#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
/* 从 8259A 芯片的控制（CTRL）端口和数据（DATA）端口 */
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

/* 
 * 中断描述符的总数
 */
#define IDT_DESC_COUNT 0x21


/**
 * 结构体 gate_desc - 中断描述符表条目（即门）结构体。
 * @func_offset_low_word: 函数偏移量的0~15位。
 * @selector: 目标代码段的选择子。
 * @dcount: 未使用的部分。
 * @attribute: 字段TYPE、S、DPL、P。
 * @func_offset_high_word: 函数偏移量的16~31位。
 * 总共8字节。
 */
struct gate_desc {
    /* 低32位 */
    uint16_t func_offset_low_word;
    uint16_t selector;

    /* 高32位 */
    uint8_t dcount;
    uint8_t attribute;
    uint16_t func_offset_high_word;
};
/* 中断描述符表 */
static struct gate_desc idt[IDT_DESC_COUNT];

/* 中断处理函数入口地址数组 */
extern intr_handler intr_entry_table[IDT_DESC_COUNT];


/**
 * pic_init - 设置主/从 8259A 芯片。
 * 此函数设置寄存器 ICW1~4 和 OCW1~4。
 */
static void pic_init() {
    /* 主芯片的 ICW1：边沿触发，级联8259，需要ICW4 */
    outb(PIC_M_CTRL, 0x11);
    /* 主芯片的 ICW2: 将起始中断号设置为 0x20 */
    outb(PIC_M_DATA, 0x20);
    /* 主芯片的 ICW3: 设置 IR2 以链接从芯片 */
    outb(PIC_M_DATA, 0x04);
    /* 主芯片的 ICW4: 设置无 AEOI，手动发送 EOI */
    outb(PIC_M_DATA, 0x01);

    /* 从芯片的 ICW1：边沿触发，级联8259，需要ICW4 */
    outb(PIC_S_CTRL, 0x11);
    /* 从芯片的 ICW2: 将起始中断号设置为 0x28 */
    outb(PIC_S_DATA, 0x28);
    /* 从芯片的 ICW3：设置从片连接到主片的IR2引脚 */
    outb(PIC_S_DATA, 0x02);
    /* 从芯片的 ICW4：设置无 AEOI，手动发送 EOI*/
    outb(PIC_S_DATA, 0x01);

    /* 打开主片上的IR0，也就是目前只接受时钟产生的中断 */
    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);

    put_str("  pic_init done\n");
}


/**
 * make_idt_desc - 构造中断描述符
 * @pt_gdesc: 指向中断门描述符的指针
 * @attr: 描述符属性
 * @function: 中断处理程序的地址
 * 此函数将属性和函数写入由pt_gdesc指向的描述符中。
 */
static void make_idt_desc(struct gate_desc *pt_gdesc, uint8_t attr, intr_handler function) {
    pt_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    pt_gdesc->selector = SELECTOR_KERNEL_CODE;
    pt_gdesc->dcount = 0;
    pt_gdesc->attribute = attr;
    pt_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

/**
 * idt_desc_init - 初始化中断描述符表
 * 此函数通过调用函数make_idt_desc来填充IDT的中断描述符条目。
 */
static void idt_desc_init() {
    int i;
    for (i = 0; i < IDT_DESC_COUNT; i++) 
    {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }

    put_str("  idt_desc_init done\n");
}

/**
 * idt_init - 完成有关中断的所有初始化工作
 */
void idt_init() {
    put_str("idt_init start\n");
    idt_desc_init();
    pic_init();

    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
    asm volatile("lidt %0" ::"m"(idt_operand));
    put_str("idt_init done\n");
}
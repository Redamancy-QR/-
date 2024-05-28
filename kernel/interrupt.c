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

/* eflags 寄存器中的 if 位为 1 */
#define EFLAGS_IF 0x00000200
#define GET_EFLAGS(EFLAGS_VAR) asm volatile("pushfl;popl %0" : "=g"(EFLAGS_VAR))

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

/* 用于保存异常的名字 */
char *intr_name[IDT_DESC_COUNT];

/* 中断处理程序地址表 */
intr_handler idt_table[IDT_DESC_COUNT];

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

    put_str("    pic_init done\n");
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

    put_str("    idt_desc_init done\n");
}

/*
 * general_intr_handler - 打印中断向量号
 * @vec_nr: 要打印的中断向量号
 *
 * 这个函数是所有中断的默认通用的中断处理程序,一般在异常出现时处理。
 */
static void general_intr_handler(uint8_t vec_nr) {
    /* 
    * 忽略虚假中断 */
    if (vec_nr == 0x27 || vec_nr == 0x2f)
        return;

    put_str("int vector : 0x");
    put_int(vec_nr);
    put_char('\n');
}

/*
 * exception_init - 初始化 idt 表,完成一般中断处理函数注册及异常名称注册
 *
 * 这个函数将所有中断的默认中断处理程序函数设置为 general_intr_handler。
 */
static void exception_init() {
    int i;
    for (i = 0; i < IDT_DESC_COUNT; ++i) {
        idt_table[i] = general_intr_handler;
        intr_name[i] = "unknown";
    }

    intr_name[0]  = "#DE Divide Error";
    intr_name[1]  = "#DB Debug";
    intr_name[2]  = "NMI Interrupt";
    intr_name[3]  = "#BP BreakPoint";
    intr_name[4]  = "#OF Overflow";
    intr_name[5]  = "#BR BOUND Range Exceeded";
    intr_name[6]  = "#UD Undefined Opcode";
    intr_name[7]  = "#NM Device Not Available";
    intr_name[8]  = "#DF Double Fault";
    intr_name[9]  = "#MF CoProcessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Segment Fault";
    intr_name[13] = "#GP General  Protection";
    intr_name[14] = "#PF Page Fault";
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check";
    intr_name[18] = "#MC Machine Check";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}

/**
 * idt_init - 完成有关中断的所有初始化工作
 */
void idt_init() {
    put_str("  idt_init start\n");
    idt_desc_init();  //初始化中断描述符表
    exception_init(); //中断处理函数注册及异常名称注册
    pic_init();       //初始化8259A

    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
    asm volatile("lidt %0" ::"m"(idt_operand));
    put_str("  idt_init done\n");
}

/**
 * intr_enable - 启用中断
 *
 * 此函数使用 `sti` 指令将 eflag 寄存器中的 "if" 位设置为 1。
 * 返回: 旧的中断状态。
 */
enum intr_status intr_enable() {
    enum intr_status old_status;
    if (INTR_ON == intr_get_status()) {
        old_status = INTR_ON;
    } else {
        old_status = INTR_OFF;
        asm volatile("sti");
    }
    return old_status;
}

/**
 * intr_disable - 禁用中断
 *
 * 此函数使用 `cli` 指令将 eflag 寄存器中的 "if" 位设置为 0。
 * 返回: 旧的中断状态。
 */
enum intr_status intr_disable() {
    enum intr_status old_status;
    if (INTR_ON == intr_get_status()) {
        old_status = INTR_ON;
        asm volatile("cli" : : : "memory");
    } else {
        old_status = INTR_OFF;
    }
    return old_status;
}

/**
 * intr_set_status - 将中断状态设置为参数 status 的值
 * @status: 要设置的中断状态。
 *
 * 返回: 旧的中断状态
 */
enum intr_status intr_set_status(enum intr_status status) {
    return status & INTR_ON ? intr_enable() : intr_disable();
}

/**
 * intr_get_status - 通过检查 IF 标志获取当前中断状态
 *
 * 返回: 中断状态。相关的枚举变量在 interrupt.h 中定义。
 */
enum intr_status intr_get_status() {
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}
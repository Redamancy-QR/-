#include "timer.h"
#include "io.h"
#include "print.h"

#define IRQ0_FREQUENCY    100
#define INPUT_FREQUENCY   1193180
#define COUNTER0_VALUE    INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT     0x40
#define COUNTER0_NO       0
#define COUNTER0_MODE     2
#define READ_WRITE_LATCH  3
#define PIT_CONTROL_PORT  0x43

/**
 * frequency_set - 初始化可编程间隔定时器 Intel 8253
 * @counter_port: 对于计数器编号 0，此值为 0x40
 * @counter_no: 要设置的计时器序列号，在这里设置为 0
 * @rwl: 控制字的读/写/锁存位在 Intel 8253 中的值
 * @counter_mode: 计数器的工作模式，在这里设置为 2，表示周期性计数
 * @counter_value: 计数器0的初始计数值
 *
 * 这个函数为 Intel 8253 中的计数器 NO.0 设置适当的值。
 */
static void frequency_set(  uint8_t counter_port, 
                            uint8_t counter_no, 
                            uint8_t rwl,
                            uint8_t counter_mode, 
                            uint16_t counter_value) {
    /* 往控制寄存器端口0x43写入控制字 */
    outb(PIT_CONTROL_PORT,(uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
    /* 计数器0初始计数值的低8位 */
    outb(counter_port, (uint8_t)counter_value);
    /* 计数器0初始计数值的高8位 */
    outb(counter_port, (uint8_t)counter_value >> 8);
}

/*
 * timer_init - 初始化定时器
 *
 * 这个函数封装了 frequency_set 函数。因此，外部函数可以轻松地调用 timer_init 完成定时器的初始化。
 * 此外，这个函数重新注册了时钟中断处理程序函数。
 */
void timer_init() {
    put_str("  timer_init start\n");
    frequency_set(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER0_MODE,COUNTER0_VALUE);
    put_str("  timer_init done\n");
}
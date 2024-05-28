#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H
#include "stdint.h"
typedef void *intr_handler;
void idt_init();
/**
 * 中断的两种状态:
 * @INTR_OFF: 中断关闭，IF 等于 0
 * @INTR_ON:  中断开启，IF 等于 1
 */
enum intr_status { INTR_OFF, INTR_ON };

enum intr_status intr_get_status();
enum intr_status intr_set_status(enum intr_status status);
enum intr_status intr_enable();
enum intr_status intr_disable();
#endif
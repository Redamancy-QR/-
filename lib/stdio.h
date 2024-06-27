/*
 * Author: Zhang Xun
 * Time: 2023-11-28
 */
#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H
#include "stdint.h"
typedef char *va_list;

/* va_start - 初始化 va_list 以处理可变参数列表。
 * @ap: 要初始化的 va_list。
 * @v: 可变参数列表之前的最后一个固定参数。
 *
 * 该宏将 ap 初始化为指向可变参数函数参数列表中的第一个参数，
 * 以便随后通过 va_arg 和 va_end 访问。
 */
#define va_start(ap, v) ap = (va_list)&v

/* va_arg - 从 va_list 中获取下一个参数。
 * @ap: 要从中获取参数的 va_list。
 * @t: 要获取的下一个参数的类型。
 *
 * 该宏访问 va_list 中的下一个参数，并递增 ap 以指向后续参数。
 * 参数的类型由 t 指定。
 */
#define va_arg(ap, t) *((t *)(ap += 4))

/* va_end - 处理完可变参数列表后清理 va_list。
 * @ap: 要清理的 va_list。
 *
 * 该宏在处理完可变参数列表后执行必要的清理操作。应在函数返回之前调用。
 */
#define va_end(ap) ap = NULL

uint32_t printf(const char *format, ...);
uint32_t vsprintf(char *str, const char *format, va_list ap);
uint32_t sprintf(char *buf, const char *format, ...);

#endif
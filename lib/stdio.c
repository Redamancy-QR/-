/*
 * Author: Zhang Xun
 * Time: 2023-11-28
 */
#include "stdio.h"

#include "global.h"
#include "stdint.h"
#include "string.h"
#include "syscall.h"

/* itoa - 将整数值转换为指定进制的字符串。
 * @value: 要转换的整数值。
 * @buf_ptr_addr: 指向存储转换后字符串的缓冲区的指针。
 * @base: 转换的进制（例如，10 表示十进制，16 表示十六进制）。
 *
 * 将给定的整数（value）转换为指定进制的字符串表示，并将其存储在提供的缓冲区中。
 */
static void itoa(uint32_t value, char **buf_ptr_addr, uint8_t base) {
    uint32_t m = value % base;
    uint32_t i = value / base;
    if (i) {
        itoa(i, buf_ptr_addr, base);
    }
    if (m < 10) {
        *((*buf_ptr_addr)++) = m + '0';
    } else {
        *((*buf_ptr_addr)++) = m - 10 + 'A';
    }
}

/* vsprintf - 格式化字符串并将其存储在提供的缓冲区中。
 * @str: 用于存储格式化字符串的缓冲区。
 * @format: 指定所需输出的格式字符串。
 * @ap: 提供要格式化的值的可变参数列表。
 *
 * 此函数根据指定的格式和参数列表格式化字符串，并将结果存储在 str 中。
 * 它在内部由 printf 和类似函数使用。返回格式化字符串的长度。
 */
uint32_t vsprintf(char *str, const char *format, va_list ap) {
    char *buf_ptr = str;
    const char *iter = format;
    char ch = *iter;
    int32_t arg_int;
    char *arg_str;
    while (ch) {
        if (ch != '%') {
            *(buf_ptr++) = ch;
            ch = *(++iter);
            continue;
        }
        
        ch = *(++iter);
        switch (ch) {
        case 'x':
            arg_int = va_arg(ap, int);
            itoa(arg_int, &buf_ptr, 16);
            ch = *(++iter);
            break;
        case 'c':
            *(buf_ptr++) = va_arg(ap, char);
            ch = *(++iter);
            break;
        case 'd':
            arg_int = va_arg(ap, int);
            if (arg_int < 0) {
                arg_int = 0 - arg_int;
                *(buf_ptr++) = '-';
            }
            itoa(arg_int, &buf_ptr, 10);
            ch = *(++iter);
            break;
        case 's':
            arg_str = va_arg(ap, char *);
            strcpy(buf_ptr, arg_str);
            buf_ptr += strlen(arg_str);
            ch = *(++iter);
            break;
        }
    }
    return strlen(str);
}

/**
 * sprintf - 格式化字符串并将其存储在缓冲区中。
 * @buf: 存储格式化字符串的缓冲区。
 * @format: 指定数据格式的格式字符串。
 * ...: 要格式化的可变数量的参数。
 *
 * 此函数接受一个格式字符串和可变数量的参数，
 * 根据 'format' 中的格式说明符将它们格式化为字符串，
 * 然后将结果存储在 'buf' 中。缓冲区必须足够大以容纳生成的字符串。
 *
 * 返回值: 存储在 'buf' 中的字符数量（不包括空终止符）。
 */
uint32_t sprintf(char *buf, const char *format, ...) {
    va_list args;
    uint32_t ret_val;
    va_start(args, format);
    ret_val = vsprintf(buf, format, args);
    va_end(args);
    return ret_val;
}

/* printf - 格式化并打印字符串到标准输出。
 * @format: 指定所需输出的格式字符串。
 *
 * 此函数接受一个格式字符串和可变数量的参数，
 * 将它们格式化为字符串，并将字符串打印到标准输出。
 * 返回打印的字符数量。
 */
uint32_t printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buf[1024] = {0};
    vsprintf(buf, format, args);
    va_end(args);
    return write(buf);
}
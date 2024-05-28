#include "debug.h"
#include "interrupt.h"
#include "print.h"

/**
 * panic_spin - 打印错误消息并终止当前程序
 * @filename: 发生错误的文件
 * @line: 发生错误的行号
 * @func: 发生错误的函数名
 * @func: assert 中的条件语句
 *
 * 当 assert 宏中的表达式为假时调用此函数。
 */
void panic_spin(char *filename, int line, const char *func,const char *condition) {
    intr_disable();
    put_str("\n\n\n!!!!!!error!!!!!!\n");

    put_str("filename: ");
    put_str(filename);
    put_str("\n");

    put_str("line: 0x");
    put_int(line);
    put_str("\n");

    put_str("function: ");
    put_str((char *)func);
    put_str("\n");

    put_str("condition: ");
    put_str((char *)condition);
    put_str("\n");
    while (1);
}
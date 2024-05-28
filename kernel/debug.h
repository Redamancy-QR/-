#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H
void panic_spin(char *filename, int line, const char *func,const char *condition);

#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)

/*
 * ASSERT(CONDITION) - 一个类似函数的宏
 * @CONDITION: 可以转换为布尔类型的表达式
 *
 * 此类似函数的宏用于调试程序，当表达式为假时触发 panic_spin 函数，否则什么也不做。
 * 如果当前文件中定义了 NDEBUG，assert 就变得无意义（什么也不做）。
 *
 */
#ifdef NDEBUG
    #define ASSERT(CONDITION) ((void)0)
#else
#define ASSERT(CONDITION)                                                      \
    if (CONDITION) {                                                             \
    } else {                                                                     \
        PANIC(#CONDITION);                                                         \
    }
#endif /*__NDEBUG*/

#endif /*__KERNEL_DEBUG_H*/
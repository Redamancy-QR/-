#include "string.h"
#include "debug.h"
#include "global.h"

/**
 * memset - 将从dst开始的size字节设置为value
 * @dst: 指向起始地址的指针
 * @value: 要设置的值
 * @size: 要设置的字节数
 *
 * 此函数不安全。size 不能超过dst可以访问的范围，否则将导致非法内存访问。
 */
void memset(void *dst, uint8_t value, uint32_t size) {
    ASSERT(dst != NULL);
    uint8_t *dst_in_byte = (uint8_t *)dst;
    while (size-- > 0){
        *dst_in_byte++ = value;
    }
}

/**
 * memcpy - 将src指向的地址的size字节复制到dest指向的地址
 * @dst: 复制的目的地
 * @src: 复制的来源
 * @size: 要复制的字节数
 *
 * 此函数不安全，如果src的长度大于dst，将导致非法内存访问。
 */
void memcpy(void *dst, const void *src, uint32_t size) {
    ASSERT(dst != NULL && src != NULL);
    uint8_t *dst_in_byte = (uint8_t *)dst;
    const uint8_t *src_in_byte = (uint8_t *)src;
    while (size-- > 0) {
        *dst_in_byte++ = *src_in_byte++;
    }
}

/**
 * memcmp - 比较指针a和指针b所指向的内存块的前size个字节
 * @a: 指向第一个内存块起始地址的指针
 * @b: 指向第二个内存块起始地址的指针
 *
 * 返回值: 如果全部匹配则返回零，如果不匹配则返回非零值，表示哪个大。
 */
int memcmp(const void *a, const void *b, uint32_t size) {
    ASSERT(a != NULL && b != NULL);
    const char *a_in_char = a;
    const char *b_in_char = b;
    while (size-- > 0) {
        if (*a_in_char != *b_in_char)
            return *a_in_char > *b_in_char ? 1 : -1;
        ++a_in_char;
        ++b_in_char;
    }
    return 0;
}

/**
 * strcpy - 将由src指向的C字符串复制到由dst指向的数组中，包括终止的空字符
 * @dst: 指向要复制内容的目标数组的指针
 * @src: 指向要复制的以空字符结尾的字节字符串的指针
 *
 * 返回值: 指向目标字符串dst的指针。
 */
char *strcpy(char *dst, const char *src) {
    ASSERT(dst != NULL && src != NULL);
    char *ret = dst;
    while ((*dst++ = *src++));
    return ret;
}

/**
 * strlen - 计算由str指向的字符串的长度，不包括终止的空字节('\0')
 * @str: 指向要测量长度的以空字符结尾的字节字符串的指针
 *
 * 返回值: str指向的字符串中的字符数。
 */
uint32_t strlen(const char *str) {
    ASSERT(str != NULL);
    const char *p = str;
    while (*p++);
    return (p - str - 1);
}

/**
 * strcmp - 比较指针a和指针b所指向的两个字符串
 * @a: 指向要比较的第一个以空字符结尾的字节字符串的指针
 * @b: 指向要比较的第二个以空字符结尾的字节字符串的指针
 *
 * 返回值: 如果字符串a小于b，则返回小于零的整数；如果字符串a等于b，则返回零；
 * 如果字符串a大于b，则返回大于零的整数。
 */
int8_t strcmp(const char *a, const char *b) {
    ASSERT(a != NULL && b != NULL);
    while (*a != 0 && *a == *b) {
        ++a;
        ++b;
    }
    return *a < *b ? -1 : *a > *b;
}

/**
 * strchr - 在由str指向的字符串中定位字符ch（转换为char）的第一次出现
 * @str: 指向要分析的以空字符结尾的字节字符串的指针
 * @ch: 要搜索的字符
 *
 * 返回值: 指向字符串str中字符ch的第一次出现的指针，如果未找到字符，则返回NULL。
 */
char *strchr(const char *str, const uint8_t ch) {
    ASSERT(str != NULL);
    while (*str != 0){
        if(*str == ch){
            return (char *)str;
        }
        ++str;
    }
    return NULL;
}

/**
 * strrchr - 在由str指向的字符串中定位字符ch（转换为char）的最后一次出现
 * @str: 指向要扫描的以空字符结尾的字节字符串的指针
 * @ch: 要搜索的字符
 *
 * 返回值: 指向字符串str中字符ch的最后一次出现的指针，如果未找到字符，则返回NULL。
 */
char *strrchr(const char *str, const uint8_t ch) {
    ASSERT(str != NULL);
    const char *last_ch = NULL;
    while (*str != 0) {
        if (*str == ch){
            last_ch = str;
        }
        ++str;
    }
    return (char *)last_ch;
}

/**
 * strcat - 将由src指向的字符串追加到由dst指向的字符串的末尾。它会覆盖dst末尾的空字节('\0')，然后添加一个终止的空字节
 * @dst: 指向目标数组的指针，该数组应包含一个C字符串，并且足够大以容纳连接后的结果字符串
 * @src: 指向要追加的以空字符结尾的字节字符串的指针
 *
 * 返回值: 指向结果字符串dst的指针。
 */
char *strcat(char *dst, const char *src) {
    ASSERT(dst != NULL && src != NULL);
    char *str = dst;
    while (*str++);
    --str;
    while ((*str++ = *src++));
    return dst;
}

/**
 * strchrs - 计算指针src指向的字符串中字符ch的出现次数
 * @src: 指向要扫描的以空字符结尾的字节字符串的指针
 * @ch: 要计数的字符
 *
 * 返回值: 字符串src中字符ch出现的次数。
 */
uint32_t strchrs(const char *src, uint8_t ch) {
    ASSERT(src != NULL);
    uint32_t ch_cnt = 0;
    const char *p = src;
    while (*p != 0) {
        if (*p == ch){
            ch_cnt++;
        }
        p++;
    }
    return ch_cnt;
}
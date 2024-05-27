#ifndef _LIB_KERNEL_IO_H
#define _LIB_KERNEL_IO_H

#include "stdint.h"

/**
 * outb - 向端口写入一个字节的数据
 * @port: 16位，对应于Intel支持的最大端口号（65536）。
 * @data: 要写入的数据
 */
static inline void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %b0, %w1" : : "a"(data), "Nd"(port));
}

/**
 * outsw - 从内存读取word_cnt个字（由 ds:esi 指向）并将数据写入端口
 * @port: 要写入的端口
 * @addr: 内存的起始地址
 * @word_cnt: 要读取和写入的字数
 */
static inline void outsw(uint16_t port, const void *addr, uint32_t word_cnt) {
    asm volatile("cld; rep outsw" : "+S"(addr), "+c"(word_cnt) : "d"(port));
}

/** 
 * inb - 从端口读取一个字节的数据并将数据写入内存
 * @port: 要读取的端口
 *
 * 返回：从端口读取的数据。
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    asm volatile("inb %w1,%b0" : "=a"(data) : "Nd"(port));
    return data;
}

/**
 * insw - 从端口port读取word_cnt个字并将数据写入内存addr（由 es:edi 指向）
 * @port: 要读取的端口
 * @addr: 内存的起始地址
 * @word_cnt: 要读取和写入的字数
 */
static inline void insw(uint16_t port, void *addr, uint32_t word_cnt) {
    asm volatile("cld; rep insw": "+D"(addr), "+c"(word_cnt): "d"(port) : "memory");
}

#endif
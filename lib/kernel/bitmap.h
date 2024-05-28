#ifndef __LIb_KERNEL_BITMAP_H
#define __LIb_KERNEL_BITMAP_H

#include "global.h"
#include "stdint.h"

#define BITMAP_MASK 1

/**
 * struct bitmap - 表示位图数据结构。
 * @bmap_bytes_len: 位图的字节长度。
 * @bits: 指向表示位图的字节数组的指针。
 *
 * 此结构用于管理位图，一种有效表示二进制数据的位集合。
 */
struct bitmap {
    uint32_t bmap_bytes_len;
    uint8_t *bits;
};

void bitmap_init(struct bitmap *btmp);
bool bitmap_bit_test(struct bitmap *btmp, uint32_t bit_idx);
int  bitmap_scan(struct bitmap *btmp, uint32_t cnt);
void bitmap_set(struct bitmap *btmp, uint32_t bit_idx, int8_t value);

#endif
#include "bitmap.h"
#include "stdint.h"
#include "debug.h"
#include "interrupt.h"
#include "print.h"
#include "string.h"

/**
 * bitmap_init - 初始化位图。
 * @btmp: 要初始化的位图的指针。
 *
 * 将提供的位图中的所有位设置为0。
 */
void bitmap_init(struct bitmap *btmp) {
    memset(btmp->bits, 0, btmp->bmap_bytes_len);
}

/**
 * bitmap_bit_test - 在位图中测试特定索引处的位。
 * @btmp: 位图的指针。
 * @bit_idx: 要测试的位的索引。
 *
 * 返回值: 如果位被设置（为1），则返回True，否则返回false。
 */
bool bitmap_bit_test(struct bitmap *btmp, uint32_t bit_idx) {
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_idx_in_byte = bit_idx % 8;

    return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_idx_in_byte));
}

/**
 * bitmap_scan - 在位图中扫描一系列未设置的连续位。
 * @btmp: 位图的指针。
 * @cnt: 要查找的连续未设置（0）位的数量。
 *
 * 扫描至少长为'cnt'位的未设置位序列。
 *
 * 返回值: 如果找到序列，则返回其起始索引，否则返回-1。
 */
int bitmap_scan(struct bitmap *btmp, uint32_t cnt) {
    uint32_t byte_idx = 0;
    /* 通过按字节进行测试以排除全部为1的字节 */
    while ((0xff == btmp->bits[byte_idx]) && (byte_idx < btmp->bmap_bytes_len))
        ++byte_idx;

    ASSERT(byte_idx < btmp->bmap_bytes_len);
    /* 所有字节均为1，因此没有位为0，返回-1 */
    if (byte_idx == btmp->bmap_bytes_len)
        return -1;

    /* 通过逐位比较来查找第一个字节中的位0的位置。 */
    int bit_idx = 0;
    while ((uint8_t)(BITMAP_MASK << bit_idx) & btmp->bits[byte_idx])
        ++bit_idx;

    /* free_bit_idx_start 是整个位图中第一个位0的索引 */
    int free_bit_idx_start = byte_idx * 8 + bit_idx;
    if (cnt == 1)
        return free_bit_idx_start;

    uint32_t bit_left = btmp->bmap_bytes_len * 8 - free_bit_idx_start;
    uint32_t next_bit = free_bit_idx_start + 1;
    uint32_t count = 1;
    free_bit_idx_start = -1;

    /* 从 free_bit_idx_start 后的位开始遍历位图中的位，并找到序列 */
    while (bit_left-- > 0) {
        if (!bitmap_bit_test(btmp, next_bit)) {
            ++count;
        } else {
            count = 0;
        }
        if (count == cnt) {
            free_bit_idx_start = next_bit - cnt + 1;
            break;
        }
        ++next_bit;
    }
    return free_bit_idx_start;
}

/**
 * bitmap_set - 在位图中设置或清除特定的位。
 * @btmp: 位图的指针。
 * @bit_idx: 要设置或清除的位的索引。
 * @value: 要将位设置为的值（0或1）。
 *
 * 将位图中索引为'bit_idx'的位设置为'value'。
 */
void bitmap_set(struct bitmap *btmp, uint32_t bit_idx, int8_t value) {
    ASSERT((value == 0) || (value == 1));
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_idx_in_byte = bit_idx % 8;

    if (value) {
        btmp->bits[byte_idx] |= BITMAP_MASK << bit_idx_in_byte;
    } else {
        btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_idx_in_byte);
    }
}
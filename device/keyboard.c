
#include "keyboard.h"
#include "global.h"
#include "interrupt.h"
#include "io.h"
#include "io_queue.h"
#include "print.h"
#include "stdint.h"

/* 输出键盘控制器（8024芯片）的端口号 */
#define KBD_BUF_PORT 0x60

/* 使用转义字符表示部分不可见的控制字符 */
#define esc       '\033'
#define backspace '\b'
#define tab       '\t'
#define enter     '\r'
#define delete    '\177'

/* 其他不可见的控制字符（如ctrl、shift、alt、caps_lock）
 * 被定义为0 */
#define char_invisible 0
#define left_ctrl   char_invisible
#define right_ctrl  char_invisible
#define left_shift  char_invisible
#define right_shift char_invisible
#define left_alt    char_invisible
#define right_alt   char_invisible
#define caps_lock   char_invisible

/* 定义控制字符的按键码（makecode）和释放码（breakcode），用于后续判断 */
#define l_shift_makecode   0x2a
#define r_shift_makecode   0x36
#define l_alt_makecode     0x38
#define r_alt_makecode     0xe038
#define r_alt_breakcode    0xe0b8
#define l_ctrl_makecode    0x1d
#define r_ctrl_makecode    0xe01d
#define r_ctrl_breakcode   0xe09d
#define caps_lock_makecode 0x3a

/* 定义以下键用于表示对应键是否按下*/
static bool ctrl_status, shift_status, alt_status, caps_lock_status;
static bool extend_scancode;
struct ioqueue kbd_circular_buf;

/**
 * keymap - 表示键盘键映射表
 * @element: 字符对的数组，每对包含一个键的常规ASCII值和按下Shift时对应的ASCII值，
 * 总共有0x3A对（直到键 caps_lock）
 *
 * 这个二维数组将扫描码映射到它们对应的ASCII值。每对的第一个元素是键的常规ASCII字符，
 * 第二个元素是按下Shift时产生的ASCII字符。
 *
 * 示例映射：
 * - 0: 未使用（没有ASCII等价物）
 * - 1: Escape键（常规和按下Shift时的值）
 * - 2 到 6: 数字键 '1' 到 '6' 和它们相应的Shift值（'!', '@', '#', '$', '%', '^'）
 */
static char keymap[][2] = { {0, 0},
                            {esc, esc},
                            {'1', '!'},
                            {'2', '@'},
                            {'3', '#'},
                            {'4', '$'},
                            {'5', '%'},
                            {'6', '^'},
                            {'7', '&'},
                            {'8', '*'},
                            {'9', '('},
                            {'0', ')'},
                            {'-', '_'},
                            {'=', '+'},
                            {backspace, backspace},
                            {tab, tab},
                            {'q', 'Q'},
                            {'w', 'W'},
                            {'e', 'E'},
                            {'r', 'R'},
                            {'t', 'T'},
                            {'y', 'Y'},
                            {'u', 'U'},
                            {'i', 'I'},
                            {'o', 'O'},
                            {'p', 'P'},
                            {'[', '{'},
                            {']', '}'},
                            {enter, enter},
                            {left_ctrl, left_ctrl},
                            {'a', 'A'},
                            {'s', 'S'},
                            {'d', 'D'},
                            {'f', 'F'},
                            {'g', 'G'},
                            {'h', 'H'},
                            {'j', 'J'},
                            {'k', 'K'},
                            {'l', 'L'},
                            {';', ':'},
                            {',', '"'},
                            {'`', '~'},
                            {left_shift, left_shift},
                            {'\\', '|'},
                            {'z', 'Z'},
                            {'x', 'X'},
                            {'c', 'C'},
                            {'v', 'V'},
                            {'b', 'B'},
                            {'n', 'N'},
                            {'m', 'M'},
                            {',', '<'},
                            {'.', '>'},
                            {'/', '?'},
                            {right_shift, right_shift},
                            {'*', '*'},
                            {left_alt, left_alt},
                            {' ', ' '},
                            {caps_lock, caps_lock}};

static void intr_keyboard_handler(void) {
    /*记录一下这次中断前这几个键是否被按下*/
    bool ctrl_down_last = ctrl_status;
    bool shift_down_last = shift_status;
    bool caps_lock_last = caps_lock_status;

    bool break_code;
    uint16_t scancode = inb(KBD_BUF_PORT);

    /* 若扫描码是以e0开头，说明还未接收完成，马上结束进行下一次接收 */
    if (scancode == 0xe0) {
        extend_scancode = true;
        return;
    }
    /* 若上一次扫描码是以e0开头，则合并扫描码*/
    if (extend_scancode) {
        scancode = (0xe0 << 2) | scancode;
        extend_scancode = false;
    }

    /* 通过检查第7位（从0开始计数）是否为1来确定是否为断码 */
    break_code = ((scancode & (0x01 << 7)) != 0);

    if (break_code) {
        /* 通过断码获取按键码，从而可以知道哪个键处于弹起状态 */
        uint16_t makecode = scancode & (0xffff ^ (0x01 << 7));

        if (makecode == l_ctrl_makecode || makecode == r_ctrl_makecode)
            ctrl_status = false;
        if (makecode == l_shift_makecode || makecode == r_shift_makecode)
            shift_status = false;
        if (makecode == l_alt_makecode || makecode == r_alt_makecode)
            alt_status = false;
        return;
    } 
    /* 只处理主区按键 */
    else if (scancode < 0x3b || scancode == right_ctrl || scancode == right_alt) {//
        /* 这里的布尔变量 shift 决定我们打印第一个元素还是第二个元素，比如 a 或 A，= 或 + */
        bool shift = false;

        /* 首先处理两个字符的键（0~9, -, =, ;, ', [, ...） */
        if ((scancode < 0x0e) || (scancode == 0x29) || (scancode == 0x1a) ||
            (scancode == 0x1b) || (scancode == 0x2b) || (scancode == 0x27) ||
            (scancode == 0x28) || (scancode == 0x33) || (scancode == 0x34) ||
            (scancode == 0x35)) {
            if (shift_down_last)
                shift = true;
        } else {
            /* 处理字母键（a~z） */
            if (shift_down_last && caps_lock_last) {
                shift = false;
            } else if (shift_down_last || caps_lock_last) {
                shift = true;
            } else {
                shift = false;
            }
        }

        /* 对于某些键（如右Alt键），擦除高字节 'e0' */
        uint8_t index = scancode & 0x00ff;

        /* 通过键映射的索引将扫描码转换为ASCII */
        char cur_char = keymap[index][shift];
        /* 如果是可见字符或诸如退格的特殊字符，则打印 */

        /* 处理特殊情况：Ctrl + l 和 Ctrl + u */
        if ((ctrl_down_last && cur_char == 'l') ||
            (ctrl_down_last && cur_char == 'u')) {
            cur_char -= 'a';
        }

        if (cur_char) {
            if (!ioq_is_full(&kbd_circular_buf)) {
                //put_char(cur_char); 
                ioq_putchar(&kbd_circular_buf, cur_char);
            }
            return;
        }

        /* 处理控制字符 --- ctrl, shift, alt, caps_lock */
        if (scancode == l_ctrl_makecode || scancode == r_ctrl_makecode) {
            ctrl_status = true;
        } else if (scancode == l_shift_makecode || scancode == r_shift_makecode) {
            shift_status = true;
        } else if (scancode == l_alt_makecode || scancode == r_alt_makecode) {
            alt_status = true;
        } else if (scancode == caps_lock_makecode) {
            caps_lock_status = !caps_lock_status;
        }
    } else {
        put_str("unknown key\n");
    }

}

void keyboard_init() {
    put_str("  keyboard init start\n");
    ioqueue_init(&kbd_circular_buf);
    register_handler(0x21, intr_keyboard_handler);
    put_str("  keyboard init done\n");
}

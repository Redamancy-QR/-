#include "init.h"
#include "print.h"
#include "timer.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"

void init_all() {
    put_str("init_all_start\n");
    idt_init();
    mem_init();
    thread_init();
    timer_init();
    console_init();
    keyboard_init();
    tss_init();
    //put_str("init_all_end\n");
}
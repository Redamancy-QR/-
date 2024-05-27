#include "init.h"
#include "print.h"
#include "timer.h"
#include "interrupt.h"

void init_all() {
    put_str("init_all_start\n");
    idt_init();
    timer_init();
    put_str("init_all_end\n");
}
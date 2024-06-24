#include "debug.h"
#include "global.h"
#include "init.h"
#include "interrupt.h"
#include "memory.h"
#include "print.h"
#include "string.h"
#include "thread.h"
#include "console.h"

#include "ioqueue.h"
#include "keyboard.h"

void kthread_a(void *arg);
void kthread_b(void *arg);

int main() {
    put_str("I am kernel\n");
    init_all();
    thread_start("kthread_a",31,kthread_a," A_");
    thread_start("kthread_b",8,kthread_b," B_");
    intr_enable();
    while (1);
    // while (1){
    //     console_put_str("Main ");
    // }
    return 0;
}

void kthread_a(void *arg) {
    while (1){
        enum intr_status old_status = intr_disable();
        if(!ioq_is_empty(&kbd_circular_buf)){
            console_put_str(arg);
            char byte = ioq_getchar(&kbd_circular_buf);
            console_put_char(byte);
        }
        intr_set_status(old_status);
    }
}

void kthread_b(void *arg) {
    while (1){
        enum intr_status old_status = intr_disable();
        if(!ioq_is_empty(&kbd_circular_buf)){
            console_put_str(arg);
            char byte = ioq_getchar(&kbd_circular_buf);
            console_put_char(byte);
        }
        intr_set_status(old_status);
    }
}
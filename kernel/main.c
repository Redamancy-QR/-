#include "debug.h"
#include "global.h"
#include "init.h"
#include "interrupt.h"
#include "memory.h"
#include "print.h"
#include "string.h"
#include "thread.h"
#include "console.h"
#include "process.h"

#include "io_queue.h"
#include "keyboard.h"

void kthread_a(void *arg);
void kthread_b(void *arg);
void u_prog_a(void);
void u_prog_b(void);
int test_var_a = 0,test_var_b=0;

int main() {
    put_str("I am kernel\n");
    init_all();
    thread_start("kthread_a",31,kthread_a," A_");
    thread_start("kthread_b",8,kthread_b," B_");
    process_execute(u_prog_a,"user_prog_a");
    process_execute(u_prog_b,"user_prog_b");
    intr_enable();
    while (1);
    // while (1){
    //     console_put_str("Main ");
    // }
    return 0;
}

void kthread_a(void *arg) {
    char* para = arg;
    while (1){
        console_put_str(" v_a:0x");
        console_put_int(test_var_a);
    }
}

void kthread_b(void *arg) {
    char* para = arg;
    while (1){
        console_put_str(" v_b:0x");
        console_put_int(test_var_b);
    }
}

void u_prog_a(void){
    while(1){
        test_var_a++;
    }
}

void u_prog_b(void){
    while(1){
        test_var_b++;
    }
}
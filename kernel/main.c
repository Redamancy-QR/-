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

#include "syscall_init.h"
#include "syscall.h"

void kthread_a(void *arg);
void kthread_b(void *arg);
void u_prog_a(void);
void u_prog_b(void);
int prog_a_pid = 0,prog_b_pid=0;

int main() {
    put_str("I am kernel\n");
    init_all();
    
    process_execute(u_prog_a,"user_prog_a");
    process_execute(u_prog_b,"user_prog_b");
    intr_enable();
    console_put_str("Main_pid:0x ");
    console_put_int(sys_getpid());
    console_put_char('\n');
    thread_start("kthread_a",31,kthread_a," A_");
    thread_start("kthread_b",8,kthread_b," B_");
    while (1);
    // while (1){
    //     console_put_str("Main ");
    // }
    return 0;
}

void kthread_a(void *arg) {
    char* para = arg;
    console_put_str("thread_a_pid:0x ");
    console_put_int(sys_getpid());
    console_put_char('\n');
    console_put_str("prog_a_pid:0x ");
    console_put_int(prog_a_pid);
    console_put_char('\n');
    while (1);
}

void kthread_b(void *arg) {
    char* para = arg;
    console_put_str("thread_b_pid:0x ");
    console_put_int(sys_getpid());
    console_put_char('\n');
    console_put_str("prog_b_pid:0x ");
    console_put_int(prog_b_pid);
    console_put_char('\n');
    while (1);
}

void u_prog_a(void){
    prog_a_pid = getpid();
    while(1);
}

void u_prog_b(void){
    prog_b_pid = getpid();
    while(1);
}
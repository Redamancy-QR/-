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
#include "stdio.h"

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
    console_put_str("I am Main_pid:0x ");
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
    console_put_str("I am thread_a_pid:0x ");
    console_put_int(sys_getpid());
    console_put_char('\n');
    while (1);
}

void kthread_b(void *arg) {
    char* para = arg;
    console_put_str("I am thread_b_pid:0x ");
    console_put_int(sys_getpid());
    console_put_char('\n');
    while (1);
}

void u_prog_a(void){
    char* name = "prog_a";
    printf("I am %s, my pid:%d%c",name,getpid(),'\n');
    while(1);
}

void u_prog_b(void){
    char* name = "prog_b";
    printf("I am %s, my pid:%d%c",name,getpid(),'\n');
    while(1);
}
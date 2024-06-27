; ============================================================
; 定义中断处理程序
; ============================================================
[bits 32]
;------------------------------------
; 在没有错误代码的情况下，推送4个字节的0
; 目的是统一堆栈操作
; 当CPU执行iret指令时，ESP必须指向EIP
;------------------------------------
%define ERROR_CODE nop
%define ZERO push 0

extern idt_table

section .data
;------------------------
; 中断处理程序的入口地址数组
;------------------------
global intr_entry_table
intr_entry_table:
;---------------------------------------------------------------
; 宏函数VECTOR - 在函数体中调用中断处理程序（该处理程序在C文件中定义）
;---------------------------------------------------------------
%macro VECTOR 2
section .text
intr_%1_entry:
    ; 这里可能会使用 push 0 或 nop 指令。这条指令必须首先执行。
    %2

    push ds
    push es
    push fs
    push gs
    pushad

    ; 发送 EOI（End of Interrupt）信号给中断代理
    mov al,0x20   ;中断结束命令EOI
    out 0xa0,al   ;向从片发送
    out 0x20,al   ;向主片发送

    ; 用于调试的时候，%1 代表中断向量号
    push %1

    ; 调用真正的中断处理程序
    call [idt_table + %1*4]
    jmp intr_exit

; 存储中断处理程序的入口地址
section .data
    dd intr_%1_entry

%endmacro

section .text
global intr_exit
intr_exit:
    add esp,4 ; 跳过中断向量号。
    popad
    pop gs
    pop fs
    pop es
    pop ds

    add esp,4  ;跳过ERROR_CODE
    iretd

;-------------------------------------------------------------------------
; 调用宏函数,传递两个参数
; 如果中断事件没有将错误代码推送到堆栈上,则传递参数零;表示推送4个字节的0
; 操作系统内核的最低可用中断向量编号为32(从0开始编号),因此在这里定义了共33个中断
;-------------------------------------------------------------------------
VECTOR 0X00, ZERO
VECTOR 0X01, ZERO
VECTOR 0X02, ZERO
VECTOR 0X03, ZERO
VECTOR 0X04, ZERO
VECTOR 0X05, ZERO
VECTOR 0X06, ZERO
VECTOR 0X07, ZERO
VECTOR 0X08, ERROR_CODE
VECTOR 0X09, ZERO
VECTOR 0X0a, ERROR_CODE
VECTOR 0X0b, ERROR_CODE
VECTOR 0X0c, ERROR_CODE
VECTOR 0X0d, ERROR_CODE
VECTOR 0X0e, ERROR_CODE
VECTOR 0X0f, ZERO
VECTOR 0X10, ZERO
VECTOR 0X11, ERROR_CODE
VECTOR 0X12, ZERO
VECTOR 0X13, ZERO
VECTOR 0X14, ZERO
VECTOR 0X15, ZERO
VECTOR 0X16, ZERO
VECTOR 0X17, ZERO
VECTOR 0X18, ZERO
VECTOR 0X19, ZERO
VECTOR 0X1a, ZERO
VECTOR 0X1b, ZERO
VECTOR 0X1c, ZERO
VECTOR 0X1d, ZERO
VECTOR 0X1e, ERROR_CODE
VECTOR 0X1f, ZERO
VECTOR 0X20, ZERO ; IR0 --- 时钟中断
VECTOR 0X21, ZERO ; IR1 --- 键盘中断
VECTOR 0X22, ZERO
VECTOR 0X23, ZERO
VECTOR 0X24, ZERO
VECTOR 0X25, ZERO
VECTOR 0X26, ZERO
VECTOR 0X27, ZERO
VECTOR 0X28, ZERO
VECTOR 0X29, ZERO
VECTOR 0X2a, ZERO
VECTOR 0X2b, ZERO
VECTOR 0X2c, ZERO
VECTOR 0X2d, ZERO
VECTOR 0X2e, ZERO
VECTOR 0X2f, ZERO

;------------------------ 0x80 中断------------------------
[bits 32]
extern syscall_table
section .text
global syscall_handler
syscall_handler:
    push 0
    push ds
    push es
    push fs
    push gs
    pushad

    push 0x80

    push edx
    push ecx
    push ebx

    call [syscall_table+4*eax]
    ; 跳过堆栈中的 3 个参数
    add esp,12

    ; esp+(8)*4 是 eax 恢复的地方
    ; 8 是 `push 0x80` 的一个字节加上 `pushad` 的七个字节
    mov [esp+8*4], eax
    jmp intr_exit
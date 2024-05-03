;编译之前将"boot.inc"包含进来
%include "boot.inc"
; 加载器加载到内存中的位置
SECTION LOADER vstart=LOADER_BASE_ADDR
;栈是向下增长，故加载器的起始地址也是栈的起始地址
LOADER_STACK_TOP equ LOADER_BASE_ADDR

jmp loader_start
; ============================================================
; 构建GDT及其内部描述符
; ============================================================

;第0个不可段描述符(因为不可用,所以全部初始化为0)
GDT_BASE:
    dd 0x00000000
    dd 0x00000000

;代码段描述符
CODE_DESC:
    dd 0x0000FFFF
    dd DESC_CODE_HIGH4

;数据段和栈段描述符
DATA_STACK_DESC:
    dd 0x0000FFFF
    dd DESC_DATA_HIGH4

;显存段描述符
VIDEO_DESC:
    dd 0x80000007
    dd DESC_VIDEO_HIGH4
 
;通过地址差获得GDT的大小 
GDT_SIZE equ $-GDT_BASE
;减1得到段界限
GDT_LIMIT equ GDT_SIZE - 1

;预留一定空间方便扩充
times 60 dq 0

;构建代码段、数据段、显存段的选择子
SELECTOR_CODE   equ (0x0001 << 3) + TI_GDT + RPL0
SELECTOR_DATA   equ (0x0002 << 3) + TI_GDT + RPL0
SELECTOR_VIDEO  equ (0x0003 << 3) + TI_GDT + RPL0

;构建DGT的指针，在lgdt加载GDT到gdtr寄存器时使用
gdt_ptr:
    dw GDT_LIMIT
    dd GDT_BASE

loadermsg db '2 loader in real.'

loader_start:

    mov sp, LOADER_BASE_ADDR
    mov bp, loadermsg
    mov cx, 17
    mov ax, 0x1301
    mov bx, 0x001f
    mov dx, 0x1800
    int 0x10

; ============================================================
; 准备进入保护模式
; ============================================================

    ;打开A20
    in al, 0x92
    or al, 0000_0010B
    out 0x92, al

    ;加载DGT
    lgdt [gdt_ptr]

    ;cr0第0位置1
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

    ;刷新流水线
    jmp dword SELECTOR_CODE:p_mode_start

[bits 32]
p_mode_start:
    ;使用选择子初始化各段寄存器
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    mov byte [gs:160], 'P'

    jmp $
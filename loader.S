%include "boot.inc"
; 加载器加载到内存中的位置
SECTION LOADER vstart=LOADER_BASE_ADDR
; 栈是向下增长，故加载器的起始地址也是栈的起始地址
LOADER_STACK_TOP equ LOADER_BASE_ADDR

jmp loader_start

; ============================================================
; 构建GDT及其内部描述符
; ============================================================

;第0个不可段描述符(因为不可用,所以全部初始化为0)
GDT_BASE:
    dd 0x00000000
    dd 0x00000000

;代码段描述符 -> 段基址为0x0，段大小为4GB
CODE_DESC:
    dd 0x0000FFFF
    dd DESC_CODE_HIGH4

;数据段和栈段描述符 -> 段基址为0x0，段大小为4GB
DATA_STACK_DESC: 
    dd 0x0000FFFF
    dd DESC_DATA_HIGH4

;显存段描述符 -> 段基址为0xb8000，段大小为32KB
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
SELECTOR_CODE   equ (0x0001 << 3) + TI_GDT + RPL_0
SELECTOR_DATA   equ (0x0002 << 3) + TI_GDT + RPL_0
SELECTOR_VIDEO  equ (0x0003 << 3) + TI_GDT + RPL_0

;构建DGT的指针，在lgdt加载GDT到gdtr寄存器时使用
gdt_ptr:
    dw GDT_LIMIT
    dd GDT_BASE

loadermsg db '2 loader in real.'

loader_start:

    ;mov sp, LOADER_BASE_ADDR
    ;mov bp, loadermsg
    ;mov cx, 17
    ;mov ax, 0x1301
    ;mov bx, 0x001f
    ;mov dx, 0x1800
    ;int 0x10

    mov byte [gs:0xa0], '2'
    mov byte [gs:0xa1], 0xA4
    mov byte [gs:0xa2], ' '
    mov byte [gs:0xa3], 0xA4
    mov byte [gs:0xa4], 'L'
    mov byte [gs:0xa5], 0xA4
    mov byte [gs:0xa6], 'O'
    mov byte [gs:0xa7], 0xA4
    mov byte [gs:0xa8], 'A'
    mov byte [gs:0xa9], 0xA4
    mov byte [gs:0xaa], 'D'
    mov byte [gs:0xab], 0xA4
    mov byte [gs:0xac], 'E'
    mov byte [gs:0xad], 0xA4
    mov byte [gs:0xae], 'R'
    mov byte [gs:0xaf], 0xA4

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

    mov byte [gs:0x140], '3'
    mov byte [gs:0x141], 0xA4
    mov byte [gs:0x142], ' '
    mov byte [gs:0x143], 0xA4
    mov byte [gs:0x144], 'L'
    mov byte [gs:0x145], 0xA4
    mov byte [gs:0x146], 'O'
    mov byte [gs:0x147], 0xA4
    mov byte [gs:0x148], 'A'
    mov byte [gs:0x149], 0xA4
    mov byte [gs:0x14a], 'D'
    mov byte [gs:0x14b], 0xA4
    mov byte [gs:0x14c], 'E'
    mov byte [gs:0x14d], 0xA4
    mov byte [gs:0x14e], 'R'
    mov byte [gs:0x14f], 0xA4
    mov byte [gs:0x150], ' '
    mov byte [gs:0x151], 0xA4
    mov byte [gs:0x152], 'I'
    mov byte [gs:0x153], 0xA4
    mov byte [gs:0x154], 'N'
    mov byte [gs:0x155], 0xA4
    mov byte [gs:0x156], ' '
    mov byte [gs:0x157], 0xA4
    mov byte [gs:0x158], 'P'
    mov byte [gs:0x159], 0xA4
    mov byte [gs:0x15a], 'R'
    mov byte [gs:0x15b], 0xA4
    mov byte [gs:0x15c], 'O'
    mov byte [gs:0x15d], 0xA4
    mov byte [gs:0x15e], 'T'
    mov byte [gs:0x15f], 0xA4
    mov byte [gs:0x160], 'E'
    mov byte [gs:0x161], 0xA4
    mov byte [gs:0x162], 'C'
    mov byte [gs:0x163], 0xA4
    mov byte [gs:0x164], 'T'
    mov byte [gs:0x165], 0xA4

    jmp $
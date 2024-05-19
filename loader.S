%include "boot.inc"
; 加载器加载到内存中的位置
SECTION LOADER vstart=LOADER_BASE_ADDR
; 栈是向下增长，故加载器的起始地址也是栈的起始地址
LOADER_STACK_TOP equ LOADER_BASE_ADDR

; ============================================================
; 构建GDT及其内部描述符
; ============================================================

; 第0个不可段描述符(因为不可用,所以全部初始化为0)
GDT_BASE:
    dd 0x00000000
    dd 0x00000000

; 代码段描述符 -> 段基址为0x0，段大小为4GB
CODE_DESC:
    dd 0x0000FFFF
    dd DESC_CODE_HIGH4

; 数据段和栈段描述符 -> 段基址为0x0，段大小为4GB
DATA_STACK_DESC: 
    dd 0x0000FFFF
    dd DESC_DATA_HIGH4

; 显存段描述符 -> 段基址为0xb8000，段大小为32KB
VIDEO_DESC:
    dd 0x80000007
    dd DESC_VIDEO_HIGH4

; 通过地址差获得GDT的大小 
GDT_SIZE equ $-GDT_BASE
; 减1得到段界限
GDT_LIMIT equ GDT_SIZE - 1

; 预留一定空间方便扩充
times 60 dq 0

; 构建代码段、数据段、显存段的选择子
SELECTOR_CODE   equ (0x0001 << 3) + TI_GDT + RPL_0
SELECTOR_DATA   equ (0x0002 << 3) + TI_GDT + RPL_0
SELECTOR_VIDEO  equ (0x0003 << 3) + TI_GDT + RPL_0

; 用于保存内存容量,以字节为单位,该变量在内存中地址为0xb00
total_mem_bytes dd 0

; 构建DGT的指针，在lgdt加载GDT到gdtr寄存器时使用
gdt_ptr:
    dw GDT_LIMIT
    dd GDT_BASE

; 定义一个缓冲区,存储BIOS返回的ARDS结构数据,244字节是为了使loader_start处起始地址为0x300
ards_buf times 244 db 0

; 用于记录ARDS结构体数量
ards_nr dw 0

loader_start:

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
; 获取物理内存容量
; ============================================================

; ------------------------
; 利用BIOS中断0x15子功能0xe820获取内存大小
; ------------------------
    xor ebx, ebx          ;使用说明中要求此寄存器必须置0
    mov edx, 0x534d4150   ;签名标记,固定不变,用于校验
    mov di, ards_buf      ;将ES:DI指向准备接收返回数据的地方,ES已在MBR中赋值

.E820_mem_get_loop:
    mov eax, 0x0000e820   ;指定子功能号,因为每次执行int 0x15后eax寄存器的值会被改变,想要循环查询就要重新制定子功能号
    mov ecx, 20           ;指定ARDS结构大小为20字节
    int 0x15              ;执行0x15中断

    ;若cf位为1则有错误发生,尝试0xe801子功能获取内存
    jc .E820_failed_so_try_E801

    ;无错误发生则准备下一次查询
    add di, cx               ;指向缓冲区中下一个ARDS结构位置
    inc word [ards_nr]       ;ARDS数量+1
    cmp ebx, 0
    jnz .E820_mem_get_loop   ;若ebx为0且cf不为1,说明ARDS全部返回,结束循环

; ---------------------------------
; 在所有ARDS中找出内存容量最大的ARDS
; ---------------------------------
    mov cx, [ards_nr]     ;指定循环次数为ARDS数量
    mov ebx, ards_buf
    xor edx, edx          ;使用edx存储最大容量,故先初始化为0

.find_max_mem_area:
    mov eax, [ebx]        ;base_add_low
    add eax, [ebx+8]      ;length_low
    add ebx, 20           ;指向缓冲区中下一个ARDS结构位置
    cmp edx, eax
    jge .next_ards        ;如果edx中的值大于等于eax中的值则跳转
    mov edx, eax          ;更新最大值

.next_ards:
    loop .find_max_mem_area  ;cx寄存器为0后结束查找最大值过程
    jmp .mem_get_ok

; ----------------------------------------
; 利用BIOS中断0x15子功能0xe801获取内存大小
; ----------------------------------------
.E820_failed_so_try_E801:
    mov ax, 0xe801    ;指定子功能号
    int 0x15

    ;若cf位为1则有错误发生,尝试0xe88子功能获取内存
    jc .E801_failed_so_try_88

    ;先计算低15MB的内存
    mov cx,0x400          ;ax和cx中以KB为单,先转化为字节单位
    mul cx
    shl edx, 16
    and eax, 0x0000FFFF
    or edx, eax
    add edx, 0x100000
    mov esi, edx          ;备份低15MB内容

    ;计算内存在16MB~4GB部分
    xor eax, eax
    mov ax, bx            ;bx和dx中以64KB为单,先转化为字节单位
    mov ecx, 0x10000
    mul ecx               ;32位乘法,被乘数eax,结果高32位存入edx,低32位存入eax

    add esi, eax          ;高32位一定为0,因为最多4GB,只加eax即可
    mov edx, esi          ;与前述方法保持一致,edx存储总内存大小
    jmp .mem_get_ok

; ---------------------------------------
; 利用BIOS中断0x15子功能0xe88获取内存大小
; ---------------------------------------
.E801_failed_so_try_88:
    mov ah, 0x88     ;指定子功能号
    int 0x15

    ;若cf位为1则有错误发生,暂停CPU
    jc .error_hlt

    ;int 0x15后,ax被存入内存大小,单位为KB
    and eax, 0x0000FFFF

    mov cx, 0x400      ;转化单位为字节
    mul cx             ;16位乘法,被乘数ax,结果高16位存入dx,低32位存入ax
    shl edx, 16
    or edx, eax
    add edx, 0x100000  ;0x88子功能只会返回1MB以上的部分,故还要加1

; 将 CPU 暂停,直到下一个外部中断被触发（发生）
.error_hlt:
    hlt

.mem_get_ok:
    mov [total_mem_bytes], edx ;将内存大小存入指定处

    mov byte [gs:0x1e0], '4'
    mov byte [gs:0x1e1], 0xA4
    mov byte [gs:0x1e2], ' '
    mov byte [gs:0x1e3], 0xA4
    mov byte [gs:0x1e4], 'M'
    mov byte [gs:0x1e5], 0xA4
    mov byte [gs:0x1e6], 'E'
    mov byte [gs:0x1e7], 0xA4
    mov byte [gs:0x1e8], 'M'
    mov byte [gs:0x1e9], 0xA4
    mov byte [gs:0x1ea], 'E'
    mov byte [gs:0x1eb], 0xA4
    mov byte [gs:0x1ec], 'R'
    mov byte [gs:0x1ed], 0xA4
    mov byte [gs:0x1ee], 'Y'
    mov byte [gs:0x1ef], 0xA4
    mov byte [gs:0x1f0], ' '
    mov byte [gs:0x1f1], 0xA4
    mov byte [gs:0x1f2], 'G'
    mov byte [gs:0x1f3], 0xA4
    mov byte [gs:0x1f4], 'E'
    mov byte [gs:0x1f5], 0xA4
    mov byte [gs:0x1f6], 'T'
    mov byte [gs:0x1f7], 0xA4
    mov byte [gs:0x1f8], ' '
    mov byte [gs:0x1f9], 0xA4
    mov byte [gs:0x1fa], 'O'
    mov byte [gs:0x1fb], 0xA4
    mov byte [gs:0x1fc], 'K'
    mov byte [gs:0x1fd], 0xA4

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
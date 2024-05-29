[bits 32]
section .data
put_int_buffer dq 0

section .text

;定义视频段的选择子
TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

; ============================================================
; put_char: 从光标处打印堆栈中的字符
; ============================================================
global put_char
put_char:
    ; ------------------------
    ; 备份32位寄存器(共八个)
    ; ------------------------
    pushad

    ; ------------------------
    ; 为gs安装正确的选择子
    ; ------------------------
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    ; -------------------------------
    ; 从显卡寄存器中获取光标位置（16位）
    ; -------------------------------
    ; 高8位
    mov dx, 0x03d4  ;指定索引寄存器
    mov al, 0x0e    ;指定子功能:获取光标高8位
    out dx, al 
    mov dx, 0x03d5  ;指定读写数据寄存器(端口)
    in al, dx       
    mov ah, al

    ; 低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    in al, dx

    ; 寄存器 bx 存储着光标的（线性）坐标
    mov bx, ax

    ; -------------------------------------------------------
    ; 检索要打印的字符,32字节的寄存器空间 + 4字节的调用者返回地址
    ; -------------------------------------------------------
    mov ecx, [esp+36]

    ; -------------------------------------------------------
    ; 处理要打印的字符
    ; 1.对控制字符进行特殊处理，并打印普通可见字符
    ; 2.如果可见字符超出屏幕（cmp bx, 2000），则添加回车处理操作
    ; -------------------------------------------------------
    cmp cl, 0xd
    jz .is_carriage_return ; 回车符
    cmp cl, 0xa          
    jz .is_line_feed       ; 换行符
    cmp cl, 0x8
    jz .is_backspace       ; 退格键
    jmp .put_other         ; 其它字符

; ------------------------
; 处理退格键
; ------------------------
.is_backspace:
    ; 光标坐标减1,相当于光标向左移动
    dec bx
    ; 光标是字符的坐标，而一个字符占据 2 字节，所以通过光标向视频内存写入数据时，光标需要乘以 2
    shl bx, 1

    mov byte [gs:bx], 0x20  ; 指定字符:空格->覆盖原有字符实现擦除
    inc bx                  ; 加一指向设置属性的地址
    mov byte [gs:bx], 0x07  ; 指定属性:黑屏白字

    ; 恢复 bx 值,使其重新为光标位置,而不是光标的内存地址
    shr bx, 1
    jmp .set_cursor  ;处理光标的位置

; ------------------------
; 处理可见字符
; ------------------------
.put_other:
    shl bx, 1
    mov [gs:bx], cl
    inc bx
    mov byte [gs:bx], 0x07
    shr bx, 1
    inc bx          ; 将光标指向下一个待打印的位置
    cmp bx, 2000
    jl .set_cursor  ; 若光标值小于2000表示还可以显示,否则执行换行处理

; -----------------------------------
; 处理回车符和换行符(统一看做回车换行符)
; -----------------------------------
; "\n" --- 将光标移动到下一行的开头
.is_line_feed:
; "\r" --- 将光标移动到同一行的开头
.is_carriage_return:
    ; 16位除法,求模80的结果
    xor dx, dx
    mov ax, bx
    mov si, 80
    div si
    ; 减去余数,即回到行首
    sub bx, dx
    ; 加80,即到了下一行
    add bx, 80
    ; 如果光标超出了屏幕范围（即指令jl的结果为假），则滚动屏幕
    cmp bx, 2000
    jl .set_cursor

; ------------------------
; 滚屏
; ------------------------
.roll_screen:
    ; 将第 1 行到第 24 行的内容覆盖到第 0 行到第 23 行
    cld                 ; 将eflags寄存器中方向标志位DF清0
    mov ecx, 960        ; ((2000-80)*2)/4=960
    mov esi, 0xc00b80a0 ; 第 0 行开始
    mov edi, 0xc00b8000 ; 第 1 行开始
    rep movsd           ; 每次复制 4 字节

    ; 清除当前屏幕的最后一行,填充为白空格(0x0720)
    mov ebx, 3840 ; 1920*2 = 3840
    mov ecx, 80
.cls:
    mov word [gs:ebx], 0x0720
    add ebx, 2
    loop .cls
    ; 更新光标位置信息->指向最后一行的开头
    mov bx, 1920

; ------------------------
; 更新图形卡中的光标位置信息
; ------------------------
.set_cursor:
    ; 设置高 8 位
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al

    mov dx, 0x03d5
    mov al, bh
    out dx, al

    ; 设置低 8 位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al

    mov dx, 0x03d5
    mov al, bl
    out dx, al

.put_char_end:
    popad  ; 恢复之前压入栈的 8 个寄存器
    ret    ; 执行完函数流程,返回

; ============================================================
; put_str: 通过 put_char 来打印以 0 字符结尾的字符串
; ============================================================
global put_str
put_str:
    ; -----------------------------------
    ; 备份寄存器,准备参数（字符串的起始地址）
    ; -----------------------------------
    push ebx
    push ecx
    xor ecx, ecx       ; 清空
    mov ebx, [esp+12]  ; 备份寄存器的8个字节 + 调用者返回地址的4个字节

; 通过调用 put_char 实现该函数
.goon:
    mov cl, [ebx]
    cmp cl, 0
    jz .str_over  ; 判断是不是到了结尾

    push ecx
    call put_char
    add esp, 4
    inc ebx
    loop .goon

.str_over:
    pop ecx
    pop ebx
    ret


; ====================================================================
; put_int: 打印栈中的数字(put_int_buffer 用作缓冲区，用于存储转换后的结果)
; ====================================================================
global put_int
put_int:
    pushad
    mov ebp, esp      ; 获取esp的值，通过esp来访问栈
    mov eax, [ebp+36] ; 32字节的寄存器 + 4字节的调用者返回地址
    mov edx, eax

    mov edi, 7        ; 指定 put_int_buffer 中初始的偏移量
    mov ecx, 8        ; 待计算的位数(32/4=8)
    mov ebx, put_int_buffer  ; EBX代表缓冲区的基地址

; ------------------------------------------
; 将字符（32位数中的每4位）转换为相应的ASCII值
; ------------------------------------------
.16based_4bits:
    and edx, 0x0000000F
    cmp edx, 9
    jg .is_A2F
    add edx, '0'
    jmp .store
.is_A2F:
    sub edx, 10
    add edx, 'A'
.store:
    mov [ebx+edi], dl
    dec edi
    shr eax, 4
    mov edx, eax
    loop .16based_4bits

; ------------------------
; 去掉多余的 0
; ------------------------
.ready_to_print:
    inc edi ; 使 edi 重新指向最高位

.skip_prefix_0:
    ; 如果所有位都是 0,做特殊处理
    cmp edi, 8
    je .full0

.detect_prefix_0:
    mov cl, [put_int_buffer+edi]
    inc edi
    cmp cl, '0'
    je  .skip_prefix_0
    dec edi
    jmp .put_each_num

.full0:
    mov cl, '0'

.put_each_num:
    push ecx
    call put_char
    add esp, 4
    inc edi
    mov cl, [put_int_buffer+edi]
    cmp edi, 8
    jl .put_each_num
    popad
    ret

global set_cursor
set_cursor:
    pushad
    ; 32 bytes of 8 registers and 4 bytes of return address
    mov bx, [esp+36]
    ; set high 8 bits
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al

    mov dx, 0x03d5
    mov al, bh
    out dx, al

    ; set low 8 bits
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al

    mov dx, 0x03d5
    mov al, bl
    out dx, al

    popad
    ret

global sys_clear
sys_clear:
    pushad
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    mov ebx, 0
    mov ecx, 80*25

.cls:
    mov word [gs:ebx],0x0720
    add ebx, 2
    loop .cls

    mov ebx, 0

.set_cursor:
    ; set high 8 bits
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al

    mov dx, 0x03d5
    mov al, bh
    out dx, al

    ; set low 8 bits
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al

    mov dx, 0x03d5
    mov al, bl
    out dx, al

.put_char_end:
    popad
    ret
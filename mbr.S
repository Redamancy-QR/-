;主引导程序
;编译之前将"boot.inc"包含进来
%include "boot.inc"
SECTION MBR vstart=0x7c00
    ;初始化寄存器
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov sp, 0x7c00
    ;图形卡文本模式的起始地址
	mov ax, 0xb800
	mov gs, ax

    ;清屏
    mov ax, 0x0600   ;AL:上卷的行数(为0表示全部),AH:功能号,0x06表示清屏功能
    mov bx, 0x0700   ;上卷行属性
    mov cx, 0x0      ;左上角(0,0)
    mov dx, 0x184f   ;右下角(80,25),在VGA文本模式中，一行只能容纳80个字符，一共25行
    int 0x10         ;调用BIOS的视频服务中断

    ;在屏幕上显示字符串"1 MBR"
    ;0x1010 0100 -> 前景色(字的颜色)为红色，背景色为绿色，闪烁(字闪烁)
    mov byte [gs:0x00], '1'
    mov byte [gs:0x01], 0xA4
    mov byte [gs:0x02], ' '
    mov byte [gs:0x03], 0xA4
    mov byte [gs:0x04], 'M'
    mov byte [gs:0x05], 0xA4
    mov byte [gs:0x06], 'B'
    mov byte [gs:0x07], 0xA4
    mov byte [gs:0x08], 'R'
    mov byte [gs:0x09], 0xA4

    ;接下来用eax,bx,cx三个寄存器传递参数，故先将值存进寄存器中
    mov eax, LOADER_START_SECTOR   ;待读入扇区的起始地址(0x2)
    mov bx, LOADER_BASE_ADDR       ;数据从硬盘读入后存放的地址(0x900)
    mov cx, 4                      ;待读入的扇区数目

    call rd_disk_m_16              ;执行该函数在"16位模式下读硬盘"
    jmp LOADER_BASE_ADDR + 0x300   ;跳转到LOADER程序

; ============================================================
; 功能:读取硬盘n(由cx寄存器决定读几个扇区)个扇区
; ============================================================
rd_disk_m_16:
    ;备份eax,cx
    mov esi, eax
    mov di, cx

;第一步:设置要读取的扇区数
    mov dx, 0x1f2 
    mov al,cl
    out dx,al

	mov eax, esi

;第二步,将LBA地址存入0x1f3 ~ 0x1f6
    ;LBA地址7~0位写入端口0x1f3
    mov dx, 0x1f3
    out dx, al

    ;LBA地址15~8位写入端口0x1f4
    mov cl, 8
    shr eax, cl
    mov dx, 0x1f4
    out dx, al

    ;LBA地址23~16位写入端口0x1f5
    shr eax, cl
    mov dx, 0x1f5
    out dx, al

    ;设置device
    shr eax, cl
    and al, 0x0f   ;设置LB地址24~27位
    or al, 0xe0    ;0xe0 -> 11100000 ,设置7~4位为1110，表示为LBA模式
    mov dx, 0x1f6
    out dx, al     ;将配置信息写入端口0x1f6

;第三步:向0x1f7端口写入读命令(0x20)
    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

;第四步:检测硬盘状态
.not_ready:
    ;同一端口，写时表示写入命令,读时表示读入硬盘状态
    nop
    in al, dx
    and al, 0x88    ;第3位为1表示硬盘控制器已经准备好数据传输
                    ;第7位为1表示硬盘忙
    cmp al, 0x08
    jnz .not_ready  ;若为准备好,则跳回not_ready处继续等待

;第五步:从0x1f0端口读入数据
    mov ax, di   ;di为待读入的扇区数
    mov dx, 256  ;一个扇区512字节,每次读入一字,即两字节,共要读256次
    mul dx
    mov cx, ax
    mov dx, 0x1f0

.go_on_read:
    in ax, dx
    mov [bx], ax
    add bx, 2
    loop .go_on_read
    ret

;填充空白符,保证整段程序大小为512字节
times 510-($-$$) db 0

;定义MBR中最后两个字节的魔数，表示这个扇区包含可加载的程序
db 0x55, 0xaa
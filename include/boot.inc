;------------------------
; 内核加载器属性
;------------------------

; 加载器在硬盘上的位置（LBA）
LOADER_START_SECTOR  equ 0x2
; 加载器加载到内存中的位置
LOADER_BASE_ADDR equ 0x900


;------------------------
; 全局描述符表（GDT）属性
;------------------------

; G 位（第 23 位）-> 段边界的单位：4KB
DESC_G_4K  equ 1000_0000_0000_0000_0000_0000b

; D 位（第 22 位）-> 32 位操作数
DESC_D_32  equ  100_0000_0000_0000_0000_0000b

; L 位（第 21 位）-> 32 位代码段
DESC_L_0   equ   00_0000_0000_0000_0000_0000b

; AVL 位（第 20 位）-> 硬件不管这一位，置0即可
DESC_AVL_0 equ    0_0000_0000_0000_0000_0000b

; 段界限的第二部分（第 16~19 位）-> 2^20 * 2^12 = 4GB
DESC_LIMIT_CODE2    equ 1111_0000_0000_0000_0000b
DESC_LIMIT_DATA2    equ 1111_0000_0000_0000_0000b
DESC_LIMIT_DISPLAY2 equ 0000_0000_0000_0000_0000b
DESC_LIMIT_VIDEO2   equ 0000_0000_0000_0000_0000b

; P 位（第 15 位）-> 段在内存中
DESC_P_1    equ 1000_0000_0000_0000b

; DPL（13~14 位）-> 描述符特权级别的 4 个可能值
DESC_DPL_0  equ  000_0000_0000_0000b
DESC_DPL_1  equ  010_0000_0000_0000b
DESC_DPL_2  equ  100_0000_0000_0000b
DESC_DPL_3  equ  110_0000_0000_0000b

; S 位（第 12 位）-> 描述段是系统段还是非系统段
DESC_S_CODE equ    1_0000_0000_0000b
DESC_S_DATA equ    1_0000_0000_0000b
DESC_S_sys  equ    0_0000_0000_0000b

; TYPE 位（第 8~11 位）-> 代码（可执行）/ 数据（可读）段的类型
DESC_TYPE_CODE equ   1000_0000_0000b
DESC_TYPE_DATA equ   0010_0000_0000b

; 描述符的高 4 字节
DESC_CODE_HIGH4  equ (0x00<<24) + DESC_G_4K + DESC_D_32 + DESC_L_0 + \
    DESC_AVL_0 + DESC_LIMIT_CODE2  + DESC_P_1 + DESC_DPL_0 + DESC_S_CODE + \
    + DESC_TYPE_CODE + 0x00

DESC_DATA_HIGH4  equ (0x00<<24) + DESC_G_4K + DESC_D_32 + DESC_L_0 + \
    DESC_AVL_0 + DESC_LIMIT_DATA2  + DESC_P_1 + DESC_DPL_0 + DESC_S_DATA + \
    + DESC_TYPE_DATA + 0x00

DESC_VIDEO_HIGH4 equ (0x00<<24) + DESC_G_4K + DESC_D_32 + DESC_L_0 + \
    DESC_AVL_0 + DESC_LIMIT_VIDEO2 + DESC_P_1 + DESC_DPL_0 + DESC_S_DATA + \
    + DESC_TYPE_DATA + 0x0b

DESC_DISPLAY_HIGH4 equ (0x00<<24) + DESC_G_4K + DESC_D_32 + DESC_L_0 + \
    DESC_AVL_0 + DESC_LIMIT_DISPLAY2 + DESC_P_1 + DESC_DPL_0 + DESC_S_DATA + \
    + DESC_TYPE_DATA + 0x0b

;------------------------
; 选择子属性
;------------------------

; RPL 位（第 0~1 位）-> RPL 的 4 个可能值
RPL_0  equ  00b
RPL_1  equ  01b
RPL_2  equ  10b
RPL_3  equ  11b

; TI 位（第 2 位）-> 指示段描述符在 GDT 中还是在 LDT 中
TI_GDT equ 000b
TI_LDT equ 100b

;------------------------------------
; 页目录表属性
;------------------------------------

; 页目录表的起始地址
PAGE_DIR_TABLE_POS equ 0x100000

;------------------------------------
; 页表属性
;------------------------------------

; present 位 -> 表示页是否存在于内存中
PG_P equ 1b

; 读写位
PG_RW_R equ 00b
PG_RW_W equ 10b

; 用户和根位 -> 是否可以被特权级别为 3 的程序访问
PG_US_S equ 000b
PG_US_U equ 100b

;------------------------------------
; ELF 段相关值
;------------------------------------
PT_NULL equ 0

;------------------------------------
; 内核属性
;------------------------------------

;kernel.bin 所在的扇区号
KERNEL_START_SECTOR equ 0x9 

;从磁盘读出后要写到内存中的地址
KERNEL_BIN_BASE_ADDR equ 0x70000

;解析完后要写到内存中的地址
KERNEL_ENTRY_POINT equ 0xc0001500

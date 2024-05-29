[bits 32]
section .text
global switch_to
switch_to:
    push esi
    push edi
    push ebx
    push ebp

    ; 从参数'cur_thread'中获取当前线程的 PCB 地址（即 self_kstack），并将 esp 存储到此 PCB 中
    mov eax, [esp+20]
    mov [eax], esp

    ; 从参数'next'中检索下一个线程的 PCB 地址。
    mov eax,[esp+24]
    mov esp, [eax]

    pop ebp
    pop ebx
    pop edi
    pop esi
    ret
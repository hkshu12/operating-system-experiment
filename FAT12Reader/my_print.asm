section .text
global myPrint
myPrint:
	mov    edx, [esp+16]
    mov    ecx, [esp+12]
    mov    ebx, 1
    mov    eax, 4
	int    80h

    mov    edx, [esp+8]
    mov    ecx, [esp+4]
    mov    ebx, 1
    mov    eax, 4
    int    80h
	ret

section .data
    msg  db  "Please input x and y:", 0Ah, 0h

section .bss
    sinput  resb 60
    x       resb 30
    xlen    resb 1
    y       resb 30
    ylen    resb 1
    addRes  resb 31
    multRes resb 61
    resLen  resb 1

section .text
global _start
_start:
    mov  eax,msg
    call sprint
input:
; read x & y
    mov  eax, 3
    mov  ebx, 0
    mov  ecx, sinput
    mov  edx, 30
    int  80h

; memory allocate x & y
    xor  eax, eax
    xor  ebx, ebx
    xor  ecx, ecx
    xor  edx, edx
loop1:
    cmp  byte [sinput + eax], ' '
    jz   loop2
    inc  eax
    jmp  loop1
loop2:
    cmp  ebx, eax
    jz   loop3
    mov  cl, byte [sinput + ebx]
    mov  byte [x + ebx], cl
    inc  ebx
    jmp  loop2
loop3:
    cmp  byte [sinput + ebx], 0
    jz   processFinished
    cmp  byte [sinput + ebx], ' '
    jz   spaceProcess
    mov  cl, byte [sinput + ebx]
    mov  edx, ebx
    sub  edx, 1
    sub  edx, eax
    mov  byte [y + edx], cl
spaceProcess:
    inc  ebx
    jmp  loop3

processFinished:
    mov  byte [xlen], al
    add  edx, 1
    mov  byte [ylen], dl
    
    xor  ebx, ebx
    mov  eax, x
    mov  bl, byte [xlen]
    call strToInt
    
    mov  eax, y
    mov  bl, byte [ylen]
    call strToInt
    
    mov  ecx, 0Ah
    push ecx
    mov  edx, 1
    mov  ecx, esp
    mov  ebx, 1
    mov  eax, 4
    int  80h
    pop  ecx

addPrepare:
    xor  eax, eax
    xor  ebx, ebx
    mov  al, byte [xlen]
    mov  bl, byte [ylen]
    mov  byte [resLen], al
    cmp  eax, ebx
    jg   addStart
    mov  byte [resLen], bl
addStart:
    xor  ecx, ecx
    xor  edx, edx
addLoop:
    cmp  ecx, [resLen]
    jg   addLoopFinished
    dec  eax
    dec  ebx
eaxProcess:
    cmp  eax, 0
    jl   eaxOutOfBounds
    mov  dl, byte [x + eax]
    jmp  ebxProcess
eaxOutOfBounds:
    mov  dl, 0
ebxProcess:
    cmp  ebx, 0
    jl   ebxOutOfBounds
    add  dl, byte [y + ebx]
    jmp  continue
ebxOutOfBounds:
    add  dl, 0
continue:
    mov  byte [addRes + ecx], dl
    inc  ecx
    jmp  addLoop
addLoopFinished:
    mov  eax, addRes
    mov  ebx, [resLen]
    call carry
    call numPrinter
multPrepare:
    xor  eax, eax
    xor  ebx, ebx
    mov  al, [xlen]
    mov  bl, [ylen]
    add  eax, ebx
    mov  byte [resLen], al
    mov  al, [xlen]
multStart:
    xor  esi, esi
    xor  edi, edi
    xor  ecx, ecx
    xor  edx, edx
multiplierLoop:
    dec  eax
    cmp  eax, 0
    jl   multiplierLoopFinished
multiplicandLoop:
    dec  ebx
    cmp  ebx, 0
    jl   multiplicandLoopFinished
    xor  ecx, ecx
    mov  cl, byte [x + eax]
    mov  ch, byte [y + ebx]
    push eax
    xor  eax, eax
    mov  al, cl
    mov  ah, ch
    mul  ah
    xor  ecx, ecx
    mov  cl, byte [ylen]
    sub  ecx, ebx
    sub  ecx, 1
    mov  esi, ecx
    add  esi, edx
    add  dword [multRes + esi], eax
    pop  eax
    jmp  multiplicandLoop
multiplicandLoopFinished:
    xor  ebx, ebx
    mov  bl, byte [ylen]
    inc  edx
    
    push eax
    
    mov  eax, multRes
    xor  ebx, ebx
    mov  bl, byte [resLen]
    sub  ebx, 1
    call carry
    pop  eax
    
    jmp  multiplierLoop
multiplierLoopFinished:
    mov  eax, multRes
    xor  ebx, ebx
    mov  bl, byte [resLen]
    sub  ebx, 1
    call carry
    call numPrinter
    
    call quit

; Here are the functions.
;------------------------
; numPrinter
numPrinter:
    push edx
    push ecx
    push eax
    push ebx
.printLoop:
    cmp  ebx, 0
    jl   .printFinished
    pop  ecx
    push ecx
    cmp  ebx, ecx
    jz   .headZeroProcess
    jmp  .continue
.headZeroProcess:
    cmp  byte [eax + ecx], 0
    jz   .nextPrintLoop
.continue:
    add  byte [eax + ebx], 48
    
    push eax
    push ebx
    
    mov  edx, 1
    mov  ecx, ebx
    add  ecx, eax
    mov  ebx, 1
    mov  eax, 4
    int  80h
    
    pop  ebx
    pop  eax
.nextPrintLoop:
    dec  ebx
    jmp  .printLoop
.printFinished:
    mov  ecx, 0Ah
    push ecx
    mov  edx, 1
    mov  ecx, esp
    mov  ebx, 1
    mov  eax, 4
    int  80h
    pop  ecx
    
    pop  ebx
    pop  eax
    pop  ecx
    pop  edx
    ret
; carry
carry:
    push ecx
    push edx
    xor  ecx, ecx
    xor  edx, edx  
.carryingLoop:
    cmp  ecx, ebx
    jz   .outerLoopFinished
    xor  edx, edx
.countCarryingLoop:
    cmp  byte [eax + ecx], 10
    jl   .innerLoopFinished
    sub  byte [eax + ecx], 10
    inc  edx
    jmp  .countCarryingLoop
.innerLoopFinished:
    inc  ecx
    add  byte [eax + ecx], dl
    jmp  .carryingLoop
.outerLoopFinished:
    pop  edx
    pop  ecx
    ret
; strToInt
strToInt:
    push ecx
    push edx
    xor  ecx, ecx
    xor  edx, edx
.loop:
    cmp  ecx, ebx
    jz   .finished
    mov  dl, byte [eax + ecx]
    sub  edx, 48
    mov  byte [eax + ecx], dl
    inc  ecx
    jmp  .loop
.finished:
    pop  edx
    pop  ecx
    ret
;------------------------
; slen
; eax stores the length of str in ebx
slen:
    push ebx
    mov  ebx, eax

.nextChar:
    cmp  byte [eax], 0
    jz   .finished
    inc  eax
    jmp  .nextChar

.finished:
    sub  eax, ebx
    pop  ebx
    ret

;-------------------
; sprint
; print str in [eax]
sprint:
    push edx
    push ecx
    push ebx
    push eax
    call slen

    mov  edx, eax
    pop  eax

    mov  ecx, eax
    mov  ebx, 1
    mov  eax, 4
    int  80h

    pop  ebx
    pop  ecx
    pop  edx
    ret

;---------------
; quit
quit:
    mov  ebx, 0
    mov  eax, 1
    int  80h
    ret

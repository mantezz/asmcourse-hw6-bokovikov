section .data
    const_3       dq 3.0
    const_0_5     dq 0.5
    const_1       dq 1.0
    const_2_5     dq 2.5
    const_9_5     dq 9.5
    const_5       dq 5.0
    const_neg5    dq -5.0
    const_nan     dq 0xFFF8000000000000

section .text
global f1, f2, f3, df1, df2, df3
; f1(x) = 3*(0.5/(x + 1) + 1)
f1:
    push ebp
    mov ebp, esp

    fld qword [ebp + 8]
    fld qword [const_1]
    faddp st1, st0
    ftst
    fstsw ax
    sahf
    je f1_invalid

    fld qword [const_0_5]
    fdivrp st1, st0
    fld qword [const_1]
    faddp st1, st0
    fld qword [const_3]
    fmulp st1, st0
    jmp f1_end

f1_invalid:
    ffree st0
    fld qword [const_nan]

f1_end:
    pop ebp
    ret

df1:
    push ebp
    mov ebp, esp

    fld qword [ebp + 8]
    fld qword [const_1]
    faddp st1, st0
    ftst
    fstsw ax
    sahf
    je df1_invalid

    fld st0
    fmulp st1, st0
    fld qword [const_neg5]
    fdivrp st1, st0
    jmp df1_end

df1_invalid:
    ffree st0
    fld qword [const_nan]

df1_end:
    pop ebp
    ret

; f2(x) = 2.5x - 9.5
f2:
    push ebp
    mov ebp, esp

    fld qword [ebp + 8]
    fld qword [const_2_5]
    fmulp st1, st0
    fld qword [const_9_5]
    fsubp st1, st0

    pop ebp
    ret

;f2'(x) = 2.5
df2:
    push ebp
    mov ebp, esp

    fld qword [const_2_5]

    pop ebp
    ret

; f3(x) = 5/x (x > 0)
f3:
    push ebp
    mov ebp, esp

    fld qword [ebp + 8]
    ftst
    fstsw ax
    sahf
    jbe f3_invalid

    fld qword [const_5]
    fdivrp st1, st0
    jmp f3_end

f3_invalid:
    ffree st0
    fld qword [const_nan]

f3_end:
    pop ebp
    ret

;f3'(x) = -5/x^2
df3:
    push ebp
    mov ebp, esp

    fld qword [ebp + 8]
    ftst
    fstsw ax
    sahf
    jbe df3_invalid

    fld st0
    fmulp st1, st0
    fld qword [const_neg5]
    fdivrp st1, st0
    jmp df3_end

df3_invalid:
    ffree st0
    fld qword [const_nan]

df3_end:
    pop ebp
    ret

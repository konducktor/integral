section .data
    four_integer dq 4
    eight_integer dq 8
    three_integer dq 3
    two_integer dq 2

section .text
global f1, df1, f2, df2, f3, df3
f1: ; 1 + 4 / (x^2 + 1)
	push ebp
	mov ebp, esp

        finit
        fld qword[ebp+8]
        fld qword[ebp+8]
        fmulp
        fld1
        faddp
        fild qword[four_integer]
        fxch
        fdivp
        fld1
        faddp

	mov esp, ebp
	pop ebp
	ret

df1: ; -8x / (x^2 + 1)^2
    push ebp
    mov ebp, esp

    finit
    fild qword[eight_integer]
    fchs
    fld qword[ebp+8]
    fmulp
    fld qword[ebp+8]
    fld st0
    fmulp
    fld1
    faddp
    fld st0
    fmulp
    fdivp

    mov esp, ebp
    pop ebp
    ret

f2: ; x^3
    push ebp
    mov ebp, esp

    finit
    fld qword[ebp+8]
    fld qword[ebp+8]
    fmulp
    fld qword[ebp+8]
    fmulp

    mov esp, ebp
    pop ebp
    ret

df2: ; 3x^2
    push ebp
    mov ebp, esp

    finit
    fild qword[three_integer]
    fld qword[ebp+8]
    fld qword[ebp+8]
    fmulp
    fmulp

    mov esp, ebp
    pop ebp
    ret

f3: ; 2^-x
    push ebp
    mov ebp, esp

    fld qword[ebp+8]
    fchs
    fld st0
    frndint
    fsub st1, st0
    fxch
    f2xm1
    fld1
    faddp
    fscale
    fstp st1

    mov esp, ebp
    pop ebp
    ret

df3: ; -ln(2) * 2^-x
    push ebp
    mov ebp, esp

    fld qword[ebp+8]
    fchs
    fld st0
    frndint
    fsub st1, st0
    fxch
    f2xm1
    fld1
    faddp
    fscale
    fstp st1
    fldln2
    fmul
    fchs

    mov esp, ebp
    pop ebp
    ret

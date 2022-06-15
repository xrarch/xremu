	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 10, 15	sdk_version 10, 15, 4
	.globl	_CPUReset               ## -- Begin function CPUReset
	.p2align	4, 0x90
_CPUReset:                              ## @CPUReset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	_CurrentException@GOTPCREL(%rip), %rax
	movq	_ControlReg@GOTPCREL(%rip), %rcx
	movl	$-131072, _PC(%rip)     ## imm = 0xFFFE0000
	movl	$0, (%rcx)
	movl	$0, 16(%rcx)
	movl	$0, (%rax)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_CPUDoCycles            ## -- Begin function CPUDoCycles
	.p2align	4, 0x90
_CPUDoCycles:                           ## @CPUDoCycles
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$256, %rsp              ## imm = 0x100
	movl	%edi, -8(%rbp)
	testb	$1, _Running(%rip)
	jne	LBB1_2
## %bb.1:
	movl	-8(%rbp), %eax
	movl	%eax, -4(%rbp)
	jmp	LBB1_240
LBB1_2:
	testb	$1, _UserBreak(%rip)
	je	LBB1_5
## %bb.3:
	movq	_CurrentException@GOTPCREL(%rip), %rax
	cmpl	$0, (%rax)
	jne	LBB1_5
## %bb.4:
	movl	$6, %edi
	callq	_Limn2500Exception
	movb	$0, _UserBreak(%rip)
LBB1_5:
	testb	$1, _Halted(%rip)
	je	LBB1_12
## %bb.6:
	movq	_CurrentException@GOTPCREL(%rip), %rax
	cmpl	$0, (%rax)
	jne	LBB1_9
## %bb.7:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	(%rax), %ecx
	andl	$2, %ecx
	cmpl	$0, %ecx
	je	LBB1_10
## %bb.8:
	movq	_LSICInterruptPending@GOTPCREL(%rip), %rax
	testb	$1, (%rax)
	je	LBB1_10
LBB1_9:
	movb	$0, _Halted(%rip)
	jmp	LBB1_11
LBB1_10:
	movl	-8(%rbp), %eax
	movl	%eax, -4(%rbp)
	jmp	LBB1_240
LBB1_11:
	jmp	LBB1_12
LBB1_12:
	movl	$0, -12(%rbp)
LBB1_13:                                ## =>This Loop Header: Depth=1
                                        ##     Child Loop BB1_137 Depth 2
                                        ##     Child Loop BB1_143 Depth 2
                                        ##     Child Loop BB1_151 Depth 2
                                        ##     Child Loop BB1_125 Depth 2
	movl	-12(%rbp), %eax
	cmpl	-8(%rbp), %eax
	jae	LBB1_239
## %bb.14:                              ##   in Loop: Header=BB1_13 Depth=1
	movq	_CPUProgress@GOTPCREL(%rip), %rax
	cmpl	$0, (%rax)
	jg	LBB1_16
## %bb.15:
	movl	-8(%rbp), %eax
	movl	%eax, -4(%rbp)
	jmp	LBB1_240
LBB1_16:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_CurrentException@GOTPCREL(%rip), %rax
	cmpl	$0, (%rax)
	jne	LBB1_19
## %bb.17:                              ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	(%rax), %ecx
	andl	$2, %ecx
	cmpl	$0, %ecx
	je	LBB1_37
## %bb.18:                              ##   in Loop: Header=BB1_13 Depth=1
	movq	_LSICInterruptPending@GOTPCREL(%rip), %rax
	testb	$1, (%rax)
	je	LBB1_37
LBB1_19:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_CurrentException@GOTPCREL(%rip), %rax
	movq	_ControlReg@GOTPCREL(%rip), %rcx
	movl	(%rcx), %edx
	andl	$252, %edx
	movl	%edx, -16(%rbp)
	cmpl	$3, (%rax)
	jne	LBB1_21
## %bb.20:                              ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	36(%rax), %ecx
	movl	%ecx, -20(%rbp)
	movl	-16(%rbp), %ecx
	andl	$248, %ecx
	movl	%ecx, -16(%rbp)
	jmp	LBB1_27
LBB1_21:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_CurrentException@GOTPCREL(%rip), %rax
	cmpl	$15, (%rax)
	jne	LBB1_23
## %bb.22:                              ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	32(%rax), %ecx
	movl	%ecx, -20(%rbp)
	movl	-16(%rbp), %ecx
	andl	$248, %ecx
	movl	%ecx, -16(%rbp)
	jmp	LBB1_26
LBB1_23:                                ##   in Loop: Header=BB1_13 Depth=1
	movl	-16(%rbp), %eax
	andl	$128, %eax
	cmpl	$0, %eax
	je	LBB1_25
## %bb.24:                              ##   in Loop: Header=BB1_13 Depth=1
	movl	-16(%rbp), %eax
	andl	$248, %eax
	movl	%eax, -16(%rbp)
LBB1_25:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	16(%rax), %ecx
	movl	%ecx, -20(%rbp)
LBB1_26:                                ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_27
LBB1_27:                                ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -20(%rbp)
	jne	LBB1_29
## %bb.28:                              ##   in Loop: Header=BB1_13 Depth=1
	callq	_CPUReset
	jmp	LBB1_36
LBB1_29:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_CurrentException@GOTPCREL(%rip), %rax
	cmpl	$0, (%rax)
	jne	LBB1_31
## %bb.30:                              ##   in Loop: Header=BB1_13 Depth=1
	movq	_CurrentException@GOTPCREL(%rip), %rax
	movl	$1, (%rax)
LBB1_31:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_CurrentException@GOTPCREL(%rip), %rax
	movl	(%rax), %ecx
	movl	%ecx, %edx
	decl	%edx
	subl	$3, %edx
	movl	%ecx, -124(%rbp)        ## 4-byte Spill
	jb	LBB1_32
	jmp	LBB1_241
LBB1_241:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	-124(%rbp), %eax        ## 4-byte Reload
	subl	$15, %eax
	je	LBB1_33
	jmp	LBB1_34
LBB1_32:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	_PC(%rip), %ecx
	movl	%ecx, 12(%rax)
	jmp	LBB1_35
LBB1_33:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_CurrentException@GOTPCREL(%rip), %rax
	movq	_ControlReg@GOTPCREL(%rip), %rcx
	movl	(%rcx), %edx
	shrl	$28, %edx
	movl	%edx, (%rax)
	movl	_PC(%rip), %edx
	subl	$4, %edx
	movl	%edx, _TLBPC(%rip)
	movb	$1, _TLBMiss(%rip)
	jmp	LBB1_35
LBB1_34:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	_PC(%rip), %ecx
	subl	$4, %ecx
	movl	%ecx, 12(%rax)
LBB1_35:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movq	_CurrentException@GOTPCREL(%rip), %rcx
	movl	-20(%rbp), %edx
	movl	%edx, _PC(%rip)
	movl	(%rcx), %edx
	shll	$28, %edx
	movl	(%rax), %esi
	andl	$65535, %esi            ## imm = 0xFFFF
	shll	$8, %esi
	orl	%esi, %edx
	orl	-16(%rbp), %edx
	movl	%edx, (%rax)
LBB1_36:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_CurrentException@GOTPCREL(%rip), %rax
	movl	$0, (%rax)
LBB1_37:                                ##   in Loop: Header=BB1_13 Depth=1
	movl	_PC(%rip), %eax
	movl	%eax, -24(%rbp)
	movl	_PC(%rip), %eax
	addl	$4, %eax
	movl	%eax, _PC(%rip)
	movb	$1, _IFetch(%rip)
	movl	-24(%rbp), %edi
	leaq	-28(%rbp), %rsi
	callq	_CPUReadLong
	andb	$1, %al
	movzbl	%al, %ecx
	movl	%ecx, -72(%rbp)
	movb	$0, _IFetch(%rip)
	cmpl	$0, -72(%rbp)
	je	LBB1_234
## %bb.38:                              ##   in Loop: Header=BB1_13 Depth=1
	movl	-28(%rbp), %eax
	andl	$7, %eax
	movl	%eax, -32(%rbp)
	movl	-28(%rbp), %eax
	andl	$63, %eax
	movl	%eax, -36(%rbp)
	cmpl	$7, -32(%rbp)
	jne	LBB1_40
## %bb.39:                              ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	_PC(%rip), %ecx
	movl	%ecx, 124(%rax)
	movl	-24(%rbp), %ecx
	andl	$-2147483648, %ecx      ## imm = 0x80000000
	movl	-28(%rbp), %edx
	shrl	$3, %edx
	shll	$2, %edx
	orl	%edx, %ecx
	movl	%ecx, _PC(%rip)
	jmp	LBB1_233
LBB1_40:                                ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$6, -32(%rbp)
	jne	LBB1_42
## %bb.41:                              ##   in Loop: Header=BB1_13 Depth=1
	movl	-24(%rbp), %eax
	andl	$-2147483648, %eax      ## imm = 0x80000000
	movl	-28(%rbp), %ecx
	shrl	$3, %ecx
	shll	$2, %ecx
	orl	%ecx, %eax
	movl	%eax, _PC(%rip)
	jmp	LBB1_232
LBB1_42:                                ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$57, -36(%rbp)
	jne	LBB1_85
## %bb.43:                              ##   in Loop: Header=BB1_13 Depth=1
	movl	-28(%rbp), %eax
	shrl	$28, %eax
	movl	%eax, -40(%rbp)
	movl	-28(%rbp), %eax
	shrl	$26, %eax
	andl	$3, %eax
	movl	%eax, -48(%rbp)
	movl	-28(%rbp), %eax
	shrl	$21, %eax
	andl	$31, %eax
	movl	%eax, -44(%rbp)
	movl	-28(%rbp), %eax
	shrl	$6, %eax
	andl	$31, %eax
	movl	%eax, -56(%rbp)
	movl	-28(%rbp), %eax
	shrl	$11, %eax
	andl	$31, %eax
	movl	%eax, -60(%rbp)
	movl	-28(%rbp), %eax
	shrl	$16, %eax
	andl	$31, %eax
	movl	%eax, -64(%rbp)
	cmpl	$0, -44(%rbp)
	je	LBB1_50
## %bb.44:                              ##   in Loop: Header=BB1_13 Depth=1
	movl	-48(%rbp), %eax
	movl	%eax, %ecx
	movq	%rcx, %rdx
	subq	$3, %rdx
	movq	%rcx, -136(%rbp)        ## 8-byte Spill
	ja	LBB1_49
## %bb.245:                             ##   in Loop: Header=BB1_13 Depth=1
	leaq	LJTI1_3(%rip), %rax
	movq	-136(%rbp), %rcx        ## 8-byte Reload
	movslq	(%rax,%rcx,4), %rdx
	addq	%rax, %rdx
	jmpq	*%rdx
LBB1_45:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-44(%rbp), %esi
	movl	%ecx, -140(%rbp)        ## 4-byte Spill
	movl	%esi, %ecx
                                        ## kill: def $cl killed $ecx
	movl	-140(%rbp), %esi        ## 4-byte Reload
	shll	%cl, %esi
	movl	%esi, -52(%rbp)
	jmp	LBB1_49
LBB1_46:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-44(%rbp), %esi
	movl	%ecx, -144(%rbp)        ## 4-byte Spill
	movl	%esi, %ecx
                                        ## kill: def $cl killed $ecx
	movl	-144(%rbp), %esi        ## 4-byte Reload
	shrl	%cl, %esi
	movl	%esi, -52(%rbp)
	jmp	LBB1_49
LBB1_47:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-44(%rbp), %esi
	movl	%ecx, -148(%rbp)        ## 4-byte Spill
	movl	%esi, %ecx
                                        ## kill: def $cl killed $ecx
	movl	-148(%rbp), %esi        ## 4-byte Reload
	sarl	%cl, %esi
	movl	%esi, -52(%rbp)
	jmp	LBB1_49
LBB1_48:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %edi
	movl	-44(%rbp), %esi
	callq	_RoR
	movl	%eax, -52(%rbp)
LBB1_49:                                ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_51
LBB1_50:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	%ecx, -52(%rbp)
LBB1_51:                                ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_54
## %bb.52:                              ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$9, -40(%rbp)
	jb	LBB1_84
## %bb.53:                              ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$11, -40(%rbp)
	ja	LBB1_84
LBB1_54:                                ##   in Loop: Header=BB1_13 Depth=1
	movl	-40(%rbp), %eax
	movl	%eax, %ecx
	movq	%rcx, %rdx
	subq	$15, %rdx
	movq	%rcx, -160(%rbp)        ## 8-byte Spill
	ja	LBB1_82
## %bb.246:                             ##   in Loop: Header=BB1_13 Depth=1
	leaq	LJTI1_4(%rip), %rax
	movq	-160(%rbp), %rcx        ## 8-byte Reload
	movslq	(%rax,%rcx,4), %rdx
	addq	%rax, %rdx
	jmpq	*%rdx
LBB1_55:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	orl	-52(%rbp), %ecx
	xorl	$-1, %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_83
LBB1_56:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	orl	-52(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_83
LBB1_57:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	xorl	-52(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_83
LBB1_58:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	andl	-52(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_83
LBB1_59:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	cmpl	-52(%rbp), %ecx
	jge	LBB1_61
## %bb.60:                              ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	$1, (%rax,%rdx,4)
	jmp	LBB1_62
LBB1_61:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	$0, (%rax,%rdx,4)
LBB1_62:                                ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_83
LBB1_63:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	cmpl	-52(%rbp), %ecx
	jae	LBB1_65
## %bb.64:                              ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	$1, (%rax,%rdx,4)
	jmp	LBB1_66
LBB1_65:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	$0, (%rax,%rdx,4)
LBB1_66:                                ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_83
LBB1_67:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	subl	-52(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_83
LBB1_68:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	addl	-52(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_83
LBB1_69:                                ##   in Loop: Header=BB1_13 Depth=1
	movl	-48(%rbp), %eax
	movl	%eax, %ecx
	movq	%rcx, %rdx
	subq	$3, %rdx
	movq	%rcx, -168(%rbp)        ## 8-byte Spill
	ja	LBB1_74
## %bb.247:                             ##   in Loop: Header=BB1_13 Depth=1
	leaq	LJTI1_5(%rip), %rax
	movq	-168(%rbp), %rcx        ## 8-byte Reload
	movslq	(%rax,%rcx,4), %rdx
	addq	%rax, %rdx
	jmpq	*%rdx
LBB1_70:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-60(%rbp), %esi
	movl	%esi, %edx
	movl	(%rax,%rdx,4), %esi
	movl	%ecx, -172(%rbp)        ## 4-byte Spill
	movl	%esi, %ecx
                                        ## kill: def $cl killed $ecx
	movl	-172(%rbp), %esi        ## 4-byte Reload
	shll	%cl, %esi
	movl	-56(%rbp), %edi
	movl	%edi, %edx
	movl	%esi, (%rax,%rdx,4)
	jmp	LBB1_74
LBB1_71:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-60(%rbp), %esi
	movl	%esi, %edx
	movl	(%rax,%rdx,4), %esi
	movl	%ecx, -176(%rbp)        ## 4-byte Spill
	movl	%esi, %ecx
                                        ## kill: def $cl killed $ecx
	movl	-176(%rbp), %esi        ## 4-byte Reload
	shrl	%cl, %esi
	movl	-56(%rbp), %edi
	movl	%edi, %edx
	movl	%esi, (%rax,%rdx,4)
	jmp	LBB1_74
LBB1_72:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-60(%rbp), %esi
	movl	%esi, %edx
	movl	(%rax,%rdx,4), %esi
	movl	%ecx, -180(%rbp)        ## 4-byte Spill
	movl	%esi, %ecx
                                        ## kill: def $cl killed $ecx
	movl	-180(%rbp), %esi        ## 4-byte Reload
	sarl	%cl, %esi
	movl	-56(%rbp), %edi
	movl	%edi, %edx
	movl	%esi, (%rax,%rdx,4)
	jmp	LBB1_74
LBB1_73:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %edi
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %esi
	callq	_RoR
	movq	_Reg@GOTPCREL(%rip), %rdx
	movl	-56(%rbp), %ecx
	movl	%ecx, %r8d
	movl	%eax, (%rdx,%r8,4)
LBB1_74:                                ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_83
LBB1_75:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	addl	-52(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	(%rax,%rdx,4), %esi
	movl	%ecx, %edi
	callq	_CPUWriteLong
	jmp	LBB1_83
LBB1_76:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	addl	-52(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	(%rax,%rdx,4), %esi
	movl	%ecx, %edi
	callq	_CPUWriteInt
	jmp	LBB1_83
LBB1_77:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	addl	-52(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	(%rax,%rdx,4), %esi
	movl	%ecx, %edi
	callq	_CPUWriteByte
	jmp	LBB1_83
LBB1_78:                                ##   in Loop: Header=BB1_13 Depth=1
	movl	$7, %edi
	callq	_Limn2500Exception
	jmp	LBB1_83
LBB1_79:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	addl	-52(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	shlq	$2, %rdx
	addq	%rdx, %rax
	movl	%ecx, %edi
	movq	%rax, %rsi
	callq	_CPUReadLong
	jmp	LBB1_83
LBB1_80:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	addl	-52(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	shlq	$2, %rdx
	addq	%rdx, %rax
	movl	%ecx, %edi
	movq	%rax, %rsi
	callq	_CPUReadInt
	jmp	LBB1_83
LBB1_81:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	addl	-52(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	shlq	$2, %rdx
	addq	%rdx, %rax
	movl	%ecx, %edi
	movq	%rax, %rsi
	callq	_CPUReadByte
	jmp	LBB1_83
LBB1_82:
	callq	_abort
LBB1_83:                                ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_84
LBB1_84:                                ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_231
LBB1_85:                                ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$49, -36(%rbp)
	jne	LBB1_117
## %bb.86:                              ##   in Loop: Header=BB1_13 Depth=1
	movl	-28(%rbp), %eax
	shrl	$28, %eax
	movl	%eax, -40(%rbp)
	movl	-28(%rbp), %eax
	shrl	$6, %eax
	andl	$31, %eax
	movl	%eax, -56(%rbp)
	movl	-28(%rbp), %eax
	shrl	$11, %eax
	andl	$31, %eax
	movl	%eax, -60(%rbp)
	movzwl	-26(%rbp), %eax
	andl	$31, %eax
	movl	%eax, -64(%rbp)
	movl	-40(%rbp), %eax
	movl	%eax, %ecx
	movq	%rcx, %rdx
	subq	$15, %rdx
	movq	%rcx, -192(%rbp)        ## 8-byte Spill
	ja	LBB1_115
## %bb.244:                             ##   in Loop: Header=BB1_13 Depth=1
	leaq	LJTI1_2(%rip), %rax
	movq	-192(%rbp), %rcx        ## 8-byte Reload
	movslq	(%rax,%rcx,4), %rdx
	addq	%rax, %rdx
	jmpq	*%rdx
LBB1_87:                                ##   in Loop: Header=BB1_13 Depth=1
	movl	$2, %edi
	callq	_Limn2500Exception
	jmp	LBB1_116
LBB1_88:                                ##   in Loop: Header=BB1_13 Depth=1
	movl	$6, %edi
	callq	_Limn2500Exception
	jmp	LBB1_116
LBB1_89:                                ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, _CPULocked(%rip)
	je	LBB1_91
## %bb.90:                              ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %edi
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %esi
	callq	_CPUWriteLong
LBB1_91:                                ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_93
## %bb.92:                              ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_116
LBB1_93:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	_CPULocked(%rip), %ecx
	movl	-56(%rbp), %edx
	movl	%edx, %esi
	movl	%ecx, (%rax,%rsi,4)
	jmp	LBB1_116
LBB1_94:                                ##   in Loop: Header=BB1_13 Depth=1
	movl	$1, _CPULocked(%rip)
	cmpl	$0, -56(%rbp)
	jne	LBB1_96
## %bb.95:                              ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_116
LBB1_96:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %edi
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	shlq	$2, %rdx
	addq	%rdx, %rax
	movq	%rax, %rsi
	callq	_CPUReadLong
	jmp	LBB1_116
LBB1_97:                                ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_99
## %bb.98:                              ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_116
LBB1_99:                                ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	cmpl	$0, (%rax,%rdx,4)
	jne	LBB1_101
## %bb.100:                             ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	$0, (%rax,%rdx,4)
	jmp	LBB1_116
LBB1_101:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-64(%rbp), %esi
	movl	%esi, %edx
	movq	%rax, -200(%rbp)        ## 8-byte Spill
	movl	%ecx, %eax
	xorl	%ecx, %ecx
	movq	%rdx, -208(%rbp)        ## 8-byte Spill
	movl	%ecx, %edx
	movq	-200(%rbp), %rdi        ## 8-byte Reload
	movq	-208(%rbp), %r8         ## 8-byte Reload
	divl	(%rdi,%r8,4)
	movl	-56(%rbp), %ecx
	movl	%ecx, %r9d
	movl	%edx, (%rdi,%r9,4)
	jmp	LBB1_116
LBB1_102:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_104
## %bb.103:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_116
LBB1_104:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	cmpl	$0, (%rax,%rdx,4)
	jne	LBB1_106
## %bb.105:                             ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	$0, (%rax,%rdx,4)
	jmp	LBB1_116
LBB1_106:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-64(%rbp), %esi
	movl	%esi, %edx
	movq	%rax, -216(%rbp)        ## 8-byte Spill
	movl	%ecx, %eax
	movq	%rdx, -224(%rbp)        ## 8-byte Spill
	cltd
	movq	-216(%rbp), %rdi        ## 8-byte Reload
	movq	-224(%rbp), %r8         ## 8-byte Reload
	idivl	(%rdi,%r8,4)
	movl	-56(%rbp), %ecx
	movl	%ecx, %r9d
	movl	%eax, (%rdi,%r9,4)
	jmp	LBB1_116
LBB1_107:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_109
## %bb.108:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_116
LBB1_109:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	cmpl	$0, (%rax,%rdx,4)
	jne	LBB1_111
## %bb.110:                             ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	$0, (%rax,%rdx,4)
	jmp	LBB1_116
LBB1_111:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-64(%rbp), %esi
	movl	%esi, %edx
	movq	%rax, -232(%rbp)        ## 8-byte Spill
	movl	%ecx, %eax
	xorl	%ecx, %ecx
	movq	%rdx, -240(%rbp)        ## 8-byte Spill
	movl	%ecx, %edx
	movq	-232(%rbp), %rdi        ## 8-byte Reload
	movq	-240(%rbp), %r8         ## 8-byte Reload
	divl	(%rdi,%r8,4)
	movl	-56(%rbp), %ecx
	movl	%ecx, %r9d
	movl	%eax, (%rdi,%r9,4)
	jmp	LBB1_116
LBB1_112:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_114
## %bb.113:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_116
LBB1_114:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-64(%rbp), %esi
	movl	%esi, %edx
	imull	(%rax,%rdx,4), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_116
LBB1_115:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	$7, %edi
	callq	_Limn2500Exception
LBB1_116:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_230
LBB1_117:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$41, -36(%rbp)
	jne	LBB1_169
## %bb.118:                             ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	(%rax), %ecx
	andl	$1, %ecx
	cmpl	$0, %ecx
	je	LBB1_120
## %bb.119:                             ##   in Loop: Header=BB1_13 Depth=1
	movl	$8, %edi
	callq	_Limn2500Exception
	jmp	LBB1_168
LBB1_120:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	-28(%rbp), %eax
	shrl	$28, %eax
	movl	%eax, -40(%rbp)
	movl	-28(%rbp), %eax
	shrl	$6, %eax
	andl	$31, %eax
	movl	%eax, -56(%rbp)
	movl	-28(%rbp), %eax
	shrl	$11, %eax
	andl	$31, %eax
	movl	%eax, -60(%rbp)
	movzwl	-26(%rbp), %eax
	andl	$15, %eax
	movl	%eax, -64(%rbp)
	movl	-40(%rbp), %eax
	movl	%eax, %ecx
	movq	%rcx, %rdx
	subq	$15, %rdx
	movq	%rcx, -248(%rbp)        ## 8-byte Spill
	ja	LBB1_166
## %bb.243:                             ##   in Loop: Header=BB1_13 Depth=1
	leaq	LJTI1_1(%rip), %rax
	movq	-248(%rbp), %rcx        ## 8-byte Reload
	movslq	(%rax,%rcx,4), %rdx
	addq	%rax, %rdx
	jmpq	*%rdx
LBB1_121:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	24(%rax), %ecx
	andl	$63, %ecx
	movl	%ecx, -84(%rbp)
	movl	44(%rax), %ecx
	movl	%ecx, -104(%rbp)
	movl	8(%rax), %ecx
	andl	$16, %ecx
	cmpl	$0, %ecx
	je	LBB1_123
## %bb.122:                             ##   in Loop: Header=BB1_13 Depth=1
	movl	-104(%rbp), %eax
	andl	$1048575, %eax          ## imm = 0xFFFFF
	movl	%eax, -104(%rbp)
LBB1_123:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_TLB@GOTPCREL(%rip), %rax
	movq	_ControlReg@GOTPCREL(%rip), %rcx
	movl	-104(%rbp), %edx
	movl	%edx, %esi
	shlq	$32, %rsi
	movl	8(%rcx), %edx
	movl	%edx, %ecx
	orq	%rcx, %rsi
	movl	-84(%rbp), %edx
	movl	%edx, %ecx
	movq	%rsi, (%rax,%rcx,8)
	movl	_TLBWriteCount(%rip), %edx
	addl	$1, %edx
	movl	%edx, _TLBWriteCount(%rip)
	jmp	LBB1_167
LBB1_124:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	$-2147483648, 24(%rax)  ## imm = 0x80000000
	movl	44(%rax), %ecx
	andl	$1048575, %ecx          ## imm = 0xFFFFF
	movl	%ecx, -80(%rbp)
	movl	-80(%rbp), %ecx
	andl	$7, %ecx
	movl	-80(%rbp), %edx
	shrl	$19, %edx
	shll	$3, %edx
	orl	%edx, %ecx
	movl	%ecx, -84(%rbp)
	movl	$0, -108(%rbp)
LBB1_125:                               ##   Parent Loop BB1_13 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	cmpl	$4, -108(%rbp)
	jge	LBB1_130
## %bb.126:                             ##   in Loop: Header=BB1_125 Depth=2
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movq	_TLB@GOTPCREL(%rip), %rcx
	movl	-84(%rbp), %edx
	shll	$2, %edx
	addl	-108(%rbp), %edx
	movl	%edx, %edx
	movl	%edx, %esi
	movq	(%rcx,%rsi,8), %rcx
	shrq	$32, %rcx
	movl	44(%rax), %edx
	movl	%edx, %eax
	cmpq	%rax, %rcx
	jne	LBB1_128
## %bb.127:                             ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-84(%rbp), %ecx
	shll	$2, %ecx
	addl	-108(%rbp), %ecx
	movl	%ecx, 24(%rax)
	jmp	LBB1_130
LBB1_128:                               ##   in Loop: Header=BB1_125 Depth=2
	jmp	LBB1_129
LBB1_129:                               ##   in Loop: Header=BB1_125 Depth=2
	movl	-108(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -108(%rbp)
	jmp	LBB1_125
LBB1_130:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_167
LBB1_131:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movq	_TLB@GOTPCREL(%rip), %rcx
	movl	24(%rax), %edx
	andl	$63, %edx
	movl	%edx, %edx
	movl	%edx, %esi
	movq	(%rcx,%rsi,8), %rcx
	movq	%rcx, -96(%rbp)
	movq	-96(%rbp), %rcx
                                        ## kill: def $ecx killed $ecx killed $rcx
	movl	%ecx, 8(%rax)
	movq	-96(%rbp), %rsi
	shrq	$32, %rsi
                                        ## kill: def $esi killed $esi killed $rsi
	movl	%esi, 44(%rax)
	jmp	LBB1_167
LBB1_132:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	8(%rax), %ecx
	movl	%ecx, -100(%rbp)
	movl	-100(%rbp), %ecx
	andl	$1, %ecx
	cmpl	$0, %ecx
	jne	LBB1_134
## %bb.133:                             ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	$0, 8(%rax)
	jmp	LBB1_167
LBB1_134:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movq	%rax, %rcx
	addq	$8, %rcx
	movl	-100(%rbp), %edx
	shrl	$5, %edx
	shll	$12, %edx
	movl	44(%rax), %esi
	andl	$1023, %esi             ## imm = 0x3FF
	shll	$2, %esi
	orl	%esi, %edx
	movl	%edx, %edi
	movq	%rcx, %rsi
	callq	_CPUReadLong
	jmp	LBB1_167
LBB1_135:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	-56(%rbp), %eax
	andl	$1, %eax
	cmpl	$0, %eax
	je	LBB1_141
## %bb.136:                             ##   in Loop: Header=BB1_13 Depth=1
	movl	$0, -112(%rbp)
LBB1_137:                               ##   Parent Loop BB1_13 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	cmpl	$2048, -112(%rbp)       ## imm = 0x800
	jge	LBB1_140
## %bb.138:                             ##   in Loop: Header=BB1_137 Depth=2
	movq	_ICacheTags@GOTPCREL(%rip), %rax
	movslq	-112(%rbp), %rcx
	movl	$0, (%rax,%rcx,4)
## %bb.139:                             ##   in Loop: Header=BB1_137 Depth=2
	movl	-112(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -112(%rbp)
	jmp	LBB1_137
LBB1_140:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_141
LBB1_141:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	-56(%rbp), %eax
	andl	$2, %eax
	cmpl	$0, %eax
	je	LBB1_149
## %bb.142:                             ##   in Loop: Header=BB1_13 Depth=1
	movl	$0, -116(%rbp)
LBB1_143:                               ##   Parent Loop BB1_13 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	cmpl	$2048, -116(%rbp)       ## imm = 0x800
	jge	LBB1_148
## %bb.144:                             ##   in Loop: Header=BB1_143 Depth=2
	movq	_DCacheTags@GOTPCREL(%rip), %rax
	movslq	-116(%rbp), %rcx
	movl	(%rax,%rcx,4), %edx
	andl	$1, %edx
	cmpl	$0, %edx
	je	LBB1_146
## %bb.145:                             ##   in Loop: Header=BB1_143 Depth=2
	movq	_DCache@GOTPCREL(%rip), %rax
	movq	_DCacheTags@GOTPCREL(%rip), %rcx
	movslq	-116(%rbp), %rdx
	movl	(%rcx,%rdx,4), %esi
	andl	$-32, %esi
	movl	-116(%rbp), %edi
	shll	$5, %edi
	movslq	%edi, %rcx
	addq	%rcx, %rax
	movl	%esi, %edi
	movq	%rax, %rsi
	movl	$32, %edx
	callq	_EBusWrite
	movq	_DCacheTags@GOTPCREL(%rip), %rcx
	movslq	-116(%rbp), %rsi
	movl	(%rcx,%rsi,4), %edx
	andl	$-2, %edx
	movl	%edx, (%rcx,%rsi,4)
LBB1_146:                               ##   in Loop: Header=BB1_143 Depth=2
	jmp	LBB1_147
LBB1_147:                               ##   in Loop: Header=BB1_143 Depth=2
	movl	-116(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -116(%rbp)
	jmp	LBB1_143
LBB1_148:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_149
LBB1_149:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	-56(%rbp), %eax
	andl	$4, %eax
	cmpl	$0, %eax
	je	LBB1_155
## %bb.150:                             ##   in Loop: Header=BB1_13 Depth=1
	movl	$0, -120(%rbp)
LBB1_151:                               ##   Parent Loop BB1_13 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	cmpl	$2048, -120(%rbp)       ## imm = 0x800
	jge	LBB1_154
## %bb.152:                             ##   in Loop: Header=BB1_151 Depth=2
	movq	_DCacheTags@GOTPCREL(%rip), %rax
	movslq	-120(%rbp), %rcx
	movl	$0, (%rax,%rcx,4)
## %bb.153:                             ##   in Loop: Header=BB1_151 Depth=2
	movl	-120(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -120(%rbp)
	jmp	LBB1_151
LBB1_154:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_155
LBB1_155:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_167
LBB1_156:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	$3, %edi
	callq	_Limn2500Exception
	jmp	LBB1_167
LBB1_157:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	$0, _CPULocked(%rip)
	testb	$1, _TLBMiss(%rip)
	je	LBB1_159
## %bb.158:                             ##   in Loop: Header=BB1_13 Depth=1
	movb	$0, _TLBMiss(%rip)
	movl	_TLBPC(%rip), %eax
	movl	%eax, _PC(%rip)
	jmp	LBB1_160
LBB1_159:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	12(%rax), %ecx
	movl	%ecx, _PC(%rip)
LBB1_160:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	(%rax), %ecx
	andl	$-268435456, %ecx       ## imm = 0xF0000000
	movl	(%rax), %edx
	shrl	$8, %edx
	andl	$65535, %edx            ## imm = 0xFFFF
	orl	%edx, %ecx
	movl	%ecx, (%rax)
	jmp	LBB1_167
LBB1_161:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	_CacheFillCount(%rip), %esi
	leaq	L_.str(%rip), %rdi
	movb	$0, %al
	callq	_printf
	movb	$1, _Halted(%rip)
	jmp	LBB1_167
LBB1_162:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movq	_Reg@GOTPCREL(%rip), %rcx
	movl	-60(%rbp), %edx
	movl	%edx, %esi
	movl	(%rcx,%rsi,4), %edx
	movl	-64(%rbp), %edi
	movl	%edi, %ecx
	movl	%edx, (%rax,%rcx,4)
	jmp	LBB1_167
LBB1_163:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_165
## %bb.164:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_167
LBB1_165:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movq	_ControlReg@GOTPCREL(%rip), %rcx
	movl	-64(%rbp), %edx
	movl	%edx, %esi
	movl	(%rcx,%rsi,4), %edx
	movl	-56(%rbp), %edi
	movl	%edi, %ecx
	movl	%edx, (%rax,%rcx,4)
	jmp	LBB1_167
LBB1_166:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	$7, %edi
	callq	_Limn2500Exception
LBB1_167:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_168
LBB1_168:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_229
LBB1_169:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	-28(%rbp), %eax
	shrl	$6, %eax
	andl	$31, %eax
	movl	%eax, -56(%rbp)
	movl	-28(%rbp), %eax
	shrl	$11, %eax
	andl	$31, %eax
	movl	%eax, -60(%rbp)
	movzwl	-26(%rbp), %eax
	movl	%eax, -68(%rbp)
	movl	-36(%rbp), %eax
	addl	$-4, %eax
	movl	%eax, %ecx
	subl	$57, %eax
	movq	%rcx, -256(%rbp)        ## 8-byte Spill
	ja	LBB1_227
## %bb.242:                             ##   in Loop: Header=BB1_13 Depth=1
	leaq	LJTI1_0(%rip), %rax
	movq	-256(%rbp), %rcx        ## 8-byte Reload
	movslq	(%rax,%rcx,4), %rdx
	addq	%rax, %rdx
	jmpq	*%rdx
LBB1_170:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	cmpl	$0, (%rax,%rdx,4)
	jne	LBB1_172
## %bb.171:                             ##   in Loop: Header=BB1_13 Depth=1
	movl	-24(%rbp), %eax
	movl	-28(%rbp), %ecx
	shrl	$11, %ecx
	shll	$2, %ecx
	shll	$9, %ecx
	sarl	$9, %ecx
	addl	%ecx, %eax
	movl	%eax, _PC(%rip)
LBB1_172:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_173:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	cmpl	$0, (%rax,%rdx,4)
	je	LBB1_175
## %bb.174:                             ##   in Loop: Header=BB1_13 Depth=1
	movl	-24(%rbp), %eax
	movl	-28(%rbp), %ecx
	shrl	$11, %ecx
	shll	$2, %ecx
	shll	$9, %ecx
	sarl	$9, %ecx
	addl	%ecx, %eax
	movl	%eax, _PC(%rip)
LBB1_175:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_176:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	cmpl	$0, (%rax,%rdx,4)
	jge	LBB1_178
## %bb.177:                             ##   in Loop: Header=BB1_13 Depth=1
	movl	-24(%rbp), %eax
	movl	-28(%rbp), %ecx
	shrl	$11, %ecx
	shll	$2, %ecx
	shll	$9, %ecx
	sarl	$9, %ecx
	addl	%ecx, %eax
	movl	%eax, _PC(%rip)
LBB1_178:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_179:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_181
## %bb.180:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_181:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	addl	-68(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_228
LBB1_182:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_184
## %bb.183:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_184:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	subl	-68(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_228
LBB1_185:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_187
## %bb.186:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_187:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	cmpl	-68(%rbp), %ecx
	jae	LBB1_189
## %bb.188:                             ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	$1, (%rax,%rdx,4)
	jmp	LBB1_190
LBB1_189:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	$0, (%rax,%rdx,4)
LBB1_190:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_191:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_193
## %bb.192:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_193:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-68(%rbp), %esi
	shll	$16, %esi
	sarl	$16, %esi
	cmpl	%esi, %ecx
	jge	LBB1_195
## %bb.194:                             ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	$1, (%rax,%rdx,4)
	jmp	LBB1_196
LBB1_195:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	$0, (%rax,%rdx,4)
LBB1_196:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_197:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_199
## %bb.198:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_199:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	andl	-68(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_228
LBB1_200:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_202
## %bb.201:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_202:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	xorl	-68(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_228
LBB1_203:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_205
## %bb.204:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_205:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	orl	-68(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_228
LBB1_206:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_208
## %bb.207:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_208:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-68(%rbp), %esi
	shll	$16, %esi
	orl	%esi, %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	movl	%ecx, (%rax,%rdx,4)
	jmp	LBB1_228
LBB1_209:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_211
## %bb.210:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_211:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	addl	-68(%rbp), %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	shlq	$2, %rdx
	addq	%rdx, %rax
	movl	%ecx, %edi
	movq	%rax, %rsi
	callq	_CPUReadByte
	jmp	LBB1_228
LBB1_212:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_214
## %bb.213:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_214:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-68(%rbp), %esi
	shll	$1, %esi
	addl	%esi, %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	shlq	$2, %rdx
	addq	%rdx, %rax
	movl	%ecx, %edi
	movq	%rax, %rsi
	callq	_CPUReadInt
	jmp	LBB1_228
LBB1_215:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	jne	LBB1_217
## %bb.216:                             ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_228
LBB1_217:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-68(%rbp), %esi
	shll	$2, %esi
	addl	%esi, %ecx
	movl	-56(%rbp), %esi
	movl	%esi, %edx
	shlq	$2, %rdx
	addq	%rdx, %rax
	movl	%ecx, %edi
	movq	%rax, %rsi
	callq	_CPUReadLong
	jmp	LBB1_228
LBB1_218:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	addl	-68(%rbp), %ecx
	movl	-60(%rbp), %esi
	movl	%esi, %edx
	movl	(%rax,%rdx,4), %esi
	movl	%ecx, %edi
	callq	_CPUWriteByte
	jmp	LBB1_228
LBB1_219:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-68(%rbp), %esi
	shll	$1, %esi
	addl	%esi, %ecx
	movl	-60(%rbp), %esi
	movl	%esi, %edx
	movl	(%rax,%rdx,4), %esi
	movl	%ecx, %edi
	callq	_CPUWriteInt
	jmp	LBB1_228
LBB1_220:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-68(%rbp), %esi
	shll	$2, %esi
	addl	%esi, %ecx
	movl	-60(%rbp), %esi
	movl	%esi, %edx
	movl	(%rax,%rdx,4), %esi
	movl	%ecx, %edi
	callq	_CPUWriteLong
	jmp	LBB1_228
LBB1_221:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	addl	-68(%rbp), %ecx
	movl	-60(%rbp), %esi
	shll	$27, %esi
	sarl	$27, %esi
	movl	%ecx, %edi
	callq	_CPUWriteByte
	jmp	LBB1_228
LBB1_222:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-68(%rbp), %esi
	shll	$1, %esi
	addl	%esi, %ecx
	movl	-60(%rbp), %esi
	shll	$27, %esi
	sarl	$27, %esi
	movl	%ecx, %edi
	callq	_CPUWriteInt
	jmp	LBB1_228
LBB1_223:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-56(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-68(%rbp), %esi
	shll	$2, %esi
	addl	%esi, %ecx
	movl	-60(%rbp), %esi
	shll	$27, %esi
	sarl	$27, %esi
	movl	%ecx, %edi
	callq	_CPUWriteLong
	jmp	LBB1_228
LBB1_224:                               ##   in Loop: Header=BB1_13 Depth=1
	cmpl	$0, -56(%rbp)
	je	LBB1_226
## %bb.225:                             ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	_PC(%rip), %ecx
	movl	-56(%rbp), %edx
	movl	%edx, %esi
	movl	%ecx, (%rax,%rsi,4)
LBB1_226:                               ##   in Loop: Header=BB1_13 Depth=1
	movq	_Reg@GOTPCREL(%rip), %rax
	movl	-60(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	movl	-68(%rbp), %esi
	shll	$2, %esi
	shll	$14, %esi
	sarl	$14, %esi
	addl	%esi, %ecx
	movl	%ecx, _PC(%rip)
	jmp	LBB1_228
LBB1_227:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	$7, %edi
	callq	_Limn2500Exception
LBB1_228:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_229
LBB1_229:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_230
LBB1_230:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_231
LBB1_231:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_232
LBB1_232:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_233
LBB1_233:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_234
LBB1_234:                               ##   in Loop: Header=BB1_13 Depth=1
	testb	$1, _Halted(%rip)
	jne	LBB1_236
## %bb.235:                             ##   in Loop: Header=BB1_13 Depth=1
	testb	$1, _Running(%rip)
	jne	LBB1_237
LBB1_236:
	movl	-12(%rbp), %eax
	movl	%eax, -4(%rbp)
	jmp	LBB1_240
LBB1_237:                               ##   in Loop: Header=BB1_13 Depth=1
	jmp	LBB1_238
LBB1_238:                               ##   in Loop: Header=BB1_13 Depth=1
	movl	-12(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -12(%rbp)
	jmp	LBB1_13
LBB1_239:
	movl	-12(%rbp), %eax
	movl	%eax, -4(%rbp)
LBB1_240:
	movl	-4(%rbp), %eax
	addq	$256, %rsp              ## imm = 0x100
	popq	%rbp
	retq
	.cfi_endproc
	.p2align	2, 0x90
	.data_region jt32
.set L1_0_set_206, LBB1_206-LJTI1_0
.set L1_0_set_227, LBB1_227-LJTI1_0
.set L1_0_set_223, LBB1_223-LJTI1_0
.set L1_0_set_203, LBB1_203-LJTI1_0
.set L1_0_set_222, LBB1_222-LJTI1_0
.set L1_0_set_200, LBB1_200-LJTI1_0
.set L1_0_set_221, LBB1_221-LJTI1_0
.set L1_0_set_197, LBB1_197-LJTI1_0
.set L1_0_set_191, LBB1_191-LJTI1_0
.set L1_0_set_220, LBB1_220-LJTI1_0
.set L1_0_set_215, LBB1_215-LJTI1_0
.set L1_0_set_185, LBB1_185-LJTI1_0
.set L1_0_set_176, LBB1_176-LJTI1_0
.set L1_0_set_219, LBB1_219-LJTI1_0
.set L1_0_set_212, LBB1_212-LJTI1_0
.set L1_0_set_182, LBB1_182-LJTI1_0
.set L1_0_set_173, LBB1_173-LJTI1_0
.set L1_0_set_224, LBB1_224-LJTI1_0
.set L1_0_set_218, LBB1_218-LJTI1_0
.set L1_0_set_209, LBB1_209-LJTI1_0
.set L1_0_set_179, LBB1_179-LJTI1_0
.set L1_0_set_170, LBB1_170-LJTI1_0
LJTI1_0:
	.long	L1_0_set_206
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_223
	.long	L1_0_set_227
	.long	L1_0_set_203
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_222
	.long	L1_0_set_227
	.long	L1_0_set_200
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_221
	.long	L1_0_set_227
	.long	L1_0_set_197
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_191
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_220
	.long	L1_0_set_215
	.long	L1_0_set_185
	.long	L1_0_set_176
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_219
	.long	L1_0_set_212
	.long	L1_0_set_182
	.long	L1_0_set_173
	.long	L1_0_set_227
	.long	L1_0_set_227
	.long	L1_0_set_224
	.long	L1_0_set_227
	.long	L1_0_set_218
	.long	L1_0_set_209
	.long	L1_0_set_179
	.long	L1_0_set_170
.set L1_1_set_121, LBB1_121-LJTI1_1
.set L1_1_set_124, LBB1_124-LJTI1_1
.set L1_1_set_131, LBB1_131-LJTI1_1
.set L1_1_set_132, LBB1_132-LJTI1_1
.set L1_1_set_166, LBB1_166-LJTI1_1
.set L1_1_set_135, LBB1_135-LJTI1_1
.set L1_1_set_156, LBB1_156-LJTI1_1
.set L1_1_set_157, LBB1_157-LJTI1_1
.set L1_1_set_161, LBB1_161-LJTI1_1
.set L1_1_set_162, LBB1_162-LJTI1_1
.set L1_1_set_163, LBB1_163-LJTI1_1
LJTI1_1:
	.long	L1_1_set_121
	.long	L1_1_set_124
	.long	L1_1_set_131
	.long	L1_1_set_132
	.long	L1_1_set_166
	.long	L1_1_set_166
	.long	L1_1_set_166
	.long	L1_1_set_166
	.long	L1_1_set_135
	.long	L1_1_set_166
	.long	L1_1_set_156
	.long	L1_1_set_157
	.long	L1_1_set_161
	.long	L1_1_set_166
	.long	L1_1_set_162
	.long	L1_1_set_163
.set L1_2_set_87, LBB1_87-LJTI1_2
.set L1_2_set_88, LBB1_88-LJTI1_2
.set L1_2_set_115, LBB1_115-LJTI1_2
.set L1_2_set_89, LBB1_89-LJTI1_2
.set L1_2_set_94, LBB1_94-LJTI1_2
.set L1_2_set_97, LBB1_97-LJTI1_2
.set L1_2_set_102, LBB1_102-LJTI1_2
.set L1_2_set_107, LBB1_107-LJTI1_2
.set L1_2_set_112, LBB1_112-LJTI1_2
LJTI1_2:
	.long	L1_2_set_87
	.long	L1_2_set_88
	.long	L1_2_set_115
	.long	L1_2_set_115
	.long	L1_2_set_115
	.long	L1_2_set_115
	.long	L1_2_set_115
	.long	L1_2_set_115
	.long	L1_2_set_89
	.long	L1_2_set_94
	.long	L1_2_set_115
	.long	L1_2_set_97
	.long	L1_2_set_102
	.long	L1_2_set_107
	.long	L1_2_set_115
	.long	L1_2_set_112
.set L1_3_set_45, LBB1_45-LJTI1_3
.set L1_3_set_46, LBB1_46-LJTI1_3
.set L1_3_set_47, LBB1_47-LJTI1_3
.set L1_3_set_48, LBB1_48-LJTI1_3
LJTI1_3:
	.long	L1_3_set_45
	.long	L1_3_set_46
	.long	L1_3_set_47
	.long	L1_3_set_48
.set L1_4_set_55, LBB1_55-LJTI1_4
.set L1_4_set_56, LBB1_56-LJTI1_4
.set L1_4_set_57, LBB1_57-LJTI1_4
.set L1_4_set_58, LBB1_58-LJTI1_4
.set L1_4_set_59, LBB1_59-LJTI1_4
.set L1_4_set_63, LBB1_63-LJTI1_4
.set L1_4_set_67, LBB1_67-LJTI1_4
.set L1_4_set_68, LBB1_68-LJTI1_4
.set L1_4_set_69, LBB1_69-LJTI1_4
.set L1_4_set_75, LBB1_75-LJTI1_4
.set L1_4_set_76, LBB1_76-LJTI1_4
.set L1_4_set_77, LBB1_77-LJTI1_4
.set L1_4_set_78, LBB1_78-LJTI1_4
.set L1_4_set_79, LBB1_79-LJTI1_4
.set L1_4_set_80, LBB1_80-LJTI1_4
.set L1_4_set_81, LBB1_81-LJTI1_4
LJTI1_4:
	.long	L1_4_set_55
	.long	L1_4_set_56
	.long	L1_4_set_57
	.long	L1_4_set_58
	.long	L1_4_set_59
	.long	L1_4_set_63
	.long	L1_4_set_67
	.long	L1_4_set_68
	.long	L1_4_set_69
	.long	L1_4_set_75
	.long	L1_4_set_76
	.long	L1_4_set_77
	.long	L1_4_set_78
	.long	L1_4_set_79
	.long	L1_4_set_80
	.long	L1_4_set_81
.set L1_5_set_70, LBB1_70-LJTI1_5
.set L1_5_set_71, LBB1_71-LJTI1_5
.set L1_5_set_72, LBB1_72-LJTI1_5
.set L1_5_set_73, LBB1_73-LJTI1_5
LJTI1_5:
	.long	L1_5_set_70
	.long	L1_5_set_71
	.long	L1_5_set_72
	.long	L1_5_set_73
	.end_data_region
                                        ## -- End function
	.p2align	4, 0x90         ## -- Begin function Limn2500Exception
_Limn2500Exception:                     ## @Limn2500Exception
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movq	_CurrentException@GOTPCREL(%rip), %rax
	movl	%edi, -4(%rbp)
	cmpl	$0, (%rax)
	je	LBB2_2
## %bb.1:
	movq	___stderrp@GOTPCREL(%rip), %rax
	movq	(%rax), %rdi
	leaq	L_.str.1(%rip), %rsi
	xorl	%ecx, %ecx
                                        ## kill: def $cl killed $cl killed $ecx
	movb	%cl, %al
	callq	_fprintf
	movl	%eax, -8(%rbp)          ## 4-byte Spill
	callq	_abort
LBB2_2:
	movq	_CurrentException@GOTPCREL(%rip), %rax
	movl	-4(%rbp), %ecx
	movl	%ecx, (%rax)
	addq	$16, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4, 0x90         ## -- Begin function CPUReadLong
_CPUReadLong:                           ## @CPUReadLong
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movl	%edi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	-8(%rbp), %eax
	andl	$3, %eax
	cmpl	$0, %eax
	je	LBB3_2
## %bb.1:
	movl	$9, %edi
	callq	_Limn2500Exception
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movb	$0, -1(%rbp)
	jmp	LBB3_14
LBB3_2:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	(%rax), %ecx
	andl	$4, %ecx
	cmpl	$0, %ecx
	je	LBB3_6
## %bb.3:
	xorl	%edx, %edx
	movl	-8(%rbp), %edi
	leaq	-8(%rbp), %rsi
	callq	_CPUTranslate
	testb	$1, %al
	jne	LBB3_5
## %bb.4:
	movb	$0, -1(%rbp)
	jmp	LBB3_14
LBB3_5:
	jmp	LBB3_6
LBB3_6:
	movl	-8(%rbp), %eax
	andl	$-1073741824, %eax      ## imm = 0xC0000000
	cmpl	$0, %eax
	je	LBB3_10
## %bb.7:
	movl	-8(%rbp), %edi
	movq	-16(%rbp), %rax
	movq	%rax, %rsi
	movl	$4, %edx
	callq	_EBusRead
	cmpl	$1, %eax
	jne	LBB3_9
## %bb.8:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movl	$4, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB3_14
LBB3_9:
	jmp	LBB3_13
LBB3_10:
	xorl	%edx, %edx
	movl	-8(%rbp), %edi
	leaq	-24(%rbp), %rsi
	callq	_CPUCacheLine
	testb	$1, %al
	jne	LBB3_12
## %bb.11:
	movb	$0, -1(%rbp)
	jmp	LBB3_14
LBB3_12:
	movq	-24(%rbp), %rax
	movl	(%rax), %ecx
	movq	-16(%rbp), %rax
	movl	%ecx, (%rax)
LBB3_13:
	movb	$1, -1(%rbp)
LBB3_14:
	movb	-1(%rbp), %al
	andb	$1, %al
	movzbl	%al, %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4, 0x90         ## -- Begin function RoR
_RoR:                                   ## @RoR
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %eax
	movl	-8(%rbp), %ecx
                                        ## kill: def $cl killed $ecx
	shrl	%cl, %eax
	andl	$31, %eax
	movl	-4(%rbp), %edx
	movl	$32, %esi
	subl	-8(%rbp), %esi
	movl	%esi, %ecx
                                        ## kill: def $cl killed $ecx
	shll	%cl, %edx
	andl	$31, %edx
	orl	%edx, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4, 0x90         ## -- Begin function CPUWriteLong
_CPUWriteLong:                          ## @CPUWriteLong
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movl	%edi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	-8(%rbp), %eax
	andl	$3, %eax
	cmpl	$0, %eax
	je	LBB5_2
## %bb.1:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movl	$9, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB5_14
LBB5_2:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	(%rax), %ecx
	andl	$4, %ecx
	cmpl	$0, %ecx
	je	LBB5_6
## %bb.3:
	movl	-8(%rbp), %edi
	leaq	-8(%rbp), %rsi
	movl	$1, %edx
	callq	_CPUTranslate
	testb	$1, %al
	jne	LBB5_5
## %bb.4:
	movb	$0, -1(%rbp)
	jmp	LBB5_14
LBB5_5:
	jmp	LBB5_6
LBB5_6:
	movl	-8(%rbp), %eax
	andl	$-1073741824, %eax      ## imm = 0xC0000000
	cmpl	$0, %eax
	je	LBB5_10
## %bb.7:
	movl	-8(%rbp), %edi
	leaq	-12(%rbp), %rax
	movq	%rax, %rsi
	movl	$4, %edx
	callq	_EBusWrite
	cmpl	$1, %eax
	jne	LBB5_9
## %bb.8:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movl	$4, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB5_14
LBB5_9:
	jmp	LBB5_13
LBB5_10:
	movl	-8(%rbp), %edi
	leaq	-24(%rbp), %rsi
	movl	$1, %edx
	callq	_CPUCacheLine
	testb	$1, %al
	jne	LBB5_12
## %bb.11:
	movb	$0, -1(%rbp)
	jmp	LBB5_14
LBB5_12:
	movl	-12(%rbp), %eax
	movq	-24(%rbp), %rcx
	movl	%eax, (%rcx)
LBB5_13:
	movb	$1, -1(%rbp)
LBB5_14:
	movb	-1(%rbp), %al
	andb	$1, %al
	movzbl	%al, %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4, 0x90         ## -- Begin function CPUWriteInt
_CPUWriteInt:                           ## @CPUWriteInt
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movl	%edi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	-8(%rbp), %eax
	andl	$1, %eax
	cmpl	$0, %eax
	je	LBB6_2
## %bb.1:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movl	$9, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB6_14
LBB6_2:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	(%rax), %ecx
	andl	$4, %ecx
	cmpl	$0, %ecx
	je	LBB6_6
## %bb.3:
	movl	-8(%rbp), %edi
	leaq	-8(%rbp), %rsi
	movl	$1, %edx
	callq	_CPUTranslate
	testb	$1, %al
	jne	LBB6_5
## %bb.4:
	movb	$0, -1(%rbp)
	jmp	LBB6_14
LBB6_5:
	jmp	LBB6_6
LBB6_6:
	movl	-8(%rbp), %eax
	andl	$-1073741824, %eax      ## imm = 0xC0000000
	cmpl	$0, %eax
	je	LBB6_10
## %bb.7:
	movl	-8(%rbp), %edi
	leaq	-12(%rbp), %rax
	movq	%rax, %rsi
	movl	$2, %edx
	callq	_EBusWrite
	cmpl	$1, %eax
	jne	LBB6_9
## %bb.8:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movl	$4, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB6_14
LBB6_9:
	jmp	LBB6_13
LBB6_10:
	movl	-8(%rbp), %edi
	leaq	-24(%rbp), %rsi
	movl	$1, %edx
	callq	_CPUCacheLine
	testb	$1, %al
	jne	LBB6_12
## %bb.11:
	movb	$0, -1(%rbp)
	jmp	LBB6_14
LBB6_12:
	movl	-12(%rbp), %eax
                                        ## kill: def $ax killed $ax killed $eax
	movq	-24(%rbp), %rcx
	movw	%ax, (%rcx)
LBB6_13:
	movb	$1, -1(%rbp)
LBB6_14:
	movb	-1(%rbp), %al
	andb	$1, %al
	movzbl	%al, %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4, 0x90         ## -- Begin function CPUWriteByte
_CPUWriteByte:                          ## @CPUWriteByte
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	%edi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	(%rax), %ecx
	andl	$4, %ecx
	cmpl	$0, %ecx
	je	LBB7_4
## %bb.1:
	movl	-8(%rbp), %edi
	leaq	-8(%rbp), %rsi
	movl	$1, %edx
	callq	_CPUTranslate
	testb	$1, %al
	jne	LBB7_3
## %bb.2:
	movb	$0, -1(%rbp)
	jmp	LBB7_12
LBB7_3:
	jmp	LBB7_4
LBB7_4:
	movl	-8(%rbp), %eax
	andl	$-1073741824, %eax      ## imm = 0xC0000000
	cmpl	$0, %eax
	je	LBB7_8
## %bb.5:
	movl	-8(%rbp), %edi
	leaq	-12(%rbp), %rax
	movq	%rax, %rsi
	movl	$1, %edx
	callq	_EBusWrite
	cmpl	$1, %eax
	jne	LBB7_7
## %bb.6:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movl	$4, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB7_12
LBB7_7:
	jmp	LBB7_11
LBB7_8:
	movl	-8(%rbp), %edi
	leaq	-24(%rbp), %rsi
	movl	$1, %edx
	callq	_CPUCacheLine
	testb	$1, %al
	jne	LBB7_10
## %bb.9:
	movb	$0, -1(%rbp)
	jmp	LBB7_12
LBB7_10:
	movl	-12(%rbp), %eax
                                        ## kill: def $al killed $al killed $eax
	movq	-24(%rbp), %rcx
	movb	%al, (%rcx)
LBB7_11:
	movb	$1, -1(%rbp)
LBB7_12:
	movb	-1(%rbp), %al
	andb	$1, %al
	movzbl	%al, %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4, 0x90         ## -- Begin function CPUReadInt
_CPUReadInt:                            ## @CPUReadInt
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movl	%edi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	-8(%rbp), %eax
	andl	$1, %eax
	cmpl	$0, %eax
	je	LBB8_2
## %bb.1:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movl	$9, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB8_14
LBB8_2:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	(%rax), %ecx
	andl	$4, %ecx
	cmpl	$0, %ecx
	je	LBB8_6
## %bb.3:
	xorl	%edx, %edx
	movl	-8(%rbp), %edi
	leaq	-8(%rbp), %rsi
	callq	_CPUTranslate
	testb	$1, %al
	jne	LBB8_5
## %bb.4:
	movb	$0, -1(%rbp)
	jmp	LBB8_14
LBB8_5:
	jmp	LBB8_6
LBB8_6:
	movl	-8(%rbp), %eax
	andl	$-1073741824, %eax      ## imm = 0xC0000000
	cmpl	$0, %eax
	je	LBB8_10
## %bb.7:
	movl	-8(%rbp), %edi
	movq	-16(%rbp), %rax
	movq	%rax, %rsi
	movl	$2, %edx
	callq	_EBusRead
	cmpl	$1, %eax
	jne	LBB8_9
## %bb.8:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movl	$4, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB8_14
LBB8_9:
	movq	-16(%rbp), %rax
	movl	(%rax), %ecx
	andl	$65535, %ecx            ## imm = 0xFFFF
	movl	%ecx, (%rax)
	jmp	LBB8_13
LBB8_10:
	xorl	%edx, %edx
	movl	-8(%rbp), %edi
	leaq	-24(%rbp), %rsi
	callq	_CPUCacheLine
	testb	$1, %al
	jne	LBB8_12
## %bb.11:
	movb	$0, -1(%rbp)
	jmp	LBB8_14
LBB8_12:
	movq	-24(%rbp), %rax
	movzwl	(%rax), %ecx
	movq	-16(%rbp), %rax
	movl	%ecx, (%rax)
LBB8_13:
	movb	$1, -1(%rbp)
LBB8_14:
	movb	-1(%rbp), %al
	andb	$1, %al
	movzbl	%al, %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4, 0x90         ## -- Begin function CPUReadByte
_CPUReadByte:                           ## @CPUReadByte
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	%edi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	(%rax), %ecx
	andl	$4, %ecx
	cmpl	$0, %ecx
	je	LBB9_4
## %bb.1:
	xorl	%edx, %edx
	movl	-8(%rbp), %edi
	leaq	-8(%rbp), %rsi
	callq	_CPUTranslate
	testb	$1, %al
	jne	LBB9_3
## %bb.2:
	movb	$0, -1(%rbp)
	jmp	LBB9_12
LBB9_3:
	jmp	LBB9_4
LBB9_4:
	movl	-8(%rbp), %eax
	andl	$-1073741824, %eax      ## imm = 0xC0000000
	cmpl	$0, %eax
	je	LBB9_8
## %bb.5:
	movl	-8(%rbp), %edi
	movq	-16(%rbp), %rax
	movq	%rax, %rsi
	movl	$1, %edx
	callq	_EBusRead
	cmpl	$1, %eax
	jne	LBB9_7
## %bb.6:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movl	$4, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB9_12
LBB9_7:
	movq	-16(%rbp), %rax
	movl	(%rax), %ecx
	andl	$255, %ecx
	movl	%ecx, (%rax)
	jmp	LBB9_11
LBB9_8:
	xorl	%edx, %edx
	movl	-8(%rbp), %edi
	leaq	-24(%rbp), %rsi
	callq	_CPUCacheLine
	testb	$1, %al
	jne	LBB9_10
## %bb.9:
	movb	$0, -1(%rbp)
	jmp	LBB9_12
LBB9_10:
	movq	-24(%rbp), %rax
	movzbl	(%rax), %ecx
	movq	-16(%rbp), %rax
	movl	%ecx, (%rax)
LBB9_11:
	movb	$1, -1(%rbp)
LBB9_12:
	movb	-1(%rbp), %al
	andb	$1, %al
	movzbl	%al, %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4, 0x90         ## -- Begin function EBusWrite
_EBusWrite:                             ## @EBusWrite
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movq	_EBusBranches@GOTPCREL(%rip), %rax
	movl	%edi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movl	-8(%rbp), %ecx
	shrl	$27, %ecx
	movl	%ecx, -24(%rbp)
	movslq	-24(%rbp), %rsi
	shlq	$5, %rsi
	addq	%rsi, %rax
	cmpl	$0, (%rax)
	je	LBB10_2
## %bb.1:
	movq	_EBusBranches@GOTPCREL(%rip), %rax
	movslq	-24(%rbp), %rcx
	shlq	$5, %rcx
	addq	%rcx, %rax
	movq	8(%rax), %rax
	movl	-8(%rbp), %edx
	andl	$134217727, %edx        ## imm = 0x7FFFFFF
	movq	-16(%rbp), %rsi
	movl	-20(%rbp), %edi
	movl	%edi, -28(%rbp)         ## 4-byte Spill
	movl	%edx, %edi
	movl	-28(%rbp), %edx         ## 4-byte Reload
	callq	*%rax
	movl	%eax, -4(%rbp)
	jmp	LBB10_6
LBB10_2:
	cmpl	$24, -24(%rbp)
	jl	LBB10_4
## %bb.3:
	movl	$0, -4(%rbp)
	jmp	LBB10_6
LBB10_4:
	jmp	LBB10_5
LBB10_5:
	movl	$1, -4(%rbp)
LBB10_6:
	movl	-4(%rbp), %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4, 0x90         ## -- Begin function CPUTranslate
_CPUTranslate:                          ## @CPUTranslate
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$64, %rsp
                                        ## kill: def $dl killed $dl killed $edx
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	%edi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	andb	$1, %dl
	movb	%dl, -17(%rbp)
	movl	-8(%rbp), %ecx
	shrl	$12, %ecx
	movl	%ecx, -24(%rbp)
	movl	-8(%rbp), %ecx
	andl	$4095, %ecx             ## imm = 0xFFF
	movl	%ecx, -28(%rbp)
	movl	44(%rax), %ecx
	andl	$-1048576, %ecx         ## imm = 0xFFF00000
	orl	-24(%rbp), %ecx
	movl	%ecx, -32(%rbp)
	movl	-24(%rbp), %ecx
	andl	$7, %ecx
	movl	-24(%rbp), %edi
	shrl	$19, %edi
	shll	$3, %edi
	orl	%edi, %ecx
	movl	%ecx, -36(%rbp)
	movl	$-1, -56(%rbp)
	movl	$0, -60(%rbp)
LBB11_1:                                ## =>This Inner Loop Header: Depth=1
	cmpl	$4, -60(%rbp)
	jge	LBB11_8
## %bb.2:                               ##   in Loop: Header=BB11_1 Depth=1
	movq	_TLB@GOTPCREL(%rip), %rax
	movl	-36(%rbp), %ecx
	shll	$2, %ecx
	addl	-60(%rbp), %ecx
	movl	%ecx, %ecx
	movl	%ecx, %edx
	movq	(%rax,%rdx,8), %rax
	movq	%rax, -48(%rbp)
	movq	-48(%rbp), %rax
	andq	$16, %rax
	cmpq	$0, %rax
	movl	$1048575, %ecx          ## imm = 0xFFFFF
	movl	$4294967295, %esi       ## imm = 0xFFFFFFFF
	cmovnel	%ecx, %esi
	movl	%esi, -52(%rbp)
	movq	-48(%rbp), %rax
	shrq	$32, %rax
	movl	-32(%rbp), %ecx
	andl	-52(%rbp), %ecx
	movl	%ecx, %ecx
	movl	%ecx, %edx
	cmpq	%rdx, %rax
	sete	%dil
	andb	$1, %dil
	movb	%dil, -37(%rbp)
	testb	$1, -37(%rbp)
	je	LBB11_4
## %bb.3:
	jmp	LBB11_8
LBB11_4:                                ##   in Loop: Header=BB11_1 Depth=1
	movq	-48(%rbp), %rax
	andq	$1, %rax
	cmpq	$0, %rax
	jne	LBB11_6
## %bb.5:                               ##   in Loop: Header=BB11_1 Depth=1
	movl	-60(%rbp), %eax
	movl	%eax, -56(%rbp)
LBB11_6:                                ##   in Loop: Header=BB11_1 Depth=1
	jmp	LBB11_7
LBB11_7:                                ##   in Loop: Header=BB11_1 Depth=1
	movl	-60(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -60(%rbp)
	jmp	LBB11_1
LBB11_8:
	testb	$1, -37(%rbp)
	jne	LBB11_13
## %bb.9:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-32(%rbp), %ecx
	movl	%ecx, 44(%rax)
	movl	20(%rax), %ecx
	andl	$-4096, %ecx            ## imm = 0xF000
	movl	-24(%rbp), %edx
	shrl	$10, %edx
	shll	$2, %edx
	orl	%edx, %ecx
	movl	%ecx, 20(%rax)
	cmpl	$-1, -56(%rbp)
	jne	LBB11_11
## %bb.10:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-36(%rbp), %ecx
	shll	$2, %ecx
	movl	_TLBWriteCount(%rip), %edx
	andl	$3, %edx
	addl	%edx, %ecx
	movl	%ecx, 24(%rax)
	jmp	LBB11_12
LBB11_11:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-36(%rbp), %ecx
	shll	$2, %ecx
	addl	-56(%rbp), %ecx
	movl	%ecx, 24(%rax)
LBB11_12:
	movl	$15, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB11_22
LBB11_13:
	movq	-48(%rbp), %rax
	andq	$1, %rax
	cmpq	$0, %rax
	jne	LBB11_15
## %bb.14:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movb	-17(%rbp), %dl
	testb	$1, %dl
	movl	$13, %ecx
	movl	$12, %esi
	cmovnel	%ecx, %esi
	movl	%esi, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB11_22
LBB11_15:
	movq	-48(%rbp), %rax
	shrq	$5, %rax
	andq	$1048575, %rax          ## imm = 0xFFFFF
	shlq	$12, %rax
                                        ## kill: def $eax killed $eax killed $rax
	movl	%eax, -64(%rbp)
	movq	-48(%rbp), %rcx
	andq	$4, %rcx
	cmpq	$4, %rcx
	jne	LBB11_18
## %bb.16:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	(%rax), %ecx
	andl	$1, %ecx
	cmpl	$0, %ecx
	je	LBB11_18
## %bb.17:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movb	-17(%rbp), %dl
	testb	$1, %dl
	movl	$13, %ecx
	movl	$12, %esi
	cmovnel	%ecx, %esi
	movl	%esi, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB11_22
LBB11_18:
	testb	$1, -17(%rbp)
	je	LBB11_21
## %bb.19:
	movq	-48(%rbp), %rax
	andq	$2, %rax
	cmpq	$0, %rax
	jne	LBB11_21
## %bb.20:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movl	$13, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB11_22
LBB11_21:
	movl	-64(%rbp), %eax
	addl	-28(%rbp), %eax
	movq	-16(%rbp), %rcx
	movl	%eax, (%rcx)
	movb	$1, -1(%rbp)
LBB11_22:
	movb	-1(%rbp), %al
	andb	$1, %al
	movzbl	%al, %eax
	addq	$64, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4, 0x90         ## -- Begin function EBusRead
_EBusRead:                              ## @EBusRead
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movq	_EBusBranches@GOTPCREL(%rip), %rax
	movl	%edi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movl	-8(%rbp), %ecx
	shrl	$27, %ecx
	movl	%ecx, -24(%rbp)
	movslq	-24(%rbp), %rsi
	shlq	$5, %rsi
	addq	%rsi, %rax
	cmpl	$0, (%rax)
	je	LBB12_2
## %bb.1:
	movq	_EBusBranches@GOTPCREL(%rip), %rax
	movslq	-24(%rbp), %rcx
	shlq	$5, %rcx
	addq	%rcx, %rax
	movq	16(%rax), %rax
	movl	-8(%rbp), %edx
	andl	$134217727, %edx        ## imm = 0x7FFFFFF
	movq	-16(%rbp), %rsi
	movl	-20(%rbp), %edi
	movl	%edi, -28(%rbp)         ## 4-byte Spill
	movl	%edx, %edi
	movl	-28(%rbp), %edx         ## 4-byte Reload
	callq	*%rax
	movl	%eax, -4(%rbp)
	jmp	LBB12_6
LBB12_2:
	cmpl	$24, -24(%rbp)
	jl	LBB12_4
## %bb.3:
	movq	-16(%rbp), %rax
	movl	$0, (%rax)
	movl	$0, -4(%rbp)
	jmp	LBB12_6
LBB12_4:
	jmp	LBB12_5
LBB12_5:
	movl	$1, -4(%rbp)
LBB12_6:
	movl	-4(%rbp), %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4, 0x90         ## -- Begin function CPUCacheLine
_CPUCacheLine:                          ## @CPUCacheLine
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$80, %rsp
                                        ## kill: def $dl killed $dl killed $edx
	movq	_DCache@GOTPCREL(%rip), %rax
	movq	_ICache@GOTPCREL(%rip), %rcx
	movq	_DCacheTags@GOTPCREL(%rip), %r8
	movq	_ICacheTags@GOTPCREL(%rip), %r9
	movl	%edi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	andb	$1, %dl
	movb	%dl, -17(%rbp)
	movl	-8(%rbp), %edi
	andl	$-32, %edi
	movl	%edi, -24(%rbp)
	movl	-8(%rbp), %edi
	andl	$31, %edi
	movl	%edi, -28(%rbp)
	movl	-8(%rbp), %edi
	shrl	$5, %edi
	movl	%edi, -32(%rbp)
	movl	-32(%rbp), %edi
	shll	$1, %edi
	andl	$1023, %edi             ## imm = 0x3FF
	movl	%edi, -36(%rbp)
	movl	$-1, -40(%rbp)
	movb	_IFetch(%rip), %dl
	testb	$1, %dl
	cmovneq	%r9, %r8
	movq	%r8, -48(%rbp)
	movb	_IFetch(%rip), %dl
	testb	$1, %dl
	cmovneq	%rcx, %rax
	movq	%rax, -56(%rbp)
	movl	$0, -60(%rbp)
LBB13_1:                                ## =>This Inner Loop Header: Depth=1
	cmpl	$2, -60(%rbp)
	jge	LBB13_10
## %bb.2:                               ##   in Loop: Header=BB13_1 Depth=1
	movq	-48(%rbp), %rax
	movl	-36(%rbp), %ecx
	shll	$1, %ecx
	addl	-60(%rbp), %ecx
	movl	%ecx, %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	andl	$-32, %ecx
	cmpl	-24(%rbp), %ecx
	jne	LBB13_6
## %bb.3:
	testb	$1, -17(%rbp)
	je	LBB13_5
## %bb.4:
	movq	-48(%rbp), %rax
	movl	-36(%rbp), %ecx
	shll	$1, %ecx
	addl	-60(%rbp), %ecx
	movl	%ecx, %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	orl	$1, %ecx
	movl	%ecx, (%rax,%rdx,4)
LBB13_5:
	movq	-56(%rbp), %rax
	movl	-36(%rbp), %ecx
	shll	$1, %ecx
	addl	-60(%rbp), %ecx
	shll	$5, %ecx
	addl	-28(%rbp), %ecx
	movl	%ecx, %ecx
	movl	%ecx, %edx
	addq	%rdx, %rax
	movq	-16(%rbp), %rdx
	movq	%rax, (%rdx)
	movb	$1, -1(%rbp)
	jmp	LBB13_20
LBB13_6:                                ##   in Loop: Header=BB13_1 Depth=1
	movq	-48(%rbp), %rax
	movl	-36(%rbp), %ecx
	shll	$1, %ecx
	addl	-60(%rbp), %ecx
	movl	%ecx, %ecx
	movl	%ecx, %edx
	cmpl	$0, (%rax,%rdx,4)
	jne	LBB13_8
## %bb.7:                               ##   in Loop: Header=BB13_1 Depth=1
	movl	-60(%rbp), %eax
	movl	%eax, -40(%rbp)
LBB13_8:                                ##   in Loop: Header=BB13_1 Depth=1
	jmp	LBB13_9
LBB13_9:                                ##   in Loop: Header=BB13_1 Depth=1
	movl	-60(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -60(%rbp)
	jmp	LBB13_1
LBB13_10:
	cmpl	$-1, -40(%rbp)
	jne	LBB13_12
## %bb.11:
	movl	-36(%rbp), %eax
	shll	$1, %eax
	movl	_CacheFillCount(%rip), %ecx
	andl	$1, %ecx
	addl	%ecx, %eax
	movl	%eax, -64(%rbp)
	jmp	LBB13_13
LBB13_12:
	movl	-36(%rbp), %eax
	shll	$1, %eax
	addl	-40(%rbp), %eax
	movl	%eax, -64(%rbp)
LBB13_13:
	movq	-56(%rbp), %rax
	movl	-64(%rbp), %ecx
	shll	$5, %ecx
	movl	%ecx, %ecx
	movl	%ecx, %edx
	addq	%rdx, %rax
	movq	%rax, -72(%rbp)
	movq	-48(%rbp), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	andl	$1, %ecx
	cmpl	$0, %ecx
	je	LBB13_15
## %bb.14:
	movq	-48(%rbp), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	andl	$-2, %ecx
	movl	%ecx, (%rax,%rdx,4)
	movq	-48(%rbp), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	andl	$-32, %ecx
	movq	-72(%rbp), %rsi
	movl	%ecx, %edi
	movl	$32, %edx
	callq	_EBusWrite
LBB13_15:
	movl	-24(%rbp), %edi
	movq	-72(%rbp), %rsi
	movl	$32, %edx
	callq	_EBusRead
	cmpl	$1, %eax
	jne	LBB13_17
## %bb.16:
	movq	_ControlReg@GOTPCREL(%rip), %rax
	movl	-8(%rbp), %ecx
	movl	%ecx, 28(%rax)
	movl	$4, %edi
	callq	_Limn2500Exception
	movb	$0, -1(%rbp)
	jmp	LBB13_20
LBB13_17:
	movl	-24(%rbp), %eax
	movq	-48(%rbp), %rcx
	movl	-64(%rbp), %edx
	movl	%edx, %esi
	movl	%eax, (%rcx,%rsi,4)
	testb	$1, -17(%rbp)
	je	LBB13_19
## %bb.18:
	movq	-48(%rbp), %rax
	movl	-64(%rbp), %ecx
	movl	%ecx, %edx
	movl	(%rax,%rdx,4), %ecx
	orl	$1, %ecx
	movl	%ecx, (%rax,%rdx,4)
LBB13_19:
	movq	-72(%rbp), %rax
	movl	-28(%rbp), %ecx
	movl	%ecx, %edx
	addq	%rdx, %rax
	movq	-16(%rbp), %rdx
	movq	%rax, (%rdx)
	movl	_CacheFillCount(%rip), %ecx
	addl	$1, %ecx
	movl	%ecx, _CacheFillCount(%rip)
	movb	$1, -1(%rbp)
LBB13_20:
	movb	-1(%rbp), %al
	andb	$1, %al
	movzbl	%al, %eax
	addq	$80, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_UserBreak              ## @UserBreak
.zerofill __DATA,__common,_UserBreak,1,0
	.globl	_Halted                 ## @Halted
.zerofill __DATA,__common,_Halted,1,0
	.section	__DATA,__data
	.globl	_Running                ## @Running
_Running:
	.byte	1                       ## 0x1

	.globl	_PC                     ## @PC
.zerofill __DATA,__common,_PC,4,2
	.globl	_TLBPC                  ## @TLBPC
.zerofill __DATA,__common,_TLBPC,4,2
	.globl	_IFetch                 ## @IFetch
.zerofill __DATA,__common,_IFetch,1,0
	.globl	_TLBMiss                ## @TLBMiss
.zerofill __DATA,__common,_TLBMiss,1,0
	.globl	_CPULocked              ## @CPULocked
.zerofill __DATA,__common,_CPULocked,4,2
	.globl	_TLBWriteCount          ## @TLBWriteCount
.zerofill __DATA,__common,_TLBWriteCount,4,2
	.globl	_CacheFillCount         ## @CacheFillCount
.zerofill __DATA,__common,_CacheFillCount,4,2
	.comm	_ControlReg,64,4        ## @ControlReg
	.comm	_CurrentException,4,2   ## @CurrentException
	.comm	_CPUProgress,4,2        ## @CPUProgress
	.comm	_Reg,128,4              ## @Reg
	.comm	_TLB,512,4              ## @TLB
	.comm	_ICacheTags,8192,4      ## @ICacheTags
	.comm	_DCacheTags,8192,4      ## @DCacheTags
	.comm	_DCache,65536,4         ## @DCache
	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"%d\n"

	.comm	_ICache,65536,4         ## @ICache
	.comm	_LastInstruction,4,2    ## @LastInstruction
L_.str.1:                               ## @.str.1
	.asciz	"double exception, shouldnt ever happen"

.subsections_via_symbols

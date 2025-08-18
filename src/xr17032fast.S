	.build_version macos, 15, 0
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_XrReset                        ## -- Begin function XrReset
	.p2align	4
_XrReset:                               ## @XrReset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	$-126976, 4992(%rdi)            ## imm = 0xFFFE1000
	movl	$0, 4864(%rdi)
	movl	$0, 4884(%rdi)
	movl	$721156, 4944(%rdi)             ## imm = 0xB0104
	movl	$721156, 4976(%rdi)             ## imm = 0xB0104
	movl	5000(%rdi), %eax
	movl	%eax, 4868(%rdi)
	movl	$0, 4856(%rdi)
	movq	$-1, 4708(%rdi)
	movl	$0, 5016(%rdi)
	movb	$64, 5028(%rdi)
	movl	$16777216, 5031(%rdi)           ## imm = 0x1000000
	movb	$0, 4996(%rdi)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_XrExecuteFast                  ## -- Begin function XrExecuteFast
	.p2align	4
_XrExecuteFast:                         ## @XrExecuteFast
	.cfi_startproc
## %bb.0:
	xorl	%eax, %eax
	cmpb	$0, 5034(%rdi)
	je	LBB1_19
## %bb.1:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	pushq	%rax
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	movq	%rdi, %rbx
	cmpb	$0, 5033(%rdi)
	je	LBB1_5
## %bb.2:
	movl	5000(%rbx), %ecx
	movb	$0, 5028(%rbx)
	leaq	(%rcx,%rcx,8), %rcx
	movq	_LsicTable@GOTPCREL(%rip), %rdx
	cmpb	$0, 32(%rdx,%rcx,4)
	je	LBB1_18
## %bb.3:
	testb	$2, 4864(%rbx)
	je	LBB1_18
## %bb.4:
	movb	$0, 5033(%rbx)
LBB1_5:
	movl	%esi, 5012(%rbx)
	movl	$0, 5008(%rbx)
	cmpb	$0, 5032(%rbx)
	je	LBB1_14
## %bb.6:
	cmpb	$0, 5028(%rbx)
	jne	LBB1_14
## %bb.7:
	movl	4992(%rbx), %ecx
	movl	%ecx, 4888(%rbx)
	movl	4864(%rbx), %ecx
	movl	4884(%rbx), %edx
	movl	%ecx, %esi
	andl	$251658495, %esi                ## imm = 0xF0000FF
	movzwl	%cx, %edi
	shll	$8, %edi
	addl	%edi, %esi
	addl	$1342177280, %esi               ## imm = 0x50000000
	movl	%esi, 4864(%rbx)
	testl	%edx, %edx
	je	LBB1_8
## %bb.9:
	testb	%cl, %cl
	js	LBB1_11
## %bb.10:
	andl	$252, %ecx
LBB1_12:
	orl	$1280, %edx                     ## imm = 0x500
	movl	%edx, 4992(%rbx)
	andl	$1610612480, %esi               ## imm = 0x5FFFFF00
	orl	%ecx, %esi
	movl	%esi, 4864(%rbx)
	movb	$64, 5028(%rbx)
	movl	$32, 5004(%rbx)
LBB1_13:
	movw	$0, 5032(%rbx)
LBB1_14:
	cmpl	$0, 5004(%rbx)
	jle	LBB1_18
## %bb.15:
	movl	$0, 5016(%rbx)
	movb	$0, 5035(%rbx)
	movq	%rbx, -48(%rbp)                 ## 8-byte Spill
	.p2align	4
LBB1_16:                                ## =>This Inner Loop Header: Depth=1
	movq	-48(%rbp), %r12                 ## 8-byte Reload
	callq	_XrCheckConditions
	movq	-48(%rbp), %rbx                 ## 8-byte Reload
	cmpb	$0, 5035(%rbx)
	je	LBB1_16
## %bb.17:
	movl	5008(%rbx), %eax
LBB1_18:
	addq	$8, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
LBB1_19:
	retq
LBB1_8:
	movq	%rbx, %rdi
	callq	_XrReset
	xorl	%eax, %eax
	jmp	LBB1_13
LBB1_11:
	andl	$248, %ecx
	jmp	LBB1_12
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrBasicException
_XrBasicException:                      ## @XrBasicException
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edx, 4888(%rdi)
	movl	4864(%rdi), %eax
	movl	4884(%rdi), %ecx
	movl	%eax, %r8d
	andl	$251658495, %r8d                ## imm = 0xF0000FF
	movzwl	%ax, %r9d
	shll	$8, %r9d
	movl	%esi, %edx
	shll	$28, %edx
	orl	%r9d, %edx
	orl	%r8d, %edx
	movl	%edx, 4864(%rdi)
	testl	%ecx, %ecx
	je	LBB2_5
## %bb.1:
	testb	%al, %al
	js	LBB2_3
## %bb.2:
	andl	$252, %eax
LBB2_4:
	shll	$8, %esi
	orl	%esi, %ecx
	movl	%ecx, 4992(%rdi)
	andl	$-256, %edx
	orl	%eax, %edx
	movl	%edx, 4864(%rdi)
	movb	$64, 5028(%rdi)
	movl	$32, 5004(%rdi)
	popq	%rbp
	retq
LBB2_5:
	popq	%rbp
	jmp	_XrReset                        ## TAILCALL
LBB2_3:
	andl	$248, %eax
	jmp	LBB2_4
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrCheckConditions
_XrCheckConditions:                     ## @XrCheckConditions
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	5008(%r12), %eax
	cmpl	5012(%r12), %eax
	jae	LBB3_12
## %bb.1:
	movq	_LsicTable@GOTPCREL(%rip), %rbx
LBB3_2:                                 ## =>This Inner Loop Header: Depth=1
	cmpb	$0, 5033(%r12)
	jne	LBB3_12
## %bb.3:                               ##   in Loop: Header=BB3_2 Depth=1
	cmpl	$256, 5016(%r12)                ## imm = 0x100
	jae	LBB3_4
## %bb.5:                               ##   in Loop: Header=BB3_2 Depth=1
	movzbl	5028(%r12), %eax
	testb	%al, %al
	jne	LBB3_6
## %bb.7:                               ##   in Loop: Header=BB3_2 Depth=1
	movl	5000(%r12), %eax
	leaq	(%rax,%rax,8), %rax
	cmpb	$0, 32(%rbx,%rax,4)
	je	LBB3_10
LBB3_8:                                 ##   in Loop: Header=BB3_2 Depth=1
	testb	$2, 4864(%r12)
	jne	LBB3_9
LBB3_10:                                ##   in Loop: Header=BB3_2 Depth=1
	movq	%r12, %rdi
	xorl	%esi, %esi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	jne	LBB3_13
## %bb.11:                              ##   in Loop: Header=BB3_2 Depth=1
	movl	5008(%r12), %eax
	addl	$32, %eax
	movl	%eax, 5008(%r12)
	cmpl	5012(%r12), %eax
	jb	LBB3_2
	jmp	LBB3_12
LBB3_6:                                 ##   in Loop: Header=BB3_2 Depth=1
	decb	%al
	movb	%al, 5028(%r12)
	movl	5000(%r12), %eax
	leaq	(%rax,%rax,8), %rax
	cmpb	$0, 32(%rbx,%rax,4)
	jne	LBB3_8
	jmp	LBB3_10
LBB3_9:                                 ##   in Loop: Header=BB3_2 Depth=1
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$1, %esi
	callq	_XrBasicException
	movl	$32, 5004(%r12)
	jmp	LBB3_10
LBB3_13:
	movq	%rax, %r14
	addq	$216, %r14
	movq	216(%rax), %rcx
	movq	%rax, %r13
	popq	%rbp
	jmpq	*%rcx                           ## TAILCALL
LBB3_4:
	movl	$0, 5016(%r12)
LBB3_12:
	movb	$1, 5035(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrVectorException
_XrVectorException:                     ## @XrVectorException
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4884(%rdi), %eax
	testl	%eax, %eax
	je	LBB4_5
## %bb.1:
	movl	4864(%rdi), %ecx
	movl	%ecx, %edx
	testb	%cl, %cl
	js	LBB4_3
## %bb.2:
	andl	$252, %edx
LBB4_4:
	shll	$8, %esi
	orl	%esi, %eax
	movl	%eax, 4992(%rdi)
	andl	$-256, %ecx
	orl	%edx, %ecx
	movl	%ecx, 4864(%rdi)
	movb	$64, 5028(%rdi)
	movl	$32, 5004(%rdi)
	popq	%rbp
	retq
LBB4_5:
	popq	%rbp
	jmp	_XrReset                        ## TAILCALL
LBB4_3:
	andl	$248, %edx
	jmp	LBB4_4
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeInstructions
_XrDecodeInstructions:                  ## @XrDecodeInstructions
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$40, %rsp
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	movq	%rdi, %rbx
	movl	4864(%rdi), %eax
	movl	4992(%rdi), %r14d
	testb	$4, %al
	je	LBB5_1
## %bb.2:
	movl	$-1048576, %r9d                 ## imm = 0xFFF00000
	andl	4932(%rbx), %r9d
LBB5_3:
	movl	%r14d, %ecx
	andl	$1020, %ecx                     ## imm = 0x3FC
	leaq	(%rbx,%rcx,4), %r15
	addq	$608, %r15                      ## imm = 0x260
	movq	608(%rbx,%rcx,4), %rcx
	jmp	LBB5_4
	.p2align	4
LBB5_7:                                 ##   in Loop: Header=BB5_4 Depth=1
	movq	(%rcx), %rcx
LBB5_4:                                 ## =>This Inner Loop Header: Depth=1
	cmpq	%r15, %rcx
	je	LBB5_8
## %bb.5:                               ##   in Loop: Header=BB5_4 Depth=1
	cmpl	%r14d, 88(%rcx)
	jne	LBB5_7
## %bb.6:                               ##   in Loop: Header=BB5_4 Depth=1
	leaq	-112(%rcx), %r12
	cmpl	%r9d, 204(%r12)
	jne	LBB5_7
## %bb.12:
	testb	$1, %al
	je	LBB5_43
## %bb.13:
	movzbl	210(%r12), %eax
	andb	$4, %al
	je	LBB5_43
	jmp	LBB5_14
LBB5_8:
	leal	127(%r14), %ecx
	xorl	%r14d, %ecx
	movl	$32, %r11d
	cmpl	$4096, %ecx                     ## imm = 0x1000
	jae	LBB5_9
LBB5_10:
	testb	$4, %al
	movq	%rsi, -48(%rbp)                 ## 8-byte Spill
	movq	%r11, -64(%rbp)                 ## 8-byte Spill
	movl	%r9d, -56(%rbp)                 ## 4-byte Spill
	je	LBB5_11
## %bb.16:
	movl	%r14d, %ecx
	shrl	$12, %ecx
	cmpl	%ecx, 4708(%rbx)
	jne	LBB5_18
## %bb.17:
	movq	512(%rbx), %r13
LBB5_22:
	testb	$1, %r13b
	je	LBB5_23
## %bb.31:
	testb	$4, %r13b
	sete	%dl
	testb	$1, %al
	sete	%al
	orb	%dl, %al
	je	LBB5_14
## %bb.32:
	movl	%ecx, 4708(%rbx)
	movq	%r13, 512(%rbx)
	movl	%r14d, %eax
	andl	$4095, %eax                     ## imm = 0xFFF
	movl	%r13d, %edi
	andl	$-32, %edi
	shll	$7, %edi
	orl	%eax, %edi
	andb	$31, %r13b
LBB5_33:
	movl	%edi, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %rcx
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	callq	*24(%rcx,%rax,8)
	movq	%rax, -72(%rbp)                 ## 8-byte Spill
	testq	%rax, %rax
	je	LBB5_34
## %bb.35:
	movq	568(%rbx), %r12
	testq	%r12, %r12
	je	LBB5_36
LBB5_37:
	leaq	112(%r12), %rax
	movq	112(%r12), %rcx
	movq	%rcx, 568(%rbx)
	movl	%r14d, 200(%r12)
	movl	-56(%rbp), %ecx                 ## 4-byte Reload
	movl	%ecx, 204(%r12)
	movw	$0, 208(%r12)
	movb	%r13b, 210(%r12)
	movb	$0, 211(%r12)
	pxor	%xmm0, %xmm0
	movdqu	%xmm0, 152(%r12)
	movdqu	%xmm0, 168(%r12)
	movdqu	%xmm0, 184(%r12)
	movabsq	$-4294967296, %rcx              ## imm = 0xFFFFFFFF00000000
	movq	%rcx, 8(%r12)
	movq	%rcx, 32(%r12)
	movq	%rcx, 56(%r12)
	movq	%rcx, 80(%r12)
	movq	(%r15), %rcx
	movq	%rcx, 112(%r12)
	movq	%r15, 120(%r12)
	movq	%rax, 8(%rcx)
	movq	%rax, (%r15)
	leaq	592(%rbx), %rax
	leaq	128(%r12), %rcx
	movq	592(%rbx), %rdx
	movq	%rdx, 128(%r12)
	movq	%rax, 136(%r12)
	movq	%rcx, 8(%rdx)
	movq	%rcx, 592(%rbx)
	movq	%rbx, %rdi
	movq	%r12, %rsi
	movl	%r14d, %edx
	callq	_XrInsertIblockInVpage
	leaq	216(%r12), %rax
	movq	-64(%rbp), %rcx                 ## 8-byte Reload
	testl	%ecx, %ecx
	je	LBB5_42
## %bb.38:
	movq	%r14, -56(%rbp)                 ## 8-byte Spill
	movq	%rbx, -48(%rbp)                 ## 8-byte Spill
	leaq	728(%r12), %rbx
	shll	$2, %ecx
	movq	%rcx, %r14
	xorl	%r13d, %r13d
	.p2align	4
LBB5_39:                                ## =>This Inner Loop Header: Depth=1
	movq	%rax, %r15
	incb	208(%r12)
	movq	-56(%rbp), %rax                 ## 8-byte Reload
	leal	(%rax,%r13), %ecx
	movq	-72(%rbp), %rax                 ## 8-byte Reload
	movl	(%rax,%r13), %edx
	movl	%edx, %eax
	andl	$7, %eax
	movq	-48(%rbp), %rdi                 ## 8-byte Reload
	movq	%r15, %rsi
	leaq	_XrDecodeLowThree(%rip), %r8
	callq	*(%r8,%rax,8)
	cmpq	%rbx, %rax
	movl	$12, %ecx
	movl	$0, %edx
	cmovbl	%edx, %ecx
	testq	%rax, %rax
	movl	$15, %edx
	cmovel	%edx, %ecx
	cmoveq	%r15, %rax
	testl	%ecx, %ecx
	jne	LBB5_40
## %bb.41:                              ##   in Loop: Header=BB5_39 Depth=1
	addq	$4, %r13
	cmpq	%r13, %r14
	jne	LBB5_39
	jmp	LBB5_42
LBB5_40:
	cmpl	$12, %ecx
	jne	LBB5_43
LBB5_42:
	leaq	_XrSpecialLinkageInstruction(%rip), %rcx
	movq	%rcx, (%rax)
LBB5_43:
	movq	%r12, %rax
	addq	$40, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
LBB5_1:
	movl	$-1, %r9d
	jmp	LBB5_3
LBB5_9:
	movl	%r14d, %ecx
	andl	$4095, %ecx                     ## imm = 0xFFF
	movl	$4096, %r11d                    ## imm = 0x1000
	subl	%ecx, %r11d
	shrl	$2, %r11d
	jmp	LBB5_10
LBB5_11:
	xorl	%r13d, %r13d
	movl	%r14d, %edi
	jmp	LBB5_33
LBB5_18:
	movl	$-1048576, %edx                 ## imm = 0xFFF00000
	andl	4932(%rbx), %edx
	orl	%ecx, %edx
	xorl	%r10d, %r10d
LBB5_21:                                ## =>This Inner Loop Header: Depth=1
	movq	(%rbx,%r10,8), %r13
	movl	%r13d, %edi
	andl	$16, %edi
	cmpq	$1, %rdi
	movl	$0, %edi
	sbbl	%edi, %edi
	orl	$1048575, %edi                  ## imm = 0xFFFFF
	movq	%r13, %r8
	shrq	$32, %r8
	andl	%edi, %r8d
	andl	%edx, %edi
	cmpl	%edi, %r8d
	je	LBB5_22
## %bb.19:                              ##   in Loop: Header=BB5_21 Depth=1
	movq	8(%rbx,%r10,8), %r13
	movl	%r13d, %edi
	andl	$16, %edi
	cmpq	$1, %rdi
	movl	$0, %edi
	sbbl	%edi, %edi
	orl	$1048575, %edi                  ## imm = 0xFFFFF
	movq	%r13, %r8
	shrq	$32, %r8
	andl	%edi, %r8d
	andl	%edx, %edi
	cmpl	%edi, %r8d
	je	LBB5_22
## %bb.20:                              ##   in Loop: Header=BB5_21 Depth=1
	addq	$2, %r10
	cmpq	$32, %r10
	jne	LBB5_21
## %bb.25:
	movl	%edx, 4932(%rbx)
	movl	$-4194304, %edx                 ## imm = 0xFFC00000
	andl	4948(%rbx), %edx
	leal	(%rdx,%rcx,4), %ecx
	movl	%ecx, 4948(%rbx)
	movb	$0, 5031(%rbx)
	testb	$8, %al
	jne	LBB5_27
## %bb.26:
	movl	%eax, %ecx
	andl	$-16776969, %ecx                ## imm = 0xFF0000F7
	shll	$8, %eax
	andl	$16774912, %eax                 ## imm = 0xFFF700
	movl	%r14d, 4900(%rbx)
	movl	%r14d, 4904(%rbx)
	addl	%ecx, %eax
	addl	$8, %eax
	movl	%eax, 4864(%rbx)
LBB5_27:
	movq	%rbx, %rdi
	movl	$14, %esi
	jmp	LBB5_28
LBB5_23:
	testb	$8, %al
	jne	LBB5_29
## %bb.24:
	movl	%r14d, 4892(%rbx)
	movl	%r14d, 4888(%rbx)
	movl	%eax, %ecx
	andl	$-16776969, %ecx                ## imm = 0xFF0000F7
	shll	$8, %eax
	andl	$16774912, %eax                 ## imm = 0xFFF700
	orl	%ecx, %eax
	movl	$12, %esi
LBB5_30:
	andl	$268435455, %eax                ## imm = 0xFFFFFFF
	movl	%esi, %ecx
	shll	$28, %ecx
	orl	%eax, %ecx
	movl	%ecx, 4864(%rbx)
	movq	%rbx, %rdi
LBB5_28:
	callq	_XrVectorException
	xorl	%r12d, %r12d
	jmp	LBB5_43
LBB5_34:
	movl	%r14d, 4892(%rbx)
	movq	%rbx, %rdi
	movl	$4, %esi
	jmp	LBB5_15
LBB5_36:
	movq	%rbx, %rdi
	movq	-48(%rbp), %rsi                 ## 8-byte Reload
	callq	_XrPopulateIblockList
	movq	568(%rbx), %r12
	jmp	LBB5_37
LBB5_14:
	movl	%r14d, 4892(%rbx)
	movq	%rbx, %rdi
	movl	$12, %esi
LBB5_15:
	movl	%r14d, %edx
	callq	_XrBasicException
	xorl	%r12d, %r12d
	jmp	LBB5_43
LBB5_29:
	movq	4900(%rbx), %xmm0               ## xmm0 = mem[0],zero
	pshufd	$225, %xmm0, %xmm0              ## xmm0 = xmm0[1,0,2,3]
	movq	%xmm0, 4888(%rbx)
	andl	$-9, %eax
	cmpb	$1, 5031(%rbx)
	movl	$13, %esi
	sbbl	$0, %esi
	jmp	LBB5_30
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrInsertIblockInVpage
_XrInsertIblockInVpage:                 ## @XrInsertIblockInVpage
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
                                        ## kill: def $edx killed $edx def $rdx
	movl	%edx, %ecx
	andl	$-4096, %ecx                    ## imm = 0xF000
	shrl	$8, %edx
	andl	$496, %edx                      ## imm = 0x1F0
	addq	%rdi, %rdx
	addq	$5040, %rdx                     ## imm = 0x13B0
	movq	%rdx, %rax
	.p2align	4
LBB6_1:                                 ## =>This Inner Loop Header: Depth=1
	movq	(%rax), %rax
	cmpq	%rdx, %rax
	je	LBB6_4
## %bb.2:                               ##   in Loop: Header=BB6_1 Depth=1
	cmpl	%ecx, 32(%rax)
	jne	LBB6_1
## %bb.3:
	incl	36(%rax)
	jmp	LBB6_5
LBB6_4:
	movq	584(%rdi), %rax
	movq	(%rax), %r8
	movq	%r8, 584(%rdi)
	movl	%ecx, 32(%rax)
	movl	$1, 36(%rax)
	movq	(%rdx), %rcx
	movq	%rcx, (%rax)
	movq	%rdx, 8(%rax)
	movq	%rax, 8(%rcx)
	movq	%rax, (%rdx)
LBB6_5:
	movq	%rax, 144(%rsi)
	leaq	16(%rax), %rcx
	leaq	96(%rsi), %rdx
	movq	16(%rax), %rdi
	movq	%rdi, 96(%rsi)
	movq	%rcx, 104(%rsi)
	movq	%rdx, 8(%rdi)
	movq	%rdx, 16(%rax)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrSpecialLinkageInstruction
_XrSpecialLinkageInstruction:           ## @XrSpecialLinkageInstruction
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%r13, %rbx
	movq	152(%r13), %r13
	testq	%r13, %r13
	je	LBB7_1
LBB7_7:
	movzbl	208(%rbx), %eax
	addl	%eax, 5008(%r12)
	movzbl	4996(%r12), %eax
	leal	1(%rax), %ecx
	movb	%cl, 4996(%r12)
	testb	$31, %al
	je	LBB7_8
## %bb.9:
	leaq	216(%r13), %r14
	movq	216(%r13), %rax
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB7_1:
	movq	%r12, %rdi
	movq	%rbx, %rsi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB7_2
## %bb.3:
	movq	%rax, %r13
	leaq	152(%rbx), %rax
	movzbl	209(%r13), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 209(%r13)
	andl	$3, %ecx
	movl	%ecx, %edx
	movq	168(%r13,%rdx,8), %rdx
	testq	%rdx, %rdx
	je	LBB7_6
## %bb.4:
	cmpq	%r13, (%rdx)
	jne	LBB7_6
## %bb.5:
	movq	$0, (%rdx)
LBB7_6:
	movq	%r13, (%rax)
	movq	%rax, 168(%r13,%rcx,8)
	jmp	LBB7_7
LBB7_8:
	popq	%rbp
	retq
LBB7_2:
	subq	%rbx, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrPopulateIblockList
_XrPopulateIblockList:                  ## @XrPopulateIblockList
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	600(%rdi), %rax
	movl	$32, %ecx
	jmp	LBB8_1
	.p2align	4
LBB8_18:                                ##   in Loop: Header=BB8_1 Depth=1
	movq	568(%rdi), %r8
	movq	%r8, 112(%rdx)
	movq	%rdx, 568(%rdi)
LBB8_19:                                ##   in Loop: Header=BB8_1 Depth=1
	addq	$8, %rax
	decl	%ecx
	je	LBB8_20
LBB8_1:                                 ## =>This Inner Loop Header: Depth=1
	movq	(%rax), %rax
	leaq	-128(%rax), %rdx
	cmpq	%rdx, %rsi
	je	LBB8_19
## %bb.2:                               ##   in Loop: Header=BB8_1 Depth=1
	movq	168(%rdx), %r8
	testq	%r8, %r8
	je	LBB8_5
## %bb.3:                               ##   in Loop: Header=BB8_1 Depth=1
	cmpq	%rdx, (%r8)
	je	LBB8_4
LBB8_5:                                 ##   in Loop: Header=BB8_1 Depth=1
	movq	176(%rdx), %r8
	testq	%r8, %r8
	je	LBB8_8
LBB8_6:                                 ##   in Loop: Header=BB8_1 Depth=1
	cmpq	%rdx, (%r8)
	jne	LBB8_8
## %bb.7:                               ##   in Loop: Header=BB8_1 Depth=1
	movq	$0, (%r8)
	jmp	LBB8_8
LBB8_4:                                 ##   in Loop: Header=BB8_1 Depth=1
	movq	$0, (%r8)
	movq	176(%rdx), %r8
	testq	%r8, %r8
	jne	LBB8_6
	.p2align	4
LBB8_8:                                 ##   in Loop: Header=BB8_1 Depth=1
	movq	184(%rdx), %r8
	testq	%r8, %r8
	je	LBB8_11
## %bb.9:                               ##   in Loop: Header=BB8_1 Depth=1
	cmpq	%rdx, (%r8)
	je	LBB8_10
LBB8_11:                                ##   in Loop: Header=BB8_1 Depth=1
	movq	192(%rdx), %r8
	testq	%r8, %r8
	je	LBB8_14
LBB8_12:                                ##   in Loop: Header=BB8_1 Depth=1
	cmpq	%rdx, (%r8)
	jne	LBB8_14
## %bb.13:                              ##   in Loop: Header=BB8_1 Depth=1
	movq	$0, (%r8)
	jmp	LBB8_14
LBB8_10:                                ##   in Loop: Header=BB8_1 Depth=1
	movq	$0, (%r8)
	movq	192(%rdx), %r8
	testq	%r8, %r8
	jne	LBB8_12
	.p2align	4
LBB8_14:                                ##   in Loop: Header=BB8_1 Depth=1
	movq	128(%rdx), %r8
	movq	136(%rdx), %r9
	movq	%r8, (%r9)
	movq	%r9, 8(%r8)
	movq	112(%rdx), %r8
	movq	120(%rdx), %r9
	movq	%r8, (%r9)
	movq	%r9, 8(%r8)
	movq	96(%rdx), %r8
	movq	104(%rdx), %r9
	movq	%r8, (%r9)
	movq	%r9, 8(%r8)
	movq	144(%rdx), %r8
	decl	36(%r8)
	je	LBB8_15
## %bb.16:                              ##   in Loop: Header=BB8_1 Depth=1
	cmpb	$0, 211(%rdx)
	jne	LBB8_17
	jmp	LBB8_18
	.p2align	4
LBB8_15:                                ##   in Loop: Header=BB8_1 Depth=1
	movq	(%r8), %r9
	movq	8(%r8), %r10
	movq	%r9, (%r10)
	movq	%r10, 8(%r9)
	movq	584(%rdi), %r9
	movq	%r9, (%r8)
	movq	%r8, 584(%rdi)
	cmpb	$0, 211(%rdx)
	je	LBB8_18
LBB8_17:                                ##   in Loop: Header=BB8_1 Depth=1
	movq	152(%rdx), %r8
	movq	576(%rdi), %r9
	movq	%r9, (%r8)
	movq	%r8, 576(%rdi)
	jmp	LBB8_18
LBB8_20:
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeMajor
_XrDecodeMajor:                         ## @XrDecodeMajor
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edx, %eax
	andl	$63, %eax
	leaq	_XrDecodeLowSix(%rip), %r8
	popq	%rbp
	jmpq	*(%r8,%rax,8)                   ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeJ
_XrDecodeJ:                             ## @XrDecodeJ
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
                                        ## kill: def $ecx killed $ecx def $rcx
                                        ## kill: def $edx killed $edx def $rdx
	leaq	_XrExecuteJ(%rip), %rax
	movq	%rax, (%rsi)
	andl	$-2147483648, %ecx              ## imm = 0x80000000
	shrl	$3, %edx
	leal	(%rcx,%rdx,4), %eax
	movl	%eax, 8(%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeJal
_XrDecodeJal:                           ## @XrDecodeJal
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
                                        ## kill: def $ecx killed $ecx def $rcx
                                        ## kill: def $edx killed $edx def $rdx
	leaq	_XrExecuteJal(%rip), %rax
	movq	%rax, (%rsi)
	andl	$-2147483648, %ecx              ## imm = 0x80000000
	shrl	$3, %edx
	leal	(%rcx,%rdx,4), %eax
	movl	%eax, 8(%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeIllegalInstruction
_XrDecodeIllegalInstruction:            ## @XrDecodeIllegalInstruction
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteIllegalInstruction(%rip), %rax
	movq	%rax, (%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeLui
_XrDecodeLui:                           ## @XrDecodeLui
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteOri(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	andl	$-65536, %edx                   ## imm = 0xFFFF0000
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeBpo
_XrDecodeBpo:                           ## @XrDecodeBpo
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteBpo(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 12(%rsi)
	sarl	$9, %edx
	andl	$-4, %edx
	movl	%edx, 8(%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeStoreLongImmOffsetImm
_XrDecodeStoreLongImmOffsetImm:         ## @XrDecodeStoreLongImmOffsetImm
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteStoreLongImmOffsetImm(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 12(%rsi)
	movl	%edx, %eax
	shrl	$11, %eax
	andb	$31, %al
	movb	%al, 13(%rsi)
	shrl	$14, %edx
	andl	$-4, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeOri
_XrDecodeOri:                           ## @XrDecodeOri
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteOri(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	shrl	$16, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeBpe
_XrDecodeBpe:                           ## @XrDecodeBpe
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteBpe(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 12(%rsi)
	sarl	$9, %edx
	andl	$-4, %edx
	movl	%edx, 8(%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeStoreIntImmOffsetImm
_XrDecodeStoreIntImmOffsetImm:          ## @XrDecodeStoreIntImmOffsetImm
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteStoreIntImmOffsetImm(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 12(%rsi)
	movl	%edx, %eax
	shrl	$11, %eax
	andb	$31, %al
	movb	%al, 13(%rsi)
	shrl	$15, %edx
	andl	$-2, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeXori
_XrDecodeXori:                          ## @XrDecodeXori
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteXori(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	shrl	$16, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeBge
_XrDecodeBge:                           ## @XrDecodeBge
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteBge(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 12(%rsi)
	sarl	$9, %edx
	andl	$-4, %edx
	movl	%edx, 8(%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeStoreByteImmOffsetImm
_XrDecodeStoreByteImmOffsetImm:         ## @XrDecodeStoreByteImmOffsetImm
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteStoreByteImmOffsetImm(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 12(%rsi)
	movl	%edx, %eax
	shrl	$11, %eax
	andb	$31, %al
	movb	%al, 13(%rsi)
	shrl	$16, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeAndi
_XrDecodeAndi:                          ## @XrDecodeAndi
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteAndi(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	shrl	$16, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeBle
_XrDecodeBle:                           ## @XrDecodeBle
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteBle(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 12(%rsi)
	sarl	$9, %edx
	andl	$-4, %edx
	movl	%edx, 8(%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeSltiSigned
_XrDecodeSltiSigned:                    ## @XrDecodeSltiSigned
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteSltiSigned(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	sarl	$16, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeBgt
_XrDecodeBgt:                           ## @XrDecodeBgt
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteBgt(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 12(%rsi)
	sarl	$9, %edx
	andl	$-4, %edx
	movl	%edx, 8(%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecode101001
_XrDecode101001:                        ## @XrDecode101001
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edx, %eax
	shrl	$28, %eax
	leaq	_XrDecodeFunctions101001(%rip), %r8
	popq	%rbp
	jmpq	*(%r8,%rax,8)                   ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeStoreLongImmOffsetReg
_XrDecodeStoreLongImmOffsetReg:         ## @XrDecodeStoreLongImmOffsetReg
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteStoreLongImmOffsetReg(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %ecx
	movl	$32, %edi
	cmovel	%edi, %ecx
	movb	%cl, 12(%rsi)
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %r8d
	shrl	$11, %r8d
	andb	$31, %r8b
	orl	%eax, %ecx
	movzbl	%r8b, %eax
	cmovel	%edi, %eax
	movb	%al, 13(%rsi)
	shrl	$14, %edx
	andl	$-4, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeLoadLongImmOffset
_XrDecodeLoadLongImmOffset:             ## @XrDecodeLoadLongImmOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteLoadLongImmOffset(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	shrl	$14, %edx
	andl	$-4, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeSlti
_XrDecodeSlti:                          ## @XrDecodeSlti
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteSlti(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	shrl	$16, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeBlt
_XrDecodeBlt:                           ## @XrDecodeBlt
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteBlt(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 12(%rsi)
	sarl	$9, %edx
	andl	$-4, %edx
	movl	%edx, 8(%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecode110001
_XrDecode110001:                        ## @XrDecode110001
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edx, %eax
	shrl	$28, %eax
	leaq	_XrDecodeFunctions110001(%rip), %r8
	popq	%rbp
	jmpq	*(%r8,%rax,8)                   ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeStoreIntImmOffsetReg
_XrDecodeStoreIntImmOffsetReg:          ## @XrDecodeStoreIntImmOffsetReg
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteStoreIntImmOffsetReg(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %ecx
	movl	$32, %edi
	cmovel	%edi, %ecx
	movb	%cl, 12(%rsi)
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %r8d
	shrl	$11, %r8d
	andb	$31, %r8b
	orl	%eax, %ecx
	movzbl	%r8b, %eax
	cmovel	%edi, %eax
	movb	%al, 13(%rsi)
	shrl	$15, %edx
	andl	$-2, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeLoadIntImmOffset
_XrDecodeLoadIntImmOffset:              ## @XrDecodeLoadIntImmOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteLoadIntImmOffset(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	shrl	$15, %edx
	andl	$-2, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeSubi
_XrDecodeSubi:                          ## @XrDecodeSubi
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteSubi(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	shrl	$16, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeBne
_XrDecodeBne:                           ## @XrDecodeBne
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteBne(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 12(%rsi)
	sarl	$9, %edx
	andl	$-4, %edx
	movl	%edx, 8(%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeJalr
_XrDecodeJalr:                          ## @XrDecodeJalr
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteJalr(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	sarl	$14, %edx
	andl	$-4, %edx
	movl	%edx, 8(%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecode111001
_XrDecode111001:                        ## @XrDecode111001
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edx, %eax
	shrl	$28, %eax
	leaq	_XrDecodeFunctions111001(%rip), %r8
	popq	%rbp
	jmpq	*(%r8,%rax,8)                   ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeStoreByteImmOffsetReg
_XrDecodeStoreByteImmOffsetReg:         ## @XrDecodeStoreByteImmOffsetReg
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteStoreByteImmOffsetReg(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	movl	%edx, %edi
	shrl	$6, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %ecx
	movl	$32, %edi
	cmovel	%edi, %ecx
	movb	%cl, 12(%rsi)
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %r8d
	shrl	$11, %r8d
	andb	$31, %r8b
	orl	%eax, %ecx
	movzbl	%r8b, %eax
	cmovel	%edi, %eax
	movb	%al, 13(%rsi)
	shrl	$16, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeLoadByteImmOffset
_XrDecodeLoadByteImmOffset:             ## @XrDecodeLoadByteImmOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteLoadByteImmOffset(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	shrl	$16, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeAddi
_XrDecodeAddi:                          ## @XrDecodeAddi
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteAddi(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	shrl	$16, %edx
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeBeq
_XrDecodeBeq:                           ## @XrDecodeBeq
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edx, %eax
	sarl	$9, %eax
	andl	$-4, %eax
	movl	%eax, 8(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$1984, %ecx                     ## imm = 0x7C0
	shrl	$6, %edx
	andb	$31, %dl
	orl	%eax, %ecx
	movzbl	%dl, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 12(%rsi)
	leaq	_XrExecuteB(%rip), %rax
	leaq	_XrExecuteBeq(%rip), %rcx
	cmoveq	%rax, %rcx
	movq	%rcx, (%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteIllegalInstruction
_XrExecuteIllegalInstruction:           ## @XrExecuteIllegalInstruction
	.cfi_startproc
## %bb.0:
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1879048192, %edx               ## imm = 0x70000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB42_1
## %bb.2:
	testb	%al, %al
	js	LBB42_4
## %bb.3:
	andl	$252, %eax
LBB42_5:
	orl	$1792, %ecx                     ## imm = 0x700
	movl	%ecx, 4992(%r12)
	andl	$2147483392, %edx               ## imm = 0x7FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	movzbl	208(%r13), %eax
	addl	%eax, 5008(%r12)
	jmp	_XrCheckConditions              ## TAILCALL
LBB42_1:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%r12, %rdi
	callq	_XrReset
	popq	%rbp
	movzbl	208(%r13), %eax
	addl	%eax, 5008(%r12)
	jmp	_XrCheckConditions              ## TAILCALL
LBB42_4:
	andl	$248, %eax
	jmp	LBB42_5
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteOri
_XrExecuteOri:                          ## @XrExecuteOri
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	movzbl	13(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	orl	8(%r14), %ecx
	movl	%ecx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteBpo
_XrExecuteBpo:                          ## @XrExecuteBpo
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	testb	$1, 4728(%r12,%rax,4)
	jne	LBB44_2
## %bb.1:
	movl	$160, %ebx
	movl	$4, %eax
	jmp	LBB44_3
LBB44_2:
	movl	8(%r14), %eax
	movl	$152, %ebx
LBB44_3:
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	je	LBB44_4
LBB44_10:
	movzbl	208(%r13), %ecx
	addl	%ecx, 5008(%r12)
	movzbl	4996(%r12), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 4996(%r12)
	testb	$31, %cl
	je	LBB44_11
## %bb.12:
	leaq	216(%rax), %r14
	movq	216(%rax), %rcx
	movq	%rax, %r13
	popq	%rbp
	jmpq	*%rcx                           ## TAILCALL
LBB44_4:
	movq	%r12, %rdi
	movq	%r13, %rsi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB44_5
## %bb.6:
	addq	%r13, %rbx
	movzbl	209(%rax), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 209(%rax)
	andl	$3, %ecx
	movl	%ecx, %edx
	movq	168(%rax,%rdx,8), %rdx
	testq	%rdx, %rdx
	je	LBB44_9
## %bb.7:
	cmpq	%rax, (%rdx)
	jne	LBB44_9
## %bb.8:
	movq	$0, (%rdx)
LBB44_9:
	movq	%rax, (%rbx)
	movq	%rbx, 168(%rax,%rcx,8)
	jmp	LBB44_10
LBB44_11:
	popq	%rbp
	retq
LBB44_5:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteStoreLongImmOffsetImm
_XrExecuteStoreLongImmOffsetImm:        ## @XrExecuteStoreLongImmOffsetImm
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movq	%r13, %rdi
	movzbl	12(%r14), %eax
	movl	4728(%r12,%rax,4), %ebx
	addl	8(%r14), %ebx
	testb	$3, %bl
	jne	LBB45_1
## %bb.2:
	movzbl	13(%r14), %r15d
	shll	$27, %r15d
	sarl	$27, %r15d
	testb	$4, 4864(%r12)
	je	LBB45_3
## %bb.4:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(%rdi,%rax,8), %r13
	movq	8(%rdi,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB45_10
## %bb.5:
	testb	$2, %al
	je	LBB45_10
## %bb.6:
	movl	16(%r13), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB45_10
## %bb.7:
	movq	(%r13), %rcx
	testq	%rcx, %rcx
	je	LBB45_12
## %bb.8:
	andl	$4092, %ebx                     ## imm = 0xFFC
	movl	%r15d, (%rcx,%rbx)
LBB45_9:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	movq	%rdi, %r13
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB45_10:
	movq	%rdi, -16(%rbp)                 ## 8-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r13, %rdx
	movl	$1, %ecx
	callq	_XrTranslateDstream
	movq	-16(%rbp), %rdi                 ## 8-byte Reload
	testl	%eax, %eax
	je	LBB45_20
## %bb.11:
	movq	8(%r13), %rax
LBB45_12:
	movq	%rdi, %r13
	andl	$4092, %ebx                     ## imm = 0xFFC
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
	jmp	LBB45_13
LBB45_1:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %edx
	movq	%rdi, %r13
	movq	%r12, %rdi
	movl	$9, %esi
	callq	_XrBasicException
	movq	%r13, %rdi
	jmp	LBB45_20
LBB45_3:
	movq	%rdi, %r13
LBB45_13:
	movl	%r15d, -4(%rbp)
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	leaq	-4(%rbp), %rsi
	movl	$4, %edx
	movq	%r12, %rcx
	movq	%r12, %r15
	callq	*8(%r8,%rax,8)
	testl	%eax, %eax
	movq	%r13, %rdi
	je	LBB45_9
## %bb.14:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB45_15
## %bb.16:
	testb	%al, %al
	js	LBB45_18
## %bb.17:
	andl	$252, %eax
LBB45_19:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
LBB45_20:
	subq	%rdi, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB45_15:
	movq	%rdi, %r13
	movq	%r12, %rdi
	callq	_XrReset
	movq	%r15, %r12
	movq	%r13, %rdi
	jmp	LBB45_20
LBB45_18:
	andl	$248, %eax
	jmp	LBB45_19
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrTranslateDstream
_XrTranslateDstream:                    ## @XrTranslateDstream
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%rbx
	pushq	%rax
	.cfi_offset %rbx, -24
	movl	%esi, %r9d
	shrl	$12, %r9d
	movl	$-1048576, %r10d                ## imm = 0xFFF00000
	andl	4964(%rdi), %r10d
	orl	%r9d, %r10d
	xorl	%r8d, %r8d
	.p2align	4
LBB46_1:                                ## =>This Inner Loop Header: Depth=1
	movq	256(%rdi,%r8,8), %rax
	movl	%eax, %r11d
	andl	$16, %r11d
	cmpq	$1, %r11
	movl	$0, %r11d
	sbbl	%r11d, %r11d
	orl	$1048575, %r11d                 ## imm = 0xFFFFF
	movq	%rax, %rbx
	shrq	$32, %rbx
	andl	%r11d, %ebx
	andl	%r10d, %r11d
	cmpl	%r11d, %ebx
	je	LBB46_11
## %bb.2:                               ##   in Loop: Header=BB46_1 Depth=1
	movq	264(%rdi,%r8,8), %rax
	movl	%eax, %r11d
	andl	$16, %r11d
	cmpq	$1, %r11
	movl	$0, %r11d
	sbbl	%r11d, %r11d
	orl	$1048575, %r11d                 ## imm = 0xFFFFF
	movq	%rax, %rbx
	shrq	$32, %rbx
	andl	%r11d, %ebx
	andl	%r10d, %r11d
	cmpl	%r11d, %ebx
	je	LBB46_10
## %bb.3:                               ##   in Loop: Header=BB46_1 Depth=1
	addq	$2, %r8
	cmpq	$32, %r8
	jne	LBB46_1
## %bb.4:
	movl	%r10d, 4964(%rdi)
	movl	$-4194304, %eax                 ## imm = 0xFFC00000
	andl	4980(%rdi), %eax
	leal	(%rax,%r9,4), %eax
	movl	%eax, 4980(%rdi)
	movb	%cl, 5031(%rdi)
	movl	4864(%rdi), %eax
	testb	$8, %al
	jne	LBB46_6
## %bb.5:
	movl	%eax, %ecx
	andl	$-16776969, %ecx                ## imm = 0xFF0000F7
	shll	$8, %eax
	andl	$16774912, %eax                 ## imm = 0xFFF700
	movl	%esi, 4900(%rdi)
	movl	4992(%rdi), %edx
	movl	%edx, 4904(%rdi)
	addl	%ecx, %eax
	addl	$8, %eax
	movl	%eax, 4864(%rdi)
LBB46_6:
	movl	4884(%rdi), %ecx
	testl	%ecx, %ecx
	je	LBB46_23
## %bb.7:
	movl	%eax, %edx
	testb	%al, %al
	js	LBB46_25
## %bb.8:
	andl	$252, %edx
LBB46_9:
	orl	$3840, %ecx                     ## imm = 0xF00
	movl	%ecx, 4992(%rdi)
	andl	$-256, %eax
	orl	%edx, %eax
	movl	%eax, 4864(%rdi)
	movb	$64, 5028(%rdi)
	movl	$32, 5004(%rdi)
LBB46_24:
	xorl	%eax, %eax
	addq	$8, %rsp
	popq	%rbx
	popq	%rbp
	retq
LBB46_10:
	incq	%r8
LBB46_11:
	testb	$1, %al
	je	LBB46_17
## %bb.12:
	testb	$4, %al
	je	LBB46_14
## %bb.13:
	testb	$1, 4864(%rdi)
	jne	LBB46_21
LBB46_14:
	testl	%ecx, %ecx
	je	LBB46_16
## %bb.15:
	movl	%eax, %ecx
	andl	$2, %ecx
	je	LBB46_20
LBB46_16:
	movq	%rax, 8(%rdx)
	movl	%r8d, 16(%rdx)
	shll	$7, %eax
	movl	%eax, %ecx
	shrl	$27, %ecx
	leaq	(%rcx,%rcx,4), %rcx
	movq	_EBusBranches@GOTPCREL(%rip), %rsi
	andl	$134213632, %eax                ## imm = 0x7FFF000
	movl	%eax, %edi
	movq	%rdx, %rbx
	callq	*24(%rsi,%rcx,8)
	movq	%rax, (%rbx)
	movl	$1, %eax
	addq	$8, %rsp
	popq	%rbx
	popq	%rbp
	retq
LBB46_17:
	movl	4864(%rdi), %eax
	testb	$8, %al
	jne	LBB46_26
## %bb.18:
	leaq	4992(%rdi), %rdx
	movl	%eax, %r8d
	andl	$-16776969, %r8d                ## imm = 0xFF0000F7
	shll	$8, %eax
	andl	$16774912, %eax                 ## imm = 0xFFF700
	orl	%r8d, %eax
LBB46_19:
	movl	(%rdx), %edx
	movl	%esi, 4892(%rdi)
	movl	%edx, 4888(%rdi)
	cmpl	$1, %ecx
	movl	$13, %esi
	sbbl	$0, %esi
	andl	$268435455, %eax                ## imm = 0xFFFFFFF
	movl	%esi, %ecx
	shll	$28, %ecx
	orl	%eax, %ecx
	movl	%ecx, 4864(%rdi)
	callq	_XrVectorException
	jmp	LBB46_24
LBB46_20:
	movl	%esi, 4892(%rdi)
	movl	4992(%rdi), %edx
	movl	$13, %esi
	jmp	LBB46_22
LBB46_21:
	movl	%esi, 4892(%rdi)
	cmpl	$1, %ecx
	movl	$13, %esi
	sbbl	$0, %esi
	movl	4992(%rdi), %edx
LBB46_22:
	callq	_XrBasicException
	jmp	LBB46_24
LBB46_23:
	callq	_XrReset
	jmp	LBB46_24
LBB46_25:
	andl	$248, %edx
	jmp	LBB46_9
LBB46_26:
	movl	4900(%rdi), %esi
	leaq	4904(%rdi), %rdx
	andl	$-9, %eax
	movzbl	5031(%rdi), %ecx
	jmp	LBB46_19
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteBpe
_XrExecuteBpe:                          ## @XrExecuteBpe
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	testb	$1, 4728(%r12,%rax,4)
	jne	LBB47_1
## %bb.2:
	movl	8(%r14), %eax
	movl	$152, %ebx
	jmp	LBB47_3
LBB47_1:
	movl	$160, %ebx
	movl	$4, %eax
LBB47_3:
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	je	LBB47_4
LBB47_10:
	movzbl	208(%r13), %ecx
	addl	%ecx, 5008(%r12)
	movzbl	4996(%r12), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 4996(%r12)
	testb	$31, %cl
	je	LBB47_11
## %bb.12:
	leaq	216(%rax), %r14
	movq	216(%rax), %rcx
	movq	%rax, %r13
	popq	%rbp
	jmpq	*%rcx                           ## TAILCALL
LBB47_4:
	movq	%r12, %rdi
	movq	%r13, %rsi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB47_5
## %bb.6:
	addq	%r13, %rbx
	movzbl	209(%rax), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 209(%rax)
	andl	$3, %ecx
	movl	%ecx, %edx
	movq	168(%rax,%rdx,8), %rdx
	testq	%rdx, %rdx
	je	LBB47_9
## %bb.7:
	cmpq	%rax, (%rdx)
	jne	LBB47_9
## %bb.8:
	movq	$0, (%rdx)
LBB47_9:
	movq	%rax, (%rbx)
	movq	%rbx, 168(%rax,%rcx,8)
	jmp	LBB47_10
LBB47_11:
	popq	%rbp
	retq
LBB47_5:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteStoreIntImmOffsetImm
_XrExecuteStoreIntImmOffsetImm:         ## @XrExecuteStoreIntImmOffsetImm
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movq	%r13, %rdi
	movzbl	12(%r14), %eax
	movl	4728(%r12,%rax,4), %ebx
	addl	8(%r14), %ebx
	testb	$1, %bl
	jne	LBB48_1
## %bb.2:
	movzbl	13(%r14), %r15d
	shll	$27, %r15d
	sarl	$27, %r15d
	testb	$4, 4864(%r12)
	je	LBB48_3
## %bb.4:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(%rdi,%rax,8), %r13
	movq	8(%rdi,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB48_10
## %bb.5:
	testb	$2, %al
	je	LBB48_10
## %bb.6:
	movl	16(%r13), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB48_10
## %bb.7:
	movq	(%r13), %rcx
	testq	%rcx, %rcx
	je	LBB48_12
## %bb.8:
	andl	$4094, %ebx                     ## imm = 0xFFE
	movw	%r15w, (%rcx,%rbx)
LBB48_9:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	movq	%rdi, %r13
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB48_10:
	movq	%rdi, -16(%rbp)                 ## 8-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r13, %rdx
	movl	$1, %ecx
	callq	_XrTranslateDstream
	movq	-16(%rbp), %rdi                 ## 8-byte Reload
	testl	%eax, %eax
	je	LBB48_20
## %bb.11:
	movq	8(%r13), %rax
LBB48_12:
	movq	%rdi, %r13
	andl	$4094, %ebx                     ## imm = 0xFFE
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
	jmp	LBB48_13
LBB48_1:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %edx
	movq	%rdi, %r13
	movq	%r12, %rdi
	movl	$9, %esi
	callq	_XrBasicException
	movq	%r13, %rdi
	jmp	LBB48_20
LBB48_3:
	movq	%rdi, %r13
LBB48_13:
	movl	%r15d, -4(%rbp)
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	leaq	-4(%rbp), %rsi
	movl	$2, %edx
	movq	%r12, %rcx
	movq	%r12, %r15
	callq	*8(%r8,%rax,8)
	testl	%eax, %eax
	movq	%r13, %rdi
	je	LBB48_9
## %bb.14:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB48_15
## %bb.16:
	testb	%al, %al
	js	LBB48_18
## %bb.17:
	andl	$252, %eax
LBB48_19:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
LBB48_20:
	subq	%rdi, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB48_15:
	movq	%rdi, %r13
	movq	%r12, %rdi
	callq	_XrReset
	movq	%r15, %r12
	movq	%r13, %rdi
	jmp	LBB48_20
LBB48_18:
	andl	$248, %eax
	jmp	LBB48_19
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteXori
_XrExecuteXori:                         ## @XrExecuteXori
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	movzbl	13(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	xorl	8(%r14), %ecx
	movl	%ecx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteBge
_XrExecuteBge:                          ## @XrExecuteBge
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	cmpl	$0, 4728(%r12,%rax,4)
	js	LBB50_1
## %bb.2:
	movl	8(%r14), %eax
	movl	$152, %ebx
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	jne	LBB50_10
	jmp	LBB50_4
LBB50_1:
	movl	$160, %ebx
	movl	$4, %eax
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	je	LBB50_4
LBB50_10:
	movzbl	208(%r13), %ecx
	addl	%ecx, 5008(%r12)
	movzbl	4996(%r12), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 4996(%r12)
	testb	$31, %cl
	je	LBB50_11
## %bb.12:
	leaq	216(%rax), %r14
	movq	216(%rax), %rcx
	movq	%rax, %r13
	popq	%rbp
	jmpq	*%rcx                           ## TAILCALL
LBB50_4:
	movq	%r12, %rdi
	movq	%r13, %rsi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB50_5
## %bb.6:
	addq	%r13, %rbx
	movzbl	209(%rax), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 209(%rax)
	andl	$3, %ecx
	movl	%ecx, %edx
	movq	168(%rax,%rdx,8), %rdx
	testq	%rdx, %rdx
	je	LBB50_9
## %bb.7:
	cmpq	%rax, (%rdx)
	jne	LBB50_9
## %bb.8:
	movq	$0, (%rdx)
LBB50_9:
	movq	%rax, (%rbx)
	movq	%rbx, 168(%rax,%rcx,8)
	jmp	LBB50_10
LBB50_11:
	popq	%rbp
	retq
LBB50_5:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteStoreByteImmOffsetImm
_XrExecuteStoreByteImmOffsetImm:        ## @XrExecuteStoreByteImmOffsetImm
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movq	%r13, %rdi
	movzbl	12(%r14), %eax
	movzbl	13(%r14), %r15d
	movl	4728(%r12,%rax,4), %ebx
	shll	$27, %r15d
	sarl	$27, %r15d
	addl	8(%r14), %ebx
	testb	$4, 4864(%r12)
	je	LBB51_1
## %bb.2:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(%rdi,%rax,8), %r13
	movq	8(%rdi,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB51_8
## %bb.3:
	testb	$2, %al
	je	LBB51_8
## %bb.4:
	movl	16(%r13), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB51_8
## %bb.5:
	movq	(%r13), %rcx
	testq	%rcx, %rcx
	je	LBB51_10
## %bb.6:
	andl	$4095, %ebx                     ## imm = 0xFFF
	movb	%r15b, (%rcx,%rbx)
LBB51_7:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	movq	%rdi, %r13
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB51_8:
	movq	%rdi, -16(%rbp)                 ## 8-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r13, %rdx
	movl	$1, %ecx
	callq	_XrTranslateDstream
	movq	-16(%rbp), %rdi                 ## 8-byte Reload
	testl	%eax, %eax
	je	LBB51_18
## %bb.9:
	movq	8(%r13), %rax
LBB51_10:
	movq	%rdi, %r13
	andl	$4095, %ebx                     ## imm = 0xFFF
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
	jmp	LBB51_11
LBB51_1:
	movq	%rdi, %r13
LBB51_11:
	movl	%r15d, -4(%rbp)
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	leaq	-4(%rbp), %rsi
	movl	$1, %edx
	movq	%r12, %rcx
	movq	%r12, %r15
	callq	*8(%r8,%rax,8)
	testl	%eax, %eax
	movq	%r13, %rdi
	je	LBB51_7
## %bb.12:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB51_13
## %bb.14:
	testb	%al, %al
	js	LBB51_16
## %bb.15:
	andl	$252, %eax
LBB51_17:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
LBB51_18:
	subq	%rdi, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB51_13:
	movq	%rdi, %r13
	movq	%r12, %rdi
	callq	_XrReset
	movq	%r15, %r12
	movq	%r13, %rdi
	jmp	LBB51_18
LBB51_16:
	andl	$248, %eax
	jmp	LBB51_17
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteAndi
_XrExecuteAndi:                         ## @XrExecuteAndi
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	movzbl	13(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	andl	8(%r14), %ecx
	movl	%ecx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteBle
_XrExecuteBle:                          ## @XrExecuteBle
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	cmpl	$0, 4728(%r12,%rax,4)
	jle	LBB53_2
## %bb.1:
	movl	$160, %ebx
	movl	$4, %eax
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	jne	LBB53_10
	jmp	LBB53_4
LBB53_2:
	movl	8(%r14), %eax
	movl	$152, %ebx
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	je	LBB53_4
LBB53_10:
	movzbl	208(%r13), %ecx
	addl	%ecx, 5008(%r12)
	movzbl	4996(%r12), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 4996(%r12)
	testb	$31, %cl
	je	LBB53_11
## %bb.12:
	leaq	216(%rax), %r14
	movq	216(%rax), %rcx
	movq	%rax, %r13
	popq	%rbp
	jmpq	*%rcx                           ## TAILCALL
LBB53_4:
	movq	%r12, %rdi
	movq	%r13, %rsi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB53_5
## %bb.6:
	addq	%r13, %rbx
	movzbl	209(%rax), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 209(%rax)
	andl	$3, %ecx
	movl	%ecx, %edx
	movq	168(%rax,%rdx,8), %rdx
	testq	%rdx, %rdx
	je	LBB53_9
## %bb.7:
	cmpq	%rax, (%rdx)
	jne	LBB53_9
## %bb.8:
	movq	$0, (%rdx)
LBB53_9:
	movq	%rax, (%rbx)
	movq	%rbx, 168(%rax,%rcx,8)
	jmp	LBB53_10
LBB53_11:
	popq	%rbp
	retq
LBB53_5:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteSltiSigned
_XrExecuteSltiSigned:                   ## @XrExecuteSltiSigned
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	movzbl	13(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	xorl	%edx, %edx
	cmpl	8(%r14), %ecx
	setl	%dl
	movl	%edx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteBgt
_XrExecuteBgt:                          ## @XrExecuteBgt
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	cmpl	$0, 4728(%r12,%rax,4)
	jle	LBB55_1
## %bb.2:
	movl	8(%r14), %eax
	movl	$152, %ebx
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	jne	LBB55_10
	jmp	LBB55_4
LBB55_1:
	movl	$160, %ebx
	movl	$4, %eax
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	je	LBB55_4
LBB55_10:
	movzbl	208(%r13), %ecx
	addl	%ecx, 5008(%r12)
	movzbl	4996(%r12), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 4996(%r12)
	testb	$31, %cl
	je	LBB55_11
## %bb.12:
	leaq	216(%rax), %r14
	movq	216(%rax), %rcx
	movq	%rax, %r13
	popq	%rbp
	jmpq	*%rcx                           ## TAILCALL
LBB55_4:
	movq	%r12, %rdi
	movq	%r13, %rsi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB55_5
## %bb.6:
	addq	%r13, %rbx
	movzbl	209(%rax), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 209(%rax)
	andl	$3, %ecx
	movl	%ecx, %edx
	movq	168(%rax,%rdx,8), %rdx
	testq	%rdx, %rdx
	je	LBB55_9
## %bb.7:
	cmpq	%rax, (%rdx)
	jne	LBB55_9
## %bb.8:
	movq	$0, (%rdx)
LBB55_9:
	movq	%rax, (%rbx)
	movq	%rbx, 168(%rax,%rcx,8)
	jmp	LBB55_10
LBB55_11:
	popq	%rbp
	retq
LBB55_5:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeRfe
_XrDecodeRfe:                           ## @XrDecodeRfe
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteRfe(%rip), %rax
	movq	%rax, (%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeHlt
_XrDecodeHlt:                           ## @XrDecodeHlt
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteHlt(%rip), %rax
	movq	%rax, (%rsi)
	xorl	%eax, %eax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeMtcr
_XrDecodeMtcr:                          ## @XrDecodeMtcr
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteMtcr(%rip), %rax
	movq	%rax, (%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 12(%rsi)
	shrl	$16, %edx
	andb	$31, %dl
	movb	%dl, 13(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeMfcr
_XrDecodeMfcr:                          ## @XrDecodeMfcr
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteMfcr(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	shrl	$16, %edx
	andb	$31, %dl
	movb	%dl, 13(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteRfe
_XrExecuteRfe:                          ## @XrExecuteRfe
	.cfi_startproc
## %bb.0:
	movb	$0, 5030(%r12)
	movl	4864(%r12), %eax
	testb	$1, %al
	jne	LBB60_4
## %bb.1:
	movl	$4888, %ecx                     ## imm = 0x1318
	testb	$8, %al
	jne	LBB60_2
LBB60_3:
	movl	(%r12,%rcx), %ecx
	movl	%ecx, 4992(%r12)
	movl	%eax, %ecx
	andl	$-268435456, %ecx               ## imm = 0xF0000000
	shrl	$8, %eax
	movzwl	%ax, %eax
	orl	%ecx, %eax
	movl	%eax, 4864(%r12)
	movzbl	208(%r13), %eax
	addl	%eax, 5008(%r12)
	jmp	_XrCheckConditions              ## TAILCALL
LBB60_4:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$8, %esi
	callq	_XrBasicException
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
LBB60_2:
	movl	$4904, %ecx                     ## imm = 0x1328
	jmp	LBB60_3
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteHlt
_XrExecuteHlt:                          ## @XrExecuteHlt
	.cfi_startproc
## %bb.0:
	testb	$1, 4864(%r12)
	jne	LBB61_1
## %bb.2:
	movb	$1, 5033(%r12)
	addl	$4, 4992(%r12)
	movzbl	208(%r13), %eax
	addl	%eax, 5008(%r12)
	jmp	_XrCheckConditions              ## TAILCALL
LBB61_1:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$8, %esi
	callq	_XrBasicException
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteMtcr
_XrExecuteMtcr:                         ## @XrExecuteMtcr
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	testb	$1, 4864(%r12)
	jne	LBB62_302
## %bb.1:
	movb	$64, 5028(%r12)
	movzbl	12(%r14), %eax
	movzbl	13(%r14), %ecx
	leal	-16(%rcx), %edx
	cmpl	$12, %edx
	ja	LBB62_15
## %bb.2:
	leaq	LJTI62_0(%rip), %rsi
	movslq	(%rsi,%rdx,4), %rdx
	addq	%rsi, %rdx
	jmpq	*%rdx
LBB62_3:
	movl	4932(%r12), %ecx
	movl	4936(%r12), %edx
	shlq	$32, %rcx
	movl	4728(%r12,%rax,4), %eax
	orq	%rcx, %rax
	movq	%rax, (%r12,%rdx,8)
	leal	1(%rdx), %eax
	cmpl	$32, %eax
	movl	$4, %ecx
	cmovnel	%eax, %ecx
	movl	%ecx, 4936(%r12)
	jmp	LBB62_110
LBB62_4:
	movl	4728(%r12,%rax,4), %eax
	movl	%eax, 4932(%r12)
	movl	$-1, 4708(%r12)
	jmp	LBB62_110
LBB62_5:
	leaq	592(%r12), %rax
	movq	592(%r12), %rcx
	jmp	LBB62_7
	.p2align	4
LBB62_6:                                ##   in Loop: Header=BB62_7 Depth=1
	movq	568(%r12), %rsi
	movq	%rsi, -16(%rcx)
	movq	%rdx, 568(%r12)
	movq	(%rcx), %rcx
LBB62_7:                                ## =>This Inner Loop Header: Depth=1
	cmpq	%rax, %rcx
	je	LBB62_178
## %bb.8:                               ##   in Loop: Header=BB62_7 Depth=1
	movq	(%rcx), %rdx
	movq	8(%rcx), %rsi
	movq	%rdx, (%rsi)
	movq	%rsi, 8(%rdx)
	movq	-16(%rcx), %rdx
	movq	-8(%rcx), %rsi
	movq	%rdx, (%rsi)
	movq	%rsi, 8(%rdx)
	movq	-32(%rcx), %rdx
	movq	-24(%rcx), %rsi
	movq	%rdx, (%rsi)
	movq	%rsi, 8(%rdx)
	movq	16(%rcx), %rdx
	decl	36(%rdx)
	je	LBB62_10
## %bb.9:                               ##   in Loop: Header=BB62_7 Depth=1
	leaq	-128(%rcx), %rdx
	cmpb	$0, 211(%rdx)
	jne	LBB62_11
	jmp	LBB62_6
	.p2align	4
LBB62_10:                               ##   in Loop: Header=BB62_7 Depth=1
	movq	(%rdx), %rsi
	movq	8(%rdx), %rdi
	movq	%rsi, (%rdi)
	movq	%rdi, 8(%rsi)
	movq	584(%r12), %rsi
	movq	%rsi, (%rdx)
	movq	%rdx, 584(%r12)
	leaq	-128(%rcx), %rdx
	cmpb	$0, 211(%rdx)
	je	LBB62_6
LBB62_11:                               ##   in Loop: Header=BB62_7 Depth=1
	movq	152(%rdx), %rsi
	movq	576(%r12), %rdi
	movq	%rdi, (%rsi)
	movq	%rsi, 576(%r12)
	jmp	LBB62_6
LBB62_13:
	movl	4964(%r12), %ecx
	movl	4968(%r12), %edx
	shlq	$32, %rcx
	movl	4728(%r12,%rax,4), %eax
	orq	%rcx, %rax
	movq	%rax, 256(%r12,%rdx,8)
	leal	1(%rdx), %eax
	cmpl	$32, %eax
	movl	$4, %ecx
	cmovnel	%eax, %ecx
	movl	%ecx, 4968(%r12)
	jmp	LBB62_110
LBB62_14:
	movl	4728(%r12,%rax,4), %eax
	movl	%eax, 4964(%r12)
	movl	$-1, 4712(%r12)
	jmp	LBB62_110
LBB62_15:
	movl	4728(%r12,%rax,4), %eax
	movl	%eax, 4864(%r12,%rcx,4)
	jmp	LBB62_110
LBB62_16:
	movl	4728(%r12,%rax,4), %eax
	andl	$31, %eax
	movl	%eax, 4936(%r12)
	jmp	LBB62_110
LBB62_17:
	movl	4728(%r12,%rax,4), %ecx
	movl	%ecx, %edx
	andl	$3, %edx
	leaq	LJTI62_2(%rip), %rsi
	movslq	(%rsi,%rdx,4), %rdx
	addq	%rsi, %rdx
	jmpq	*%rdx
LBB62_18:
	movabsq	$-4503599627370496, %rdx        ## imm = 0xFFF0000000000000
	movabsq	$4503595332403200, %rsi         ## imm = 0xFFFFF00000000
	shrl	$12, %ecx
	movq	%rcx, %rdi
	shlq	$32, %rdi
	movq	(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_19
## %bb.237:
	movq	%rdx, (%r12)
	movq	8(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_238
LBB62_20:
	movq	16(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_21
LBB62_239:
	movq	%rdx, 16(%r12)
	movq	24(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_240
LBB62_22:
	movq	32(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_23
LBB62_241:
	movq	%rdx, 32(%r12)
	movq	40(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_242
LBB62_24:
	movq	48(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_25
LBB62_243:
	movq	%rdx, 48(%r12)
	movq	56(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_244
LBB62_26:
	movq	64(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_27
LBB62_245:
	movq	%rdx, 64(%r12)
	movq	72(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_246
LBB62_28:
	movq	80(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_29
LBB62_247:
	movq	%rdx, 80(%r12)
	movq	88(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_248
LBB62_30:
	movq	96(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_31
LBB62_249:
	movq	%rdx, 96(%r12)
	movq	104(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_250
LBB62_32:
	movq	112(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_33
LBB62_251:
	movq	%rdx, 112(%r12)
	movq	120(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_252
LBB62_34:
	movq	128(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_35
LBB62_253:
	movq	%rdx, 128(%r12)
	movq	136(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_254
LBB62_36:
	movq	144(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_37
LBB62_255:
	movq	%rdx, 144(%r12)
	movq	152(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_256
LBB62_38:
	movq	160(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_39
LBB62_257:
	movq	%rdx, 160(%r12)
	movq	168(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_258
LBB62_40:
	movq	176(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_41
LBB62_259:
	movq	%rdx, 176(%r12)
	movq	184(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_260
LBB62_42:
	movq	192(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_43
LBB62_261:
	movq	%rdx, 192(%r12)
	movq	200(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_262
LBB62_44:
	movq	208(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_45
LBB62_263:
	movq	%rdx, 208(%r12)
	movq	216(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_264
LBB62_46:
	movq	224(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_47
LBB62_265:
	movq	%rdx, 224(%r12)
	movq	232(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_266
LBB62_48:
	movq	240(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_49
LBB62_267:
	movq	%rdx, 240(%r12)
	andq	248(%r12), %rsi
	cmpq	%rdi, %rsi
	je	LBB62_268
LBB62_50:
	cmpl	%ecx, 4708(%r12)
	jne	LBB62_52
LBB62_51:
	movl	$-1, 4708(%r12)
LBB62_52:
	addl	$4, 4992(%r12)
	movl	4728(%r12,%rax,4), %edx
	movl	%edx, %eax
	andl	$-4096, %eax                    ## imm = 0xF000
	shrl	$8, %edx
	andl	$496, %edx                      ## imm = 0x1F0
	leaq	(%r12,%rdx), %rcx
	addq	$5040, %rcx                     ## imm = 0x13B0
	movq	5040(%r12,%rdx), %rdx
	jmp	LBB62_55
LBB62_53:                               ##   in Loop: Header=BB62_55 Depth=1
	movq	(%rdx), %rdx
LBB62_54:                               ##   in Loop: Header=BB62_55 Depth=1
	cmpl	%eax, %esi
	je	LBB62_179
LBB62_55:                               ## =>This Loop Header: Depth=1
                                        ##     Child Loop BB62_59 Depth 2
	cmpq	%rcx, %rdx
	je	LBB62_179
## %bb.56:                              ##   in Loop: Header=BB62_55 Depth=1
	movl	32(%rdx), %esi
	cmpl	%eax, %esi
	jne	LBB62_53
## %bb.57:                              ##   in Loop: Header=BB62_55 Depth=1
	leaq	16(%rdx), %rdi
	movq	16(%rdx), %r8
	cmpq	%rdi, %r8
	jne	LBB62_59
	jmp	LBB62_179
	.p2align	4
LBB62_58:                               ##   in Loop: Header=BB62_59 Depth=2
	movq	568(%r12), %r10
	movq	%r10, 16(%r8)
	movq	%r9, 568(%r12)
	movq	(%r8), %r8
	cmpq	%rdi, %r8
	je	LBB62_54
LBB62_59:                               ##   Parent Loop BB62_55 Depth=1
                                        ## =>  This Inner Loop Header: Depth=2
	leaq	-96(%r8), %r9
	movq	72(%r8), %r10
	testq	%r10, %r10
	je	LBB62_61
## %bb.60:                              ##   in Loop: Header=BB62_59 Depth=2
	cmpq	%r9, (%r10)
	je	LBB62_64
LBB62_61:                               ##   in Loop: Header=BB62_59 Depth=2
	movq	176(%r9), %r10
	testq	%r10, %r10
	je	LBB62_65
LBB62_62:                               ##   in Loop: Header=BB62_59 Depth=2
	cmpq	%r9, (%r10)
	jne	LBB62_65
## %bb.63:                              ##   in Loop: Header=BB62_59 Depth=2
	movq	$0, (%r10)
	jmp	LBB62_65
	.p2align	4
LBB62_64:                               ##   in Loop: Header=BB62_59 Depth=2
	movq	$0, (%r10)
	movq	176(%r9), %r10
	testq	%r10, %r10
	jne	LBB62_62
	.p2align	4
LBB62_65:                               ##   in Loop: Header=BB62_59 Depth=2
	movq	184(%r9), %r10
	testq	%r10, %r10
	je	LBB62_67
## %bb.66:                              ##   in Loop: Header=BB62_59 Depth=2
	cmpq	%r9, (%r10)
	je	LBB62_70
LBB62_67:                               ##   in Loop: Header=BB62_59 Depth=2
	movq	192(%r9), %r10
	testq	%r10, %r10
	je	LBB62_71
LBB62_68:                               ##   in Loop: Header=BB62_59 Depth=2
	cmpq	%r9, (%r10)
	jne	LBB62_71
## %bb.69:                              ##   in Loop: Header=BB62_59 Depth=2
	movq	$0, (%r10)
	jmp	LBB62_71
	.p2align	4
LBB62_70:                               ##   in Loop: Header=BB62_59 Depth=2
	movq	$0, (%r10)
	movq	192(%r9), %r10
	testq	%r10, %r10
	jne	LBB62_68
	.p2align	4
LBB62_71:                               ##   in Loop: Header=BB62_59 Depth=2
	movq	128(%r9), %r10
	movq	136(%r9), %r11
	movq	%r10, (%r11)
	movq	%r11, 8(%r10)
	movq	112(%r9), %r10
	movq	120(%r9), %r11
	movq	%r10, (%r11)
	movq	%r11, 8(%r10)
	movq	96(%r9), %r10
	movq	104(%r9), %r11
	movq	%r10, (%r11)
	movq	%r11, 8(%r10)
	movq	144(%r9), %r10
	decl	36(%r10)
	je	LBB62_73
## %bb.72:                              ##   in Loop: Header=BB62_59 Depth=2
	cmpb	$0, 211(%r9)
	jne	LBB62_74
	jmp	LBB62_58
	.p2align	4
LBB62_73:                               ##   in Loop: Header=BB62_59 Depth=2
	movq	(%r10), %r11
	movq	8(%r10), %rbx
	movq	%r11, (%rbx)
	movq	%rbx, 8(%r11)
	movq	584(%r12), %r11
	movq	%r11, (%r10)
	movq	%r10, 584(%r12)
	cmpb	$0, 211(%r9)
	je	LBB62_58
LBB62_74:                               ##   in Loop: Header=BB62_59 Depth=2
	movq	152(%r9), %r10
	movq	576(%r12), %r11
	movq	%r11, (%r10)
	movq	%r10, 576(%r12)
	jmp	LBB62_58
LBB62_75:
	movl	4728(%r12,%rax,4), %eax
	movl	%eax, %ecx
	andl	$3, %ecx
	leaq	LJTI62_1(%rip), %rdx
	movslq	(%rdx,%rcx,4), %rcx
	addq	%rdx, %rcx
	jmpq	*%rcx
LBB62_76:
	movabsq	$-4503599627370496, %rcx        ## imm = 0xFFF0000000000000
	movabsq	$4503595332403200, %rdx         ## imm = 0xFFFFF00000000
	shrl	$12, %eax
	movq	%rax, %rsi
	shlq	$32, %rsi
	movq	256(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_77
## %bb.269:
	movq	%rcx, 256(%r12)
	movq	264(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_270
LBB62_78:
	movq	272(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_79
LBB62_271:
	movq	%rcx, 272(%r12)
	movq	280(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_272
LBB62_80:
	movq	288(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_81
LBB62_273:
	movq	%rcx, 288(%r12)
	movq	296(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_274
LBB62_82:
	movq	304(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_83
LBB62_275:
	movq	%rcx, 304(%r12)
	movq	312(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_276
LBB62_84:
	movq	320(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_85
LBB62_277:
	movq	%rcx, 320(%r12)
	movq	328(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_278
LBB62_86:
	movq	336(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_87
LBB62_279:
	movq	%rcx, 336(%r12)
	movq	344(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_280
LBB62_88:
	movq	352(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_89
LBB62_281:
	movq	%rcx, 352(%r12)
	movq	360(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_282
LBB62_90:
	movq	368(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_91
LBB62_283:
	movq	%rcx, 368(%r12)
	movq	376(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_284
LBB62_92:
	movq	384(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_93
LBB62_285:
	movq	%rcx, 384(%r12)
	movq	392(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_286
LBB62_94:
	movq	400(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_95
LBB62_287:
	movq	%rcx, 400(%r12)
	movq	408(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_288
LBB62_96:
	movq	416(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_97
LBB62_289:
	movq	%rcx, 416(%r12)
	movq	424(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_290
LBB62_98:
	movq	432(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_99
LBB62_291:
	movq	%rcx, 432(%r12)
	movq	440(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_292
LBB62_100:
	movq	448(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_101
LBB62_293:
	movq	%rcx, 448(%r12)
	movq	456(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_294
LBB62_102:
	movq	464(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_103
LBB62_295:
	movq	%rcx, 464(%r12)
	movq	472(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_296
LBB62_104:
	movq	480(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_105
LBB62_297:
	movq	%rcx, 480(%r12)
	movq	488(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_298
LBB62_106:
	movq	496(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_107
LBB62_299:
	movq	%rcx, 496(%r12)
	andq	504(%r12), %rdx
	cmpq	%rsi, %rdx
	je	LBB62_300
LBB62_108:
	cmpl	%eax, 4712(%r12)
	jne	LBB62_110
	jmp	LBB62_301
LBB62_109:
	movl	4728(%r12,%rax,4), %eax
	andl	$31, %eax
	movl	%eax, 4968(%r12)
LBB62_110:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB62_111:
	movabsq	$-4503599627370496, %rax        ## imm = 0xFFF0000000000000
	testb	$16, 32(%r12)
	jne	LBB62_112
## %bb.181:
	movq	%rax, 32(%r12)
	testb	$16, 40(%r12)
	je	LBB62_182
LBB62_113:
	testb	$16, 48(%r12)
	jne	LBB62_114
LBB62_183:
	movq	%rax, 48(%r12)
	testb	$16, 56(%r12)
	je	LBB62_184
LBB62_115:
	testb	$16, 64(%r12)
	jne	LBB62_116
LBB62_185:
	movq	%rax, 64(%r12)
	testb	$16, 72(%r12)
	je	LBB62_186
LBB62_117:
	testb	$16, 80(%r12)
	jne	LBB62_118
LBB62_187:
	movq	%rax, 80(%r12)
	testb	$16, 88(%r12)
	je	LBB62_188
LBB62_119:
	testb	$16, 96(%r12)
	jne	LBB62_120
LBB62_189:
	movq	%rax, 96(%r12)
	testb	$16, 104(%r12)
	je	LBB62_190
LBB62_121:
	testb	$16, 112(%r12)
	jne	LBB62_122
LBB62_191:
	movq	%rax, 112(%r12)
	testb	$16, 120(%r12)
	je	LBB62_192
LBB62_123:
	testb	$16, 128(%r12)
	jne	LBB62_124
LBB62_193:
	movq	%rax, 128(%r12)
	testb	$16, 136(%r12)
	je	LBB62_194
LBB62_125:
	testb	$16, 144(%r12)
	jne	LBB62_126
LBB62_195:
	movq	%rax, 144(%r12)
	testb	$16, 152(%r12)
	je	LBB62_196
LBB62_127:
	testb	$16, 160(%r12)
	jne	LBB62_128
LBB62_197:
	movq	%rax, 160(%r12)
	testb	$16, 168(%r12)
	je	LBB62_198
LBB62_129:
	testb	$16, 176(%r12)
	jne	LBB62_130
LBB62_199:
	movq	%rax, 176(%r12)
	testb	$16, 184(%r12)
	je	LBB62_200
LBB62_131:
	testb	$16, 192(%r12)
	jne	LBB62_132
LBB62_201:
	movq	%rax, 192(%r12)
	testb	$16, 200(%r12)
	je	LBB62_202
LBB62_133:
	testb	$16, 208(%r12)
	jne	LBB62_134
LBB62_203:
	movq	%rax, 208(%r12)
	testb	$16, 216(%r12)
	je	LBB62_204
LBB62_135:
	testb	$16, 224(%r12)
	jne	LBB62_136
LBB62_205:
	movq	%rax, 224(%r12)
	testb	$16, 232(%r12)
	je	LBB62_206
LBB62_137:
	testb	$16, 240(%r12)
	jne	LBB62_138
LBB62_207:
	movq	%rax, 240(%r12)
	testb	$16, 248(%r12)
	jne	LBB62_171
	jmp	LBB62_208
LBB62_112:
	testb	$16, 40(%r12)
	jne	LBB62_113
LBB62_182:
	movq	%rax, 40(%r12)
	testb	$16, 48(%r12)
	je	LBB62_183
LBB62_114:
	testb	$16, 56(%r12)
	jne	LBB62_115
LBB62_184:
	movq	%rax, 56(%r12)
	testb	$16, 64(%r12)
	je	LBB62_185
LBB62_116:
	testb	$16, 72(%r12)
	jne	LBB62_117
LBB62_186:
	movq	%rax, 72(%r12)
	testb	$16, 80(%r12)
	je	LBB62_187
LBB62_118:
	testb	$16, 88(%r12)
	jne	LBB62_119
LBB62_188:
	movq	%rax, 88(%r12)
	testb	$16, 96(%r12)
	je	LBB62_189
LBB62_120:
	testb	$16, 104(%r12)
	jne	LBB62_121
LBB62_190:
	movq	%rax, 104(%r12)
	testb	$16, 112(%r12)
	je	LBB62_191
LBB62_122:
	testb	$16, 120(%r12)
	jne	LBB62_123
LBB62_192:
	movq	%rax, 120(%r12)
	testb	$16, 128(%r12)
	je	LBB62_193
LBB62_124:
	testb	$16, 136(%r12)
	jne	LBB62_125
LBB62_194:
	movq	%rax, 136(%r12)
	testb	$16, 144(%r12)
	je	LBB62_195
LBB62_126:
	testb	$16, 152(%r12)
	jne	LBB62_127
LBB62_196:
	movq	%rax, 152(%r12)
	testb	$16, 160(%r12)
	je	LBB62_197
LBB62_128:
	testb	$16, 168(%r12)
	jne	LBB62_129
LBB62_198:
	movq	%rax, 168(%r12)
	testb	$16, 176(%r12)
	je	LBB62_199
LBB62_130:
	testb	$16, 184(%r12)
	jne	LBB62_131
LBB62_200:
	movq	%rax, 184(%r12)
	testb	$16, 192(%r12)
	je	LBB62_201
LBB62_132:
	testb	$16, 200(%r12)
	jne	LBB62_133
LBB62_202:
	movq	%rax, 200(%r12)
	testb	$16, 208(%r12)
	je	LBB62_203
LBB62_134:
	testb	$16, 216(%r12)
	jne	LBB62_135
LBB62_204:
	movq	%rax, 216(%r12)
	testb	$16, 224(%r12)
	je	LBB62_205
LBB62_136:
	testb	$16, 232(%r12)
	jne	LBB62_137
LBB62_206:
	movq	%rax, 232(%r12)
	testb	$16, 240(%r12)
	je	LBB62_207
LBB62_138:
	testb	$16, 248(%r12)
	jne	LBB62_171
LBB62_208:
	movq	%rax, 248(%r12)
	jmp	LBB62_171
LBB62_139:
	movabsq	$-4503599627370496, %rax        ## imm = 0xFFF0000000000000
	testb	$16, 288(%r12)
	jne	LBB62_140
## %bb.209:
	movq	%rax, 288(%r12)
	testb	$16, 296(%r12)
	je	LBB62_210
LBB62_141:
	testb	$16, 304(%r12)
	jne	LBB62_142
LBB62_211:
	movq	%rax, 304(%r12)
	testb	$16, 312(%r12)
	je	LBB62_212
LBB62_143:
	testb	$16, 320(%r12)
	jne	LBB62_144
LBB62_213:
	movq	%rax, 320(%r12)
	testb	$16, 328(%r12)
	je	LBB62_214
LBB62_145:
	testb	$16, 336(%r12)
	jne	LBB62_146
LBB62_215:
	movq	%rax, 336(%r12)
	testb	$16, 344(%r12)
	je	LBB62_216
LBB62_147:
	testb	$16, 352(%r12)
	jne	LBB62_148
LBB62_217:
	movq	%rax, 352(%r12)
	testb	$16, 360(%r12)
	je	LBB62_218
LBB62_149:
	testb	$16, 368(%r12)
	jne	LBB62_150
LBB62_219:
	movq	%rax, 368(%r12)
	testb	$16, 376(%r12)
	je	LBB62_220
LBB62_151:
	testb	$16, 384(%r12)
	jne	LBB62_152
LBB62_221:
	movq	%rax, 384(%r12)
	testb	$16, 392(%r12)
	je	LBB62_222
LBB62_153:
	testb	$16, 400(%r12)
	jne	LBB62_154
LBB62_223:
	movq	%rax, 400(%r12)
	testb	$16, 408(%r12)
	je	LBB62_224
LBB62_155:
	testb	$16, 416(%r12)
	jne	LBB62_156
LBB62_225:
	movq	%rax, 416(%r12)
	testb	$16, 424(%r12)
	je	LBB62_226
LBB62_157:
	testb	$16, 432(%r12)
	jne	LBB62_158
LBB62_227:
	movq	%rax, 432(%r12)
	testb	$16, 440(%r12)
	je	LBB62_228
LBB62_159:
	testb	$16, 448(%r12)
	jne	LBB62_160
LBB62_229:
	movq	%rax, 448(%r12)
	testb	$16, 456(%r12)
	je	LBB62_230
LBB62_161:
	testb	$16, 464(%r12)
	jne	LBB62_162
LBB62_231:
	movq	%rax, 464(%r12)
	testb	$16, 472(%r12)
	je	LBB62_232
LBB62_163:
	testb	$16, 480(%r12)
	jne	LBB62_164
LBB62_233:
	movq	%rax, 480(%r12)
	testb	$16, 488(%r12)
	je	LBB62_234
LBB62_165:
	testb	$16, 496(%r12)
	jne	LBB62_166
LBB62_235:
	movq	%rax, 496(%r12)
	testb	$16, 504(%r12)
	jne	LBB62_301
	jmp	LBB62_236
LBB62_140:
	testb	$16, 296(%r12)
	jne	LBB62_141
LBB62_210:
	movq	%rax, 296(%r12)
	testb	$16, 304(%r12)
	je	LBB62_211
LBB62_142:
	testb	$16, 312(%r12)
	jne	LBB62_143
LBB62_212:
	movq	%rax, 312(%r12)
	testb	$16, 320(%r12)
	je	LBB62_213
LBB62_144:
	testb	$16, 328(%r12)
	jne	LBB62_145
LBB62_214:
	movq	%rax, 328(%r12)
	testb	$16, 336(%r12)
	je	LBB62_215
LBB62_146:
	testb	$16, 344(%r12)
	jne	LBB62_147
LBB62_216:
	movq	%rax, 344(%r12)
	testb	$16, 352(%r12)
	je	LBB62_217
LBB62_148:
	testb	$16, 360(%r12)
	jne	LBB62_149
LBB62_218:
	movq	%rax, 360(%r12)
	testb	$16, 368(%r12)
	je	LBB62_219
LBB62_150:
	testb	$16, 376(%r12)
	jne	LBB62_151
LBB62_220:
	movq	%rax, 376(%r12)
	testb	$16, 384(%r12)
	je	LBB62_221
LBB62_152:
	testb	$16, 392(%r12)
	jne	LBB62_153
LBB62_222:
	movq	%rax, 392(%r12)
	testb	$16, 400(%r12)
	je	LBB62_223
LBB62_154:
	testb	$16, 408(%r12)
	jne	LBB62_155
LBB62_224:
	movq	%rax, 408(%r12)
	testb	$16, 416(%r12)
	je	LBB62_225
LBB62_156:
	testb	$16, 424(%r12)
	jne	LBB62_157
LBB62_226:
	movq	%rax, 424(%r12)
	testb	$16, 432(%r12)
	je	LBB62_227
LBB62_158:
	testb	$16, 440(%r12)
	jne	LBB62_159
LBB62_228:
	movq	%rax, 440(%r12)
	testb	$16, 448(%r12)
	je	LBB62_229
LBB62_160:
	testb	$16, 456(%r12)
	jne	LBB62_161
LBB62_230:
	movq	%rax, 456(%r12)
	testb	$16, 464(%r12)
	je	LBB62_231
LBB62_162:
	testb	$16, 472(%r12)
	jne	LBB62_163
LBB62_232:
	movq	%rax, 472(%r12)
	testb	$16, 480(%r12)
	je	LBB62_233
LBB62_164:
	testb	$16, 488(%r12)
	jne	LBB62_165
LBB62_234:
	movq	%rax, 488(%r12)
	testb	$16, 496(%r12)
	je	LBB62_235
LBB62_166:
	testb	$16, 504(%r12)
	jne	LBB62_301
LBB62_236:
	movq	%rax, 504(%r12)
	movl	$-1, 4712(%r12)
	jmp	LBB62_110
LBB62_167:
	leaq	L_.memset_pattern.4(%rip), %rsi
	movl	$256, %edx                      ## imm = 0x100
	movq	%r12, %rdi
	jmp	LBB62_170
LBB62_168:
	leaq	256(%r12), %rdi
	leaq	L_.memset_pattern.4(%rip), %rsi
	movl	$256, %edx                      ## imm = 0x100
	callq	_memset_pattern16
	movl	$-1, 4712(%r12)
	jmp	LBB62_110
LBB62_169:
	leaq	32(%r12), %rdi
	leaq	L_.memset_pattern.4(%rip), %rsi
	movl	$224, %edx
LBB62_170:
	callq	_memset_pattern16
LBB62_171:
	movl	$-1, 4708(%r12)
	leaq	592(%r12), %rax
	movq	592(%r12), %rcx
	jmp	LBB62_173
	.p2align	4
LBB62_172:                              ##   in Loop: Header=BB62_173 Depth=1
	movq	568(%r12), %rsi
	movq	%rsi, -16(%rcx)
	movq	%rdx, 568(%r12)
	movq	(%rcx), %rcx
LBB62_173:                              ## =>This Inner Loop Header: Depth=1
	cmpq	%rax, %rcx
	je	LBB62_178
## %bb.174:                             ##   in Loop: Header=BB62_173 Depth=1
	movq	(%rcx), %rdx
	movq	8(%rcx), %rsi
	movq	%rdx, (%rsi)
	movq	%rsi, 8(%rdx)
	movq	-16(%rcx), %rdx
	movq	-8(%rcx), %rsi
	movq	%rdx, (%rsi)
	movq	%rsi, 8(%rdx)
	movq	-32(%rcx), %rdx
	movq	-24(%rcx), %rsi
	movq	%rdx, (%rsi)
	movq	%rsi, 8(%rdx)
	movq	16(%rcx), %rdx
	decl	36(%rdx)
	je	LBB62_176
## %bb.175:                             ##   in Loop: Header=BB62_173 Depth=1
	leaq	-128(%rcx), %rdx
	cmpb	$0, 211(%rdx)
	jne	LBB62_177
	jmp	LBB62_172
	.p2align	4
LBB62_176:                              ##   in Loop: Header=BB62_173 Depth=1
	movq	(%rdx), %rsi
	movq	8(%rdx), %rdi
	movq	%rsi, (%rdi)
	movq	%rdi, 8(%rsi)
	movq	584(%r12), %rsi
	movq	%rsi, (%rdx)
	movq	%rdx, 584(%r12)
	leaq	-128(%rcx), %rdx
	cmpb	$0, 211(%rdx)
	je	LBB62_172
LBB62_177:                              ##   in Loop: Header=BB62_173 Depth=1
	movq	152(%rdx), %rsi
	movq	576(%r12), %rdi
	movq	%rdi, (%rsi)
	movq	%rsi, 576(%r12)
	jmp	LBB62_172
LBB62_178:
	addl	$4, 4992(%r12)
LBB62_179:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
LBB62_19:
	movq	8(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_20
LBB62_238:
	movq	%rdx, 8(%r12)
	movq	16(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_239
LBB62_21:
	movq	24(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_22
LBB62_240:
	movq	%rdx, 24(%r12)
	movq	32(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_241
LBB62_23:
	movq	40(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_24
LBB62_242:
	movq	%rdx, 40(%r12)
	movq	48(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_243
LBB62_25:
	movq	56(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_26
LBB62_244:
	movq	%rdx, 56(%r12)
	movq	64(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_245
LBB62_27:
	movq	72(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_28
LBB62_246:
	movq	%rdx, 72(%r12)
	movq	80(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_247
LBB62_29:
	movq	88(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_30
LBB62_248:
	movq	%rdx, 88(%r12)
	movq	96(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_249
LBB62_31:
	movq	104(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_32
LBB62_250:
	movq	%rdx, 104(%r12)
	movq	112(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_251
LBB62_33:
	movq	120(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_34
LBB62_252:
	movq	%rdx, 120(%r12)
	movq	128(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_253
LBB62_35:
	movq	136(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_36
LBB62_254:
	movq	%rdx, 136(%r12)
	movq	144(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_255
LBB62_37:
	movq	152(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_38
LBB62_256:
	movq	%rdx, 152(%r12)
	movq	160(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_257
LBB62_39:
	movq	168(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_40
LBB62_258:
	movq	%rdx, 168(%r12)
	movq	176(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_259
LBB62_41:
	movq	184(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_42
LBB62_260:
	movq	%rdx, 184(%r12)
	movq	192(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_261
LBB62_43:
	movq	200(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_44
LBB62_262:
	movq	%rdx, 200(%r12)
	movq	208(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_263
LBB62_45:
	movq	216(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_46
LBB62_264:
	movq	%rdx, 216(%r12)
	movq	224(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_265
LBB62_47:
	movq	232(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	jne	LBB62_48
LBB62_266:
	movq	%rdx, 232(%r12)
	movq	240(%r12), %r8
	andq	%rsi, %r8
	cmpq	%rdi, %r8
	je	LBB62_267
LBB62_49:
	andq	248(%r12), %rsi
	cmpq	%rdi, %rsi
	jne	LBB62_50
LBB62_268:
	movq	%rdx, 248(%r12)
	cmpl	%ecx, 4708(%r12)
	je	LBB62_51
	jmp	LBB62_52
LBB62_180:
	leaq	288(%r12), %rdi
	leaq	L_.memset_pattern.4(%rip), %rsi
	movl	$224, %edx
	callq	_memset_pattern16
	movl	$-1, 4712(%r12)
	jmp	LBB62_110
LBB62_77:
	movq	264(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_78
LBB62_270:
	movq	%rcx, 264(%r12)
	movq	272(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_271
LBB62_79:
	movq	280(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_80
LBB62_272:
	movq	%rcx, 280(%r12)
	movq	288(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_273
LBB62_81:
	movq	296(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_82
LBB62_274:
	movq	%rcx, 296(%r12)
	movq	304(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_275
LBB62_83:
	movq	312(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_84
LBB62_276:
	movq	%rcx, 312(%r12)
	movq	320(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_277
LBB62_85:
	movq	328(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_86
LBB62_278:
	movq	%rcx, 328(%r12)
	movq	336(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_279
LBB62_87:
	movq	344(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_88
LBB62_280:
	movq	%rcx, 344(%r12)
	movq	352(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_281
LBB62_89:
	movq	360(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_90
LBB62_282:
	movq	%rcx, 360(%r12)
	movq	368(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_283
LBB62_91:
	movq	376(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_92
LBB62_284:
	movq	%rcx, 376(%r12)
	movq	384(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_285
LBB62_93:
	movq	392(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_94
LBB62_286:
	movq	%rcx, 392(%r12)
	movq	400(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_287
LBB62_95:
	movq	408(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_96
LBB62_288:
	movq	%rcx, 408(%r12)
	movq	416(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_289
LBB62_97:
	movq	424(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_98
LBB62_290:
	movq	%rcx, 424(%r12)
	movq	432(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_291
LBB62_99:
	movq	440(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_100
LBB62_292:
	movq	%rcx, 440(%r12)
	movq	448(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_293
LBB62_101:
	movq	456(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_102
LBB62_294:
	movq	%rcx, 456(%r12)
	movq	464(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_295
LBB62_103:
	movq	472(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_104
LBB62_296:
	movq	%rcx, 472(%r12)
	movq	480(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_297
LBB62_105:
	movq	488(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	jne	LBB62_106
LBB62_298:
	movq	%rcx, 488(%r12)
	movq	496(%r12), %rdi
	andq	%rdx, %rdi
	cmpq	%rsi, %rdi
	je	LBB62_299
LBB62_107:
	andq	504(%r12), %rdx
	cmpq	%rsi, %rdx
	jne	LBB62_108
LBB62_300:
	movq	%rcx, 504(%r12)
	cmpl	%eax, 4712(%r12)
	jne	LBB62_110
LBB62_301:
	movl	$-1, 4712(%r12)
	jmp	LBB62_110
LBB62_302:
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$8, %esi
	callq	_XrBasicException
	jmp	LBB62_179
	.cfi_endproc
	.p2align	2
	.data_region jt32
.set L62_0_set_3, LBB62_3-LJTI62_0
.set L62_0_set_4, LBB62_4-LJTI62_0
.set L62_0_set_16, LBB62_16-LJTI62_0
.set L62_0_set_17, LBB62_17-LJTI62_0
.set L62_0_set_5, LBB62_5-LJTI62_0
.set L62_0_set_15, LBB62_15-LJTI62_0
.set L62_0_set_13, LBB62_13-LJTI62_0
.set L62_0_set_14, LBB62_14-LJTI62_0
.set L62_0_set_109, LBB62_109-LJTI62_0
.set L62_0_set_75, LBB62_75-LJTI62_0
.set L62_0_set_110, LBB62_110-LJTI62_0
LJTI62_0:
	.long	L62_0_set_3
	.long	L62_0_set_4
	.long	L62_0_set_16
	.long	L62_0_set_17
	.long	L62_0_set_5
	.long	L62_0_set_15
	.long	L62_0_set_15
	.long	L62_0_set_15
	.long	L62_0_set_13
	.long	L62_0_set_14
	.long	L62_0_set_109
	.long	L62_0_set_75
	.long	L62_0_set_110
.set L62_1_set_76, LBB62_76-LJTI62_1
.set L62_1_set_180, LBB62_180-LJTI62_1
.set L62_1_set_139, LBB62_139-LJTI62_1
.set L62_1_set_168, LBB62_168-LJTI62_1
LJTI62_1:
	.long	L62_1_set_76
	.long	L62_1_set_180
	.long	L62_1_set_139
	.long	L62_1_set_168
.set L62_2_set_18, LBB62_18-LJTI62_2
.set L62_2_set_169, LBB62_169-LJTI62_2
.set L62_2_set_111, LBB62_111-LJTI62_2
.set L62_2_set_167, LBB62_167-LJTI62_2
LJTI62_2:
	.long	L62_2_set_18
	.long	L62_2_set_169
	.long	L62_2_set_111
	.long	L62_2_set_167
	.end_data_region
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteMfcr
_XrExecuteMfcr:                         ## @XrExecuteMfcr
	.cfi_startproc
## %bb.0:
	movl	4864(%r12), %eax
	testb	$1, %al
	jne	LBB63_1
## %bb.8:
	movb	$64, 5028(%r12)
	movzbl	13(%r14), %eax
	movl	4864(%r12,%rax,4), %eax
	movzbl	12(%r14), %ecx
	movl	%eax, 4728(%r12,%rcx,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	jmpq	*%rax                           ## TAILCALL
LBB63_1:
	movl	4884(%r12), %ecx
	movl	4992(%r12), %edx
	movl	%edx, 4888(%r12)
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$-2147483648, %edx              ## imm = 0x80000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB63_2
## %bb.3:
	testb	%al, %al
	js	LBB63_5
## %bb.4:
	andl	$252, %eax
LBB63_6:
	orl	$2048, %ecx                     ## imm = 0x800
	movl	%ecx, 4992(%r12)
	andl	$-1879048448, %edx              ## imm = 0x8FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
LBB63_7:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	retq
LBB63_2:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%r12, %rdi
	callq	_XrReset
	popq	%rbp
	jmp	LBB63_7
LBB63_5:
	andl	$248, %eax
	jmp	LBB63_6
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteStoreLongImmOffsetReg
_XrExecuteStoreLongImmOffsetReg:        ## @XrExecuteStoreLongImmOffsetReg
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	12(%r14), %eax
	movl	4728(%r12,%rax,4), %ebx
	addl	8(%r14), %ebx
	testb	$3, %bl
	jne	LBB64_1
## %bb.2:
	movzbl	13(%r14), %eax
	movl	4728(%r12,%rax,4), %esi
	testb	$4, 4864(%r12)
	je	LBB64_12
## %bb.3:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %r15
	addq	%r13, %r15
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB64_9
## %bb.4:
	testb	$2, %al
	je	LBB64_9
## %bb.5:
	movl	16(%r15), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB64_9
## %bb.6:
	movq	(%r15), %rcx
	testq	%rcx, %rcx
	je	LBB64_11
## %bb.7:
	andl	$4092, %ebx                     ## imm = 0xFFC
	movl	%esi, (%rcx,%rbx)
LBB64_8:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB64_9:
	movl	%esi, -4(%rbp)                  ## 4-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r15, %rdx
	movl	$1, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB64_19
## %bb.10:
	movq	8(%r15), %rax
	movl	-4(%rbp), %esi                  ## 4-byte Reload
LBB64_11:
	andl	$4092, %ebx                     ## imm = 0xFFC
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
LBB64_12:
	movl	%esi, -8(%rbp)
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	leaq	-8(%rbp), %rsi
	movl	$4, %edx
	movq	%r12, %rcx
	callq	*8(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB64_8
## %bb.13:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB64_14
## %bb.15:
	testb	%al, %al
	js	LBB64_17
## %bb.16:
	andl	$252, %eax
LBB64_18:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	jmp	LBB64_19
LBB64_1:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$9, %esi
	callq	_XrBasicException
LBB64_19:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB64_14:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB64_19
LBB64_17:
	andl	$248, %eax
	jmp	LBB64_18
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteLoadLongImmOffset
_XrExecuteLoadLongImmOffset:            ## @XrExecuteLoadLongImmOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	13(%r14), %eax
	movl	4728(%r12,%rax,4), %ebx
	addl	8(%r14), %ebx
	testb	$3, %bl
	jne	LBB65_1
## %bb.2:
	movzbl	12(%r14), %eax
	leaq	(%r12,%rax,4), %rsi
	addq	$4728, %rsi                     ## imm = 0x1278
	testb	$4, 4864(%r12)
	je	LBB65_11
## %bb.3:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %r15
	addq	%r13, %r15
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB65_8
## %bb.4:
	movl	16(%r15), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB65_8
## %bb.5:
	movq	(%r15), %rcx
	testq	%rcx, %rcx
	je	LBB65_10
## %bb.6:
	andl	$4092, %ebx                     ## imm = 0xFFC
	movl	(%rcx,%rbx), %eax
	movl	%eax, (%rsi)
LBB65_7:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB65_8:
	movq	%rsi, -8(%rbp)                  ## 8-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r15, %rdx
	xorl	%ecx, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB65_18
## %bb.9:
	movq	8(%r15), %rax
	movq	-8(%rbp), %rsi                  ## 8-byte Reload
LBB65_10:
	andl	$4092, %ebx                     ## imm = 0xFFC
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
LBB65_11:
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	movl	$4, %edx
	movq	%r12, %rcx
	callq	*16(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB65_7
## %bb.12:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB65_13
## %bb.14:
	testb	%al, %al
	js	LBB65_16
## %bb.15:
	andl	$252, %eax
LBB65_17:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	jmp	LBB65_18
LBB65_1:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$9, %esi
	callq	_XrBasicException
LBB65_18:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB65_13:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB65_18
LBB65_16:
	andl	$248, %eax
	jmp	LBB65_17
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteSlti
_XrExecuteSlti:                         ## @XrExecuteSlti
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	movzbl	13(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	xorl	%edx, %edx
	cmpl	8(%r14), %ecx
	setb	%dl
	movl	%edx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteBlt
_XrExecuteBlt:                          ## @XrExecuteBlt
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	cmpl	$0, 4728(%r12,%rax,4)
	js	LBB67_2
## %bb.1:
	movl	$160, %ebx
	movl	$4, %eax
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	jne	LBB67_10
	jmp	LBB67_4
LBB67_2:
	movl	8(%r14), %eax
	movl	$152, %ebx
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	je	LBB67_4
LBB67_10:
	movzbl	208(%r13), %ecx
	addl	%ecx, 5008(%r12)
	movzbl	4996(%r12), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 4996(%r12)
	testb	$31, %cl
	je	LBB67_11
## %bb.12:
	leaq	216(%rax), %r14
	movq	216(%rax), %rcx
	movq	%rax, %r13
	popq	%rbp
	jmpq	*%rcx                           ## TAILCALL
LBB67_4:
	movq	%r12, %rdi
	movq	%r13, %rsi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB67_5
## %bb.6:
	addq	%r13, %rbx
	movzbl	209(%rax), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 209(%rax)
	andl	$3, %ecx
	movl	%ecx, %edx
	movq	168(%rax,%rdx,8), %rdx
	testq	%rdx, %rdx
	je	LBB67_9
## %bb.7:
	cmpq	%rax, (%rdx)
	jne	LBB67_9
## %bb.8:
	movq	$0, (%rdx)
LBB67_9:
	movq	%rax, (%rbx)
	movq	%rbx, 168(%rax,%rcx,8)
	jmp	LBB67_10
LBB67_11:
	popq	%rbp
	retq
LBB67_5:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeSys
_XrDecodeSys:                           ## @XrDecodeSys
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteSys(%rip), %rax
	movq	%rax, (%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeBrk
_XrDecodeBrk:                           ## @XrDecodeBrk
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteBrk(%rip), %rax
	movq	%rax, (%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeWmb
_XrDecodeWmb:                           ## @XrDecodeWmb
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteWmb(%rip), %rax
	movq	%rax, (%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeMb
_XrDecodeMb:                            ## @XrDecodeMb
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteWmb(%rip), %rax
	movq	%rax, (%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodePause
_XrDecodePause:                         ## @XrDecodePause
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecutePause(%rip), %rax
	movq	%rax, (%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeSC
_XrDecodeSC:                            ## @XrDecodeSC
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteSC(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %ecx
	movl	$32, %edi
	cmovel	%edi, %ecx
	movl	%edx, %r8d
	andl	$2031616, %r8d                  ## imm = 0x1F0000
	shrl	$16, %edx
	andl	$31, %edx
	orl	%eax, %r8d
	cmovel	%edi, %edx
	movb	%cl, 13(%rsi)
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeLL
_XrDecodeLL:                            ## @XrDecodeLL
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	leaq	_XrExecuteLL(%rip), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%eax, %ecx
	movzbl	%dl, %eax
	movl	$32, %ecx
	cmovnel	%eax, %ecx
	movb	%cl, 13(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeMod
_XrDecodeMod:                           ## @XrDecodeMod
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB75_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB75_2:
	leaq	_XrExecuteMod(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeDivSigned
_XrDecodeDivSigned:                     ## @XrDecodeDivSigned
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB76_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB76_2:
	leaq	_XrExecuteDivSigned(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeDiv
_XrDecodeDiv:                           ## @XrDecodeDiv
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB77_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB77_2:
	leaq	_XrExecuteDiv(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeMul
_XrDecodeMul:                           ## @XrDecodeMul
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB78_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB78_2:
	leaq	_XrExecuteMul(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteSys
_XrExecuteSys:                          ## @XrExecuteSys
	.cfi_startproc
## %bb.0:
	movl	4992(%r12), %eax
	addl	$4, %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$536870912, %edx                ## imm = 0x20000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB79_1
## %bb.2:
	testb	%al, %al
	js	LBB79_4
## %bb.3:
	andl	$252, %eax
LBB79_5:
	orl	$512, %ecx                      ## imm = 0x200
	movl	%ecx, 4992(%r12)
	andl	$805306112, %edx                ## imm = 0x2FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	movzbl	208(%r13), %eax
	addl	%eax, 5008(%r12)
	jmp	_XrCheckConditions              ## TAILCALL
LBB79_1:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%r12, %rdi
	callq	_XrReset
	popq	%rbp
	movzbl	208(%r13), %eax
	addl	%eax, 5008(%r12)
	jmp	_XrCheckConditions              ## TAILCALL
LBB79_4:
	andl	$248, %eax
	jmp	LBB79_5
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteBrk
_XrExecuteBrk:                          ## @XrExecuteBrk
	.cfi_startproc
## %bb.0:
	movl	4992(%r12), %eax
	addl	$4, %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1610612736, %edx               ## imm = 0x60000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB80_1
## %bb.2:
	testb	%al, %al
	js	LBB80_4
## %bb.3:
	andl	$252, %eax
LBB80_5:
	orl	$1536, %ecx                     ## imm = 0x600
	movl	%ecx, 4992(%r12)
	andl	$1879047936, %edx               ## imm = 0x6FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	movzbl	208(%r13), %eax
	addl	%eax, 5008(%r12)
	jmp	_XrCheckConditions              ## TAILCALL
LBB80_1:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%r12, %rdi
	callq	_XrReset
	popq	%rbp
	movzbl	208(%r13), %eax
	addl	%eax, 5008(%r12)
	jmp	_XrCheckConditions              ## TAILCALL
LBB80_4:
	andl	$248, %eax
	jmp	LBB80_5
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteWmb
_XrExecuteWmb:                          ## @XrExecuteWmb
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecutePause
_XrExecutePause:                        ## @XrExecutePause
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	5016(%r12), %eax
	leal	1(%rax), %ecx
	movl	%ecx, 5016(%r12)
	addl	$4, 4992(%r12)
	cmpl	$256, %eax                      ## imm = 0x100
	jb	LBB82_2
## %bb.1:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
LBB82_2:
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteSC
_XrExecuteSC:                           ## @XrExecuteSC
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	cmpb	$0, 5030(%r12)
	je	LBB83_1
## %bb.3:
	movzbl	13(%r14), %eax
	movl	4728(%r12,%rax,4), %ebx
	testb	$3, %bl
	jne	LBB83_4
## %bb.5:
	movl	8(%r14), %eax
	movl	4728(%r12,%rax,4), %esi
	testb	$4, 4864(%r12)
	je	LBB83_15
## %bb.6:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %r15
	addq	%r13, %r15
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB83_12
## %bb.7:
	testb	$2, %al
	je	LBB83_12
## %bb.8:
	movl	16(%r15), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB83_12
## %bb.9:
	movq	(%r15), %rcx
	testq	%rcx, %rcx
	je	LBB83_14
## %bb.10:
	andl	$4092, %ebx                     ## imm = 0xFFC
	movl	%esi, (%rcx,%rbx)
LBB83_11:
	movl	$1, %eax
LBB83_2:
	movzbl	12(%r14), %ecx
	movl	%eax, 4728(%r12,%rcx,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB83_12:
	movl	%esi, -4(%rbp)                  ## 4-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r15, %rdx
	movl	$1, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB83_22
## %bb.13:
	movq	8(%r15), %rax
	movl	-4(%rbp), %esi                  ## 4-byte Reload
LBB83_14:
	andl	$4092, %ebx                     ## imm = 0xFFC
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
LBB83_15:
	movl	%esi, -8(%rbp)
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	leaq	-8(%rbp), %rsi
	movl	$4, %edx
	movq	%r12, %rcx
	callq	*8(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB83_11
## %bb.16:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB83_17
## %bb.18:
	testb	%al, %al
	js	LBB83_20
## %bb.19:
	andl	$252, %eax
LBB83_21:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	jmp	LBB83_22
LBB83_1:
	xorl	%eax, %eax
	jmp	LBB83_2
LBB83_4:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$9, %esi
	callq	_XrBasicException
LBB83_22:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB83_17:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB83_22
LBB83_20:
	andl	$248, %eax
	jmp	LBB83_21
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteLL
_XrExecuteLL:                           ## @XrExecuteLL
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	13(%r14), %eax
	movl	4728(%r12,%rax,4), %ebx
	testb	$3, %bl
	jne	LBB84_1
## %bb.2:
	movzbl	12(%r14), %eax
	leaq	(%r12,%rax,4), %rsi
	addq	$4728, %rsi                     ## imm = 0x1278
	testb	$4, 4864(%r12)
	je	LBB84_11
## %bb.3:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %r15
	addq	%r13, %r15
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB84_8
## %bb.4:
	movl	16(%r15), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB84_8
## %bb.5:
	movq	(%r15), %rcx
	testq	%rcx, %rcx
	je	LBB84_10
## %bb.6:
	andl	$4092, %ebx                     ## imm = 0xFFC
	movl	(%rcx,%rbx), %eax
	movl	%eax, (%rsi)
LBB84_7:
	movb	$1, 5030(%r12)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB84_8:
	movq	%rsi, -8(%rbp)                  ## 8-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r15, %rdx
	xorl	%ecx, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB84_18
## %bb.9:
	movq	8(%r15), %rax
	movq	-8(%rbp), %rsi                  ## 8-byte Reload
LBB84_10:
	andl	$4092, %ebx                     ## imm = 0xFFC
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
LBB84_11:
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	movl	$4, %edx
	movq	%r12, %rcx
	callq	*16(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB84_7
## %bb.12:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB84_13
## %bb.14:
	testb	%al, %al
	js	LBB84_16
## %bb.15:
	andl	$252, %eax
LBB84_17:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	jmp	LBB84_18
LBB84_1:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$9, %esi
	callq	_XrBasicException
LBB84_18:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB84_13:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB84_18
LBB84_16:
	andl	$248, %eax
	jmp	LBB84_17
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteMod
_XrExecuteMod:                          ## @XrExecuteMod
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	8(%r14), %eax
	movl	4728(%r12,%rax,4), %ecx
	testl	%ecx, %ecx
	je	LBB85_1
## %bb.2:
	movzbl	13(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	xorl	%edx, %edx
	divl	%ecx
	jmp	LBB85_3
LBB85_1:
	xorl	%edx, %edx
LBB85_3:
	movzbl	12(%r14), %eax
	movl	%edx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteVirtualLsh
_XrExecuteVirtualLsh:                   ## @XrExecuteVirtualLsh
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	movzbl	13(%r14), %ecx
	shll	%cl, %eax
	movl	%eax, 4860(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteVirtualRsh
_XrExecuteVirtualRsh:                   ## @XrExecuteVirtualRsh
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	movzbl	13(%r14), %ecx
	shrl	%cl, %eax
	movl	%eax, 4860(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteVirtualAsh
_XrExecuteVirtualAsh:                   ## @XrExecuteVirtualAsh
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	movzbl	13(%r14), %ecx
	sarl	%cl, %eax
	movl	%eax, 4860(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteVirtualRor
_XrExecuteVirtualRor:                   ## @XrExecuteVirtualRor
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	movzbl	13(%r14), %ecx
	rorl	%cl, %eax
	movl	%eax, 4860(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteDivSigned
_XrExecuteDivSigned:                    ## @XrExecuteDivSigned
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	8(%r14), %eax
	movl	4728(%r12,%rax,4), %ecx
	testl	%ecx, %ecx
	je	LBB90_1
## %bb.2:
	movzbl	13(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	cltd
	idivl	%ecx
	jmp	LBB90_3
LBB90_1:
	xorl	%eax, %eax
LBB90_3:
	movzbl	12(%r14), %ecx
	movl	%eax, 4728(%r12,%rcx,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteDiv
_XrExecuteDiv:                          ## @XrExecuteDiv
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	8(%r14), %eax
	movl	4728(%r12,%rax,4), %ecx
	testl	%ecx, %ecx
	je	LBB91_1
## %bb.2:
	movzbl	13(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	xorl	%edx, %edx
	divl	%ecx
	jmp	LBB91_3
LBB91_1:
	xorl	%eax, %eax
LBB91_3:
	movzbl	12(%r14), %ecx
	movl	%eax, 4728(%r12,%rcx,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteMul
_XrExecuteMul:                          ## @XrExecuteMul
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	13(%r14), %eax
	movl	8(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	imull	4728(%r12,%rax,4), %ecx
	movzbl	12(%r14), %eax
	movl	%ecx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteStoreIntImmOffsetReg
_XrExecuteStoreIntImmOffsetReg:         ## @XrExecuteStoreIntImmOffsetReg
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	12(%r14), %eax
	movl	4728(%r12,%rax,4), %ebx
	addl	8(%r14), %ebx
	testb	$1, %bl
	jne	LBB93_1
## %bb.2:
	movzbl	13(%r14), %eax
	movl	4728(%r12,%rax,4), %esi
	testb	$4, 4864(%r12)
	je	LBB93_12
## %bb.3:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %r15
	addq	%r13, %r15
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB93_9
## %bb.4:
	testb	$2, %al
	je	LBB93_9
## %bb.5:
	movl	16(%r15), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB93_9
## %bb.6:
	movq	(%r15), %rcx
	testq	%rcx, %rcx
	je	LBB93_11
## %bb.7:
	andl	$4094, %ebx                     ## imm = 0xFFE
	movw	%si, (%rcx,%rbx)
LBB93_8:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB93_9:
	movl	%esi, -4(%rbp)                  ## 4-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r15, %rdx
	movl	$1, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB93_19
## %bb.10:
	movq	8(%r15), %rax
	movl	-4(%rbp), %esi                  ## 4-byte Reload
LBB93_11:
	andl	$4094, %ebx                     ## imm = 0xFFE
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
LBB93_12:
	movl	%esi, -8(%rbp)
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	leaq	-8(%rbp), %rsi
	movl	$2, %edx
	movq	%r12, %rcx
	callq	*8(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB93_8
## %bb.13:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB93_14
## %bb.15:
	testb	%al, %al
	js	LBB93_17
## %bb.16:
	andl	$252, %eax
LBB93_18:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	jmp	LBB93_19
LBB93_1:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$9, %esi
	callq	_XrBasicException
LBB93_19:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB93_14:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB93_19
LBB93_17:
	andl	$248, %eax
	jmp	LBB93_18
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteLoadIntImmOffset
_XrExecuteLoadIntImmOffset:             ## @XrExecuteLoadIntImmOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	13(%r14), %eax
	movl	4728(%r12,%rax,4), %ebx
	addl	8(%r14), %ebx
	testb	$1, %bl
	jne	LBB94_1
## %bb.2:
	movzbl	12(%r14), %eax
	leaq	(%r12,%rax,4), %rsi
	addq	$4728, %rsi                     ## imm = 0x1278
	testb	$4, 4864(%r12)
	je	LBB94_11
## %bb.3:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %r15
	addq	%r13, %r15
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB94_8
## %bb.4:
	movl	16(%r15), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB94_8
## %bb.5:
	movq	(%r15), %rcx
	testq	%rcx, %rcx
	je	LBB94_10
## %bb.6:
	andl	$4094, %ebx                     ## imm = 0xFFE
	movzwl	(%rcx,%rbx), %eax
	movw	%ax, (%rsi)
	movw	$0, 2(%rsi)
LBB94_7:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB94_8:
	movq	%rsi, -8(%rbp)                  ## 8-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r15, %rdx
	xorl	%ecx, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB94_19
## %bb.9:
	movq	8(%r15), %rax
	movq	-8(%rbp), %rsi                  ## 8-byte Reload
LBB94_10:
	andl	$4094, %ebx                     ## imm = 0xFFE
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
LBB94_11:
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	movq	%rsi, %r15
	movl	$2, %edx
	movq	%r12, %rcx
	callq	*16(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB94_12
## %bb.13:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB94_14
## %bb.15:
	testb	%al, %al
	js	LBB94_17
## %bb.16:
	andl	$252, %eax
LBB94_18:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	jmp	LBB94_19
LBB94_12:
	movzwl	(%r15), %eax
	movl	%eax, (%r15)
	jmp	LBB94_7
LBB94_1:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$9, %esi
	callq	_XrBasicException
LBB94_19:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB94_14:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB94_19
LBB94_17:
	andl	$248, %eax
	jmp	LBB94_18
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteSubi
_XrExecuteSubi:                         ## @XrExecuteSubi
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	movzbl	13(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	subl	8(%r14), %ecx
	movl	%ecx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteBne
_XrExecuteBne:                          ## @XrExecuteBne
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	cmpl	$0, 4728(%r12,%rax,4)
	je	LBB96_1
## %bb.2:
	movl	8(%r14), %eax
	movl	$152, %ebx
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	jne	LBB96_10
	jmp	LBB96_4
LBB96_1:
	movl	$160, %ebx
	movl	$4, %eax
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	je	LBB96_4
LBB96_10:
	movzbl	208(%r13), %ecx
	addl	%ecx, 5008(%r12)
	movzbl	4996(%r12), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 4996(%r12)
	testb	$31, %cl
	je	LBB96_11
## %bb.12:
	leaq	216(%rax), %r14
	movq	216(%rax), %rcx
	movq	%rax, %r13
	popq	%rbp
	jmpq	*%rcx                           ## TAILCALL
LBB96_4:
	movq	%r12, %rdi
	movq	%r13, %rsi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB96_5
## %bb.6:
	addq	%r13, %rbx
	movzbl	209(%rax), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 209(%rax)
	andl	$3, %ecx
	movl	%ecx, %edx
	movq	168(%rax,%rdx,8), %rdx
	testq	%rdx, %rdx
	je	LBB96_9
## %bb.7:
	cmpq	%rax, (%rdx)
	jne	LBB96_9
## %bb.8:
	movq	$0, (%rdx)
LBB96_9:
	movq	%rax, (%rbx)
	movq	%rbx, 168(%rax,%rcx,8)
	jmp	LBB96_10
LBB96_11:
	popq	%rbp
	retq
LBB96_5:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteJalr
_XrExecuteJalr:                         ## @XrExecuteJalr
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movq	%r13, %rsi
	movzbl	12(%r14), %eax
	movzbl	13(%r14), %ecx
	movl	4992(%r12), %edx
	addl	$4, %edx
	movl	%edx, 4728(%r12,%rax,4)
	movl	8(%r14), %ebx
	addl	4728(%r12,%rcx,4), %ebx
	movl	%ebx, 4992(%r12)
	testb	$3, %bl
	jne	LBB97_1
## %bb.4:
	movq	152(%rsi), %r13
	testq	%r13, %r13
	je	LBB97_5
LBB97_6:
	movl	%ebx, %eax
	shrl	$12, %eax
	movl	%ebx, %r15d
	shrl	$2, %r15d
	xorl	%eax, %r15d
	andl	$7, %r15d
	cmpl	%ebx, 64(%r13,%r15,4)
	jne	LBB97_8
## %bb.7:
	movq	(%r13,%r15,8), %rax
	testq	%rax, %rax
	je	LBB97_8
LBB97_13:
	movzbl	208(%rsi), %ecx
	addl	%ecx, 5008(%r12)
	movzbl	4996(%r12), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 4996(%r12)
	testb	$31, %cl
	je	LBB97_3
## %bb.14:
	leaq	216(%rax), %r14
	movq	216(%rax), %rcx
	movq	%rax, %r13
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rcx                           ## TAILCALL
LBB97_8:
	movq	%r12, %rdi
	movq	%rsi, -8(%rbp)                  ## 8-byte Spill
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB97_2
## %bb.9:
	movl	%ebx, 64(%r13,%r15,4)
	leaq	(,%r15,8), %rcx
	addq	%r13, %rcx
	movzbl	209(%rax), %edx
	leal	1(%rdx), %esi
	movb	%sil, 209(%rax)
	andl	$3, %edx
	movl	%edx, %esi
	movq	168(%rax,%rsi,8), %rsi
	testq	%rsi, %rsi
	je	LBB97_12
## %bb.10:
	cmpq	%rax, (%rsi)
	jne	LBB97_12
## %bb.11:
	movq	$0, (%rsi)
LBB97_12:
	movq	%rax, (%rcx)
	movq	%rcx, 168(%rax,%rdx,8)
	movq	-8(%rbp), %rsi                  ## 8-byte Reload
	jmp	LBB97_13
LBB97_1:
	movq	%rsi, -8(%rbp)                  ## 8-byte Spill
	movl	%ebx, 4892(%r12)
	movq	%r12, %rdi
	movl	$9, %esi
	movl	%ebx, %edx
	callq	_XrBasicException
LBB97_2:
	subq	-8(%rbp), %r14                  ## 8-byte Folded Reload
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
LBB97_3:
	addq	$16, %rsp
	popq	%rbp
	retq
LBB97_5:
	movq	576(%r12), %r13
	movq	(%r13), %rax
	movq	%rax, 576(%r12)
	xorps	%xmm0, %xmm0
	movups	%xmm0, 48(%r13)
	movups	%xmm0, 32(%r13)
	movups	%xmm0, 16(%r13)
	movups	%xmm0, (%r13)
	movq	%r13, 152(%rsi)
	movb	$1, 211(%rsi)
	jmp	LBB97_6
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeNor
_XrDecodeNor:                           ## @XrDecodeNor
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB98_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB98_2:
	leaq	_XrExecuteNor(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeOr
_XrDecodeOr:                            ## @XrDecodeOr
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB99_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB99_2:
	leaq	_XrExecuteOr(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeXor
_XrDecodeXor:                           ## @XrDecodeXor
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB100_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB100_2:
	leaq	_XrExecuteXor(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeAnd
_XrDecodeAnd:                           ## @XrDecodeAnd
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB101_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB101_2:
	leaq	_XrExecuteAnd(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeSltSigned
_XrDecodeSltSigned:                     ## @XrDecodeSltSigned
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB102_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB102_2:
	leaq	_XrExecuteSltSigned(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeSlt
_XrDecodeSlt:                           ## @XrDecodeSlt
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB103_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB103_2:
	leaq	_XrExecuteSlt(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeSub
_XrDecodeSub:                           ## @XrDecodeSub
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB104_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB104_2:
	leaq	_XrExecuteSub(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeAdd
_XrDecodeAdd:                           ## @XrDecodeAdd
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB105_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB105_2:
	leaq	_XrExecuteAdd(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeRegShifts
_XrDecodeRegShifts:                     ## @XrDecodeRegShifts
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edx, %eax
	shrl	$23, %eax
	andl	$24, %eax
	leaq	_XrRegShiftFunctionTable(%rip), %rcx
	movq	(%rax,%rcx), %rax
	movq	%rax, (%rsi)
	movl	%edx, %eax
	shrl	$6, %eax
	andb	$31, %al
	movb	%al, 12(%rsi)
	movl	4864(%rdi), %eax
	andl	$8, %eax
	movl	%edx, %ecx
	andl	$63488, %ecx                    ## imm = 0xF800
	movl	%edx, %edi
	shrl	$11, %edi
	andb	$31, %dil
	orl	%eax, %ecx
	movzbl	%dil, %ecx
	movl	$32, %edi
	cmovel	%edi, %ecx
	movl	%edx, %r8d
	andl	$2031616, %r8d                  ## imm = 0x1F0000
	shrl	$16, %edx
	andl	$31, %edx
	orl	%eax, %r8d
	cmovel	%edi, %edx
	movb	%cl, 13(%rsi)
	movl	%edx, 8(%rsi)
	leaq	16(%rsi), %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeStoreLongRegOffset
_XrDecodeStoreLongRegOffset:            ## @XrDecodeStoreLongRegOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB107_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB107_2:
	leaq	_XrExecuteStoreLongRegOffset(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	andl	$1984, %esi                     ## imm = 0x7C0
	movl	%edx, %r9d
	shrl	$6, %r9d
	andb	$31, %r9b
	orl	%ecx, %esi
	movzbl	%r9b, %esi
	cmovel	%r8d, %esi
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%ecx, %esi
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeStoreIntRegOffset
_XrDecodeStoreIntRegOffset:             ## @XrDecodeStoreIntRegOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB108_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB108_2:
	leaq	_XrExecuteStoreIntRegOffset(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	andl	$1984, %esi                     ## imm = 0x7C0
	movl	%edx, %r9d
	shrl	$6, %r9d
	andb	$31, %r9b
	orl	%ecx, %esi
	movzbl	%r9b, %esi
	cmovel	%r8d, %esi
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%ecx, %esi
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeStoreByteRegOffset
_XrDecodeStoreByteRegOffset:            ## @XrDecodeStoreByteRegOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB109_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB109_2:
	leaq	_XrExecuteStoreByteRegOffset(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	andl	$1984, %esi                     ## imm = 0x7C0
	movl	%edx, %r9d
	shrl	$6, %r9d
	andb	$31, %r9b
	orl	%ecx, %esi
	movzbl	%r9b, %esi
	cmovel	%r8d, %esi
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%ecx, %esi
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeLoadLongRegOffset
_XrDecodeLoadLongRegOffset:             ## @XrDecodeLoadLongRegOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB110_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB110_2:
	leaq	_XrExecuteLoadLongRegOffset(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeLoadIntRegOffset
_XrDecodeLoadIntRegOffset:              ## @XrDecodeLoadIntRegOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB111_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB111_2:
	leaq	_XrExecuteLoadIntRegOffset(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrDecodeLoadByteRegOffset
_XrDecodeLoadByteRegOffset:             ## @XrDecodeLoadByteRegOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4864(%rdi), %ecx
	andl	$8, %ecx
	movl	%edx, %eax
	andl	$2031616, %eax                  ## imm = 0x1F0000
	movl	%edx, %edi
	shrl	$16, %edi
	andl	$31, %edi
	orl	%ecx, %eax
	movl	$32, %r8d
	cmovel	%r8d, %edi
	movq	%rsi, %rax
	movl	%edx, %esi
	shrl	$21, %esi
	andl	$31, %esi
	je	LBB112_2
## %bb.1:
	movl	%edx, %r9d
	shrl	$23, %r9d
	andl	$24, %r9d
	leaq	_XrVirtualShiftInstructionTable(%rip), %r10
	movq	(%r9,%r10), %r9
	movq	%r9, (%rax)
	movb	%dil, 12(%rax)
	movb	%sil, 13(%rax)
	addq	$16, %rax
	movl	$33, %edi
LBB112_2:
	leaq	_XrExecuteLoadByteRegOffset(%rip), %rsi
	movq	%rsi, (%rax)
	movl	%edx, %esi
	shrl	$6, %esi
	andb	$31, %sil
	movb	%sil, 12(%rax)
	movl	%edx, %esi
	andl	$63488, %esi                    ## imm = 0xF800
	shrl	$11, %edx
	andb	$31, %dl
	orl	%esi, %ecx
	movzbl	%dl, %ecx
	cmovel	%r8d, %ecx
	movb	%cl, 13(%rax)
	movl	%edi, 8(%rax)
	addq	$16, %rax
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteNor
_XrExecuteNor:                          ## @XrExecuteNor
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	13(%r14), %eax
	movl	8(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	orl	4728(%r12,%rax,4), %ecx
	notl	%ecx
	movzbl	12(%r14), %eax
	movl	%ecx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteOr
_XrExecuteOr:                           ## @XrExecuteOr
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	13(%r14), %eax
	movl	8(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	orl	4728(%r12,%rax,4), %ecx
	movzbl	12(%r14), %eax
	movl	%ecx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteXor
_XrExecuteXor:                          ## @XrExecuteXor
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	13(%r14), %eax
	movl	8(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	xorl	4728(%r12,%rax,4), %ecx
	movzbl	12(%r14), %eax
	movl	%ecx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteAnd
_XrExecuteAnd:                          ## @XrExecuteAnd
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	13(%r14), %eax
	movl	8(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	andl	4728(%r12,%rax,4), %ecx
	movzbl	12(%r14), %eax
	movl	%ecx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteSltSigned
_XrExecuteSltSigned:                    ## @XrExecuteSltSigned
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	13(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	movl	8(%r14), %ecx
	xorl	%edx, %edx
	cmpl	4728(%r12,%rcx,4), %eax
	setl	%dl
	movzbl	12(%r14), %eax
	movl	%edx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteSlt
_XrExecuteSlt:                          ## @XrExecuteSlt
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	13(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	movl	8(%r14), %ecx
	xorl	%edx, %edx
	cmpl	4728(%r12,%rcx,4), %eax
	setb	%dl
	movzbl	12(%r14), %eax
	movl	%edx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteSub
_XrExecuteSub:                          ## @XrExecuteSub
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	13(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	movl	8(%r14), %ecx
	subl	4728(%r12,%rcx,4), %eax
	movzbl	12(%r14), %ecx
	movl	%eax, 4728(%r12,%rcx,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteAdd
_XrExecuteAdd:                          ## @XrExecuteAdd
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	13(%r14), %eax
	movl	8(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	addl	4728(%r12,%rax,4), %ecx
	movzbl	12(%r14), %eax
	movl	%ecx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteLsh
_XrExecuteLsh:                          ## @XrExecuteLsh
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	8(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	movzbl	13(%r14), %ecx
	movzbl	4728(%r12,%rcx,4), %ecx
	shll	%cl, %eax
	movzbl	12(%r14), %ecx
	movl	%eax, 4728(%r12,%rcx,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteRsh
_XrExecuteRsh:                          ## @XrExecuteRsh
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	8(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	movzbl	13(%r14), %ecx
	movzbl	4728(%r12,%rcx,4), %ecx
	shrl	%cl, %eax
	movzbl	12(%r14), %ecx
	movl	%eax, 4728(%r12,%rcx,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteAsh
_XrExecuteAsh:                          ## @XrExecuteAsh
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	8(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	movzbl	13(%r14), %ecx
	movzbl	4728(%r12,%rcx,4), %ecx
	sarl	%cl, %eax
	movzbl	12(%r14), %ecx
	movl	%eax, 4728(%r12,%rcx,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteRor
_XrExecuteRor:                          ## @XrExecuteRor
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	8(%r14), %eax
	movl	4728(%r12,%rax,4), %eax
	movzbl	13(%r14), %ecx
	movzbl	4728(%r12,%rcx,4), %ecx
	rorl	%cl, %eax
	movzbl	12(%r14), %ecx
	movl	%eax, 4728(%r12,%rcx,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteStoreLongRegOffset
_XrExecuteStoreLongRegOffset:           ## @XrExecuteStoreLongRegOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	13(%r14), %eax
	movl	8(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ebx
	addl	4728(%r12,%rax,4), %ebx
	testb	$3, %bl
	jne	LBB125_1
## %bb.2:
	movzbl	12(%r14), %eax
	movl	4728(%r12,%rax,4), %esi
	testb	$4, 4864(%r12)
	je	LBB125_12
## %bb.3:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %r15
	addq	%r13, %r15
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB125_9
## %bb.4:
	testb	$2, %al
	je	LBB125_9
## %bb.5:
	movl	16(%r15), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB125_9
## %bb.6:
	movq	(%r15), %rcx
	testq	%rcx, %rcx
	je	LBB125_11
## %bb.7:
	andl	$4092, %ebx                     ## imm = 0xFFC
	movl	%esi, (%rcx,%rbx)
LBB125_8:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB125_9:
	movl	%esi, -4(%rbp)                  ## 4-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r15, %rdx
	movl	$1, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB125_19
## %bb.10:
	movq	8(%r15), %rax
	movl	-4(%rbp), %esi                  ## 4-byte Reload
LBB125_11:
	andl	$4092, %ebx                     ## imm = 0xFFC
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
LBB125_12:
	movl	%esi, -8(%rbp)
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	leaq	-8(%rbp), %rsi
	movl	$4, %edx
	movq	%r12, %rcx
	callq	*8(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB125_8
## %bb.13:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB125_14
## %bb.15:
	testb	%al, %al
	js	LBB125_17
## %bb.16:
	andl	$252, %eax
LBB125_18:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	jmp	LBB125_19
LBB125_1:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$9, %esi
	callq	_XrBasicException
LBB125_19:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB125_14:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB125_19
LBB125_17:
	andl	$248, %eax
	jmp	LBB125_18
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteStoreIntRegOffset
_XrExecuteStoreIntRegOffset:            ## @XrExecuteStoreIntRegOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	13(%r14), %eax
	movl	8(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ebx
	addl	4728(%r12,%rax,4), %ebx
	testb	$1, %bl
	jne	LBB126_1
## %bb.2:
	movzbl	12(%r14), %eax
	movl	4728(%r12,%rax,4), %esi
	testb	$4, 4864(%r12)
	je	LBB126_12
## %bb.3:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %r15
	addq	%r13, %r15
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB126_9
## %bb.4:
	testb	$2, %al
	je	LBB126_9
## %bb.5:
	movl	16(%r15), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB126_9
## %bb.6:
	movq	(%r15), %rcx
	testq	%rcx, %rcx
	je	LBB126_11
## %bb.7:
	andl	$4094, %ebx                     ## imm = 0xFFE
	movw	%si, (%rcx,%rbx)
LBB126_8:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB126_9:
	movl	%esi, -4(%rbp)                  ## 4-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r15, %rdx
	movl	$1, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB126_19
## %bb.10:
	movq	8(%r15), %rax
	movl	-4(%rbp), %esi                  ## 4-byte Reload
LBB126_11:
	andl	$4094, %ebx                     ## imm = 0xFFE
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
LBB126_12:
	movl	%esi, -8(%rbp)
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	leaq	-8(%rbp), %rsi
	movl	$2, %edx
	movq	%r12, %rcx
	callq	*8(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB126_8
## %bb.13:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB126_14
## %bb.15:
	testb	%al, %al
	js	LBB126_17
## %bb.16:
	andl	$252, %eax
LBB126_18:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	jmp	LBB126_19
LBB126_1:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$9, %esi
	callq	_XrBasicException
LBB126_19:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB126_14:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB126_19
LBB126_17:
	andl	$248, %eax
	jmp	LBB126_18
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteStoreByteRegOffset
_XrExecuteStoreByteRegOffset:           ## @XrExecuteStoreByteRegOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	13(%r14), %eax
	movl	8(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ebx
	movzbl	12(%r14), %ecx
	movl	4728(%r12,%rcx,4), %esi
	addl	4728(%r12,%rax,4), %ebx
	testb	$4, 4864(%r12)
	je	LBB127_10
## %bb.1:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %r15
	addq	%r13, %r15
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB127_7
## %bb.2:
	testb	$2, %al
	je	LBB127_7
## %bb.3:
	movl	16(%r15), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB127_7
## %bb.4:
	movq	(%r15), %rcx
	testq	%rcx, %rcx
	je	LBB127_9
## %bb.5:
	andl	$4095, %ebx                     ## imm = 0xFFF
	movb	%sil, (%rcx,%rbx)
LBB127_6:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB127_7:
	movl	%esi, -4(%rbp)                  ## 4-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r15, %rdx
	movl	$1, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB127_17
## %bb.8:
	movq	8(%r15), %rax
	movl	-4(%rbp), %esi                  ## 4-byte Reload
LBB127_9:
	andl	$4095, %ebx                     ## imm = 0xFFF
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
LBB127_10:
	movl	%esi, -8(%rbp)
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	leaq	-8(%rbp), %rsi
	movl	$1, %edx
	movq	%r12, %rcx
	callq	*8(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB127_6
## %bb.11:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB127_12
## %bb.13:
	testb	%al, %al
	js	LBB127_15
## %bb.14:
	andl	$252, %eax
LBB127_16:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
LBB127_17:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB127_12:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB127_17
LBB127_15:
	andl	$248, %eax
	jmp	LBB127_16
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteLoadLongRegOffset
_XrExecuteLoadLongRegOffset:            ## @XrExecuteLoadLongRegOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	13(%r14), %eax
	movl	8(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ebx
	addl	4728(%r12,%rax,4), %ebx
	testb	$3, %bl
	jne	LBB128_1
## %bb.2:
	movzbl	12(%r14), %eax
	leaq	(%r12,%rax,4), %rsi
	addq	$4728, %rsi                     ## imm = 0x1278
	testb	$4, 4864(%r12)
	je	LBB128_11
## %bb.3:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %r15
	addq	%r13, %r15
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB128_8
## %bb.4:
	movl	16(%r15), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB128_8
## %bb.5:
	movq	(%r15), %rcx
	testq	%rcx, %rcx
	je	LBB128_10
## %bb.6:
	andl	$4092, %ebx                     ## imm = 0xFFC
	movl	(%rcx,%rbx), %eax
	movl	%eax, (%rsi)
LBB128_7:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB128_8:
	movq	%rsi, -8(%rbp)                  ## 8-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r15, %rdx
	xorl	%ecx, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB128_18
## %bb.9:
	movq	8(%r15), %rax
	movq	-8(%rbp), %rsi                  ## 8-byte Reload
LBB128_10:
	andl	$4092, %ebx                     ## imm = 0xFFC
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
LBB128_11:
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	movl	$4, %edx
	movq	%r12, %rcx
	callq	*16(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB128_7
## %bb.12:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB128_13
## %bb.14:
	testb	%al, %al
	js	LBB128_16
## %bb.15:
	andl	$252, %eax
LBB128_17:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	jmp	LBB128_18
LBB128_1:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$9, %esi
	callq	_XrBasicException
LBB128_18:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB128_13:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB128_18
LBB128_16:
	andl	$248, %eax
	jmp	LBB128_17
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteLoadIntRegOffset
_XrExecuteLoadIntRegOffset:             ## @XrExecuteLoadIntRegOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	13(%r14), %eax
	movl	8(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ebx
	addl	4728(%r12,%rax,4), %ebx
	testb	$1, %bl
	jne	LBB129_1
## %bb.2:
	movzbl	12(%r14), %eax
	leaq	(%r12,%rax,4), %rsi
	addq	$4728, %rsi                     ## imm = 0x1278
	testb	$4, 4864(%r12)
	je	LBB129_11
## %bb.3:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %r15
	addq	%r13, %r15
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB129_8
## %bb.4:
	movl	16(%r15), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB129_8
## %bb.5:
	movq	(%r15), %rcx
	testq	%rcx, %rcx
	je	LBB129_10
## %bb.6:
	andl	$4094, %ebx                     ## imm = 0xFFE
	movzwl	(%rcx,%rbx), %eax
	movw	%ax, (%rsi)
	movw	$0, 2(%rsi)
LBB129_7:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB129_8:
	movq	%rsi, -8(%rbp)                  ## 8-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r15, %rdx
	xorl	%ecx, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB129_19
## %bb.9:
	movq	8(%r15), %rax
	movq	-8(%rbp), %rsi                  ## 8-byte Reload
LBB129_10:
	andl	$4094, %ebx                     ## imm = 0xFFE
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
LBB129_11:
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	movq	%rsi, %r15
	movl	$2, %edx
	movq	%r12, %rcx
	callq	*16(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB129_12
## %bb.13:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB129_14
## %bb.15:
	testb	%al, %al
	js	LBB129_17
## %bb.16:
	andl	$252, %eax
LBB129_18:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
	jmp	LBB129_19
LBB129_12:
	movzwl	(%r15), %eax
	movl	%eax, (%r15)
	jmp	LBB129_7
LBB129_1:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %edx
	movq	%r12, %rdi
	movl	$9, %esi
	callq	_XrBasicException
LBB129_19:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB129_14:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB129_19
LBB129_17:
	andl	$248, %eax
	jmp	LBB129_18
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteLoadByteRegOffset
_XrExecuteLoadByteRegOffset:            ## @XrExecuteLoadByteRegOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	13(%r14), %eax
	movl	8(%r14), %ecx
	movl	4728(%r12,%rcx,4), %r15d
	movzbl	12(%r14), %ecx
	leaq	(%r12,%rcx,4), %rsi
	addq	$4728, %rsi                     ## imm = 0x1278
	addl	4728(%r12,%rax,4), %r15d
	testb	$4, 4864(%r12)
	je	LBB130_9
## %bb.1:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%r15d, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %rbx
	addq	%r13, %rbx
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB130_6
## %bb.2:
	movl	16(%rbx), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB130_6
## %bb.3:
	movq	(%rbx), %rcx
	testq	%rcx, %rcx
	je	LBB130_8
## %bb.4:
	andl	$4095, %r15d                    ## imm = 0xFFF
	movzbl	(%rcx,%r15), %eax
	movb	%al, (%rsi)
	movb	$0, 1(%rsi)
	movw	$0, 2(%rsi)
LBB130_5:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB130_6:
	movq	%rsi, -8(%rbp)                  ## 8-byte Spill
	movq	%r12, %rdi
	movl	%r15d, %esi
	movq	%rbx, %rdx
	xorl	%ecx, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB130_17
## %bb.7:
	movq	8(%rbx), %rax
	movq	-8(%rbp), %rsi                  ## 8-byte Reload
LBB130_8:
	andl	$4095, %r15d                    ## imm = 0xFFF
	andl	$-32, %eax
	shll	$7, %eax
	orl	%r15d, %eax
	movl	%eax, %r15d
LBB130_9:
	movl	%r15d, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%r15d, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	movq	%rsi, %rbx
	movl	$1, %edx
	movq	%r12, %rcx
	callq	*16(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB130_10
## %bb.11:
	movl	%r15d, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB130_12
## %bb.13:
	testb	%al, %al
	js	LBB130_15
## %bb.14:
	andl	$252, %eax
LBB130_16:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
LBB130_17:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB130_10:
	movzbl	(%rbx), %eax
	movl	%eax, (%rbx)
	jmp	LBB130_5
LBB130_12:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB130_17
LBB130_15:
	andl	$248, %eax
	jmp	LBB130_16
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteStoreByteImmOffsetReg
_XrExecuteStoreByteImmOffsetReg:        ## @XrExecuteStoreByteImmOffsetReg
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	12(%r14), %eax
	movzbl	13(%r14), %ecx
	movl	4728(%r12,%rax,4), %ebx
	movl	4728(%r12,%rcx,4), %esi
	addl	8(%r14), %ebx
	testb	$4, 4864(%r12)
	je	LBB131_10
## %bb.1:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%ebx, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %r15
	addq	%r13, %r15
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB131_7
## %bb.2:
	testb	$2, %al
	je	LBB131_7
## %bb.3:
	movl	16(%r15), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB131_7
## %bb.4:
	movq	(%r15), %rcx
	testq	%rcx, %rcx
	je	LBB131_9
## %bb.5:
	andl	$4095, %ebx                     ## imm = 0xFFF
	movb	%sil, (%rcx,%rbx)
LBB131_6:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB131_7:
	movl	%esi, -4(%rbp)                  ## 4-byte Spill
	movq	%r12, %rdi
	movl	%ebx, %esi
	movq	%r15, %rdx
	movl	$1, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB131_17
## %bb.8:
	movq	8(%r15), %rax
	movl	-4(%rbp), %esi                  ## 4-byte Reload
LBB131_9:
	andl	$4095, %ebx                     ## imm = 0xFFF
	andl	$-32, %eax
	shll	$7, %eax
	orl	%ebx, %eax
	movl	%eax, %ebx
LBB131_10:
	movl	%esi, -8(%rbp)
	movl	%ebx, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%ebx, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	leaq	-8(%rbp), %rsi
	movl	$1, %edx
	movq	%r12, %rcx
	callq	*8(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB131_6
## %bb.11:
	movl	%ebx, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB131_12
## %bb.13:
	testb	%al, %al
	js	LBB131_15
## %bb.14:
	andl	$252, %eax
LBB131_16:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
LBB131_17:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB131_12:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB131_17
LBB131_15:
	andl	$248, %eax
	jmp	LBB131_16
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteLoadByteImmOffset
_XrExecuteLoadByteImmOffset:            ## @XrExecuteLoadByteImmOffset
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movzbl	12(%r14), %eax
	movzbl	13(%r14), %ecx
	movl	4728(%r12,%rcx,4), %r15d
	leaq	(%r12,%rax,4), %rsi
	addq	$4728, %rsi                     ## imm = 0x1278
	addl	8(%r14), %r15d
	testb	$4, 4864(%r12)
	je	LBB132_9
## %bb.1:
	movl	$-1048576, %ecx                 ## imm = 0xFFF00000
	andl	4964(%r12), %ecx
	movl	%r15d, %eax
	shrl	$12, %eax
	orl	%eax, %ecx
	andl	$3, %eax
	leaq	(%rax,%rax,2), %rax
	leaq	(,%rax,8), %rbx
	addq	%r13, %rbx
	movq	8(%r13,%rax,8), %rax
	movq	%rax, %rdx
	shrq	$32, %rdx
	cmpl	%ecx, %edx
	jne	LBB132_6
## %bb.2:
	movl	16(%rbx), %ecx
	cmpq	%rax, 256(%r12,%rcx,8)
	jne	LBB132_6
## %bb.3:
	movq	(%rbx), %rcx
	testq	%rcx, %rcx
	je	LBB132_8
## %bb.4:
	andl	$4095, %r15d                    ## imm = 0xFFF
	movzbl	(%rcx,%r15), %eax
	movb	%al, (%rsi)
	movb	$0, 1(%rsi)
	movw	$0, 2(%rsi)
LBB132_5:
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	addq	$16, %rsp
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB132_6:
	movq	%rsi, -8(%rbp)                  ## 8-byte Spill
	movq	%r12, %rdi
	movl	%r15d, %esi
	movq	%rbx, %rdx
	xorl	%ecx, %ecx
	callq	_XrTranslateDstream
	testl	%eax, %eax
	je	LBB132_17
## %bb.7:
	movq	8(%rbx), %rax
	movq	-8(%rbp), %rsi                  ## 8-byte Reload
LBB132_8:
	andl	$4095, %r15d                    ## imm = 0xFFF
	andl	$-32, %eax
	shll	$7, %eax
	orl	%r15d, %eax
	movl	%eax, %r15d
LBB132_9:
	movl	%r15d, %eax
	shrl	$27, %eax
	leaq	(%rax,%rax,4), %rax
	movq	_EBusBranches@GOTPCREL(%rip), %r8
	movl	%r15d, %edi
	andl	$134217727, %edi                ## imm = 0x7FFFFFF
	movq	%rsi, %rbx
	movl	$1, %edx
	movq	%r12, %rcx
	callq	*16(%r8,%rax,8)
	testl	%eax, %eax
	je	LBB132_10
## %bb.11:
	movl	%r15d, 4892(%r12)
	movl	4992(%r12), %eax
	movl	%eax, 4888(%r12)
	movl	4864(%r12), %eax
	movl	4884(%r12), %ecx
	movl	%eax, %edx
	andl	$251658495, %edx                ## imm = 0xF0000FF
	movzwl	%ax, %esi
	shll	$8, %esi
	addl	%esi, %edx
	addl	$1073741824, %edx               ## imm = 0x40000000
	movl	%edx, 4864(%r12)
	testl	%ecx, %ecx
	je	LBB132_12
## %bb.13:
	testb	%al, %al
	js	LBB132_15
## %bb.14:
	andl	$252, %eax
LBB132_16:
	orl	$1024, %ecx                     ## imm = 0x400
	movl	%ecx, 4992(%r12)
	andl	$1342177024, %edx               ## imm = 0x4FFFFF00
	orl	%eax, %edx
	movl	%edx, 4864(%r12)
	movb	$64, 5028(%r12)
	movl	$32, 5004(%r12)
LBB132_17:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	addq	$16, %rsp
	popq	%rbp
	retq
LBB132_10:
	movzbl	(%rbx), %eax
	movl	%eax, (%rbx)
	jmp	LBB132_5
LBB132_12:
	movq	%r12, %rdi
	callq	_XrReset
	jmp	LBB132_17
LBB132_15:
	andl	$248, %eax
	jmp	LBB132_16
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteAddi
_XrExecuteAddi:                         ## @XrExecuteAddi
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	movzbl	13(%r14), %ecx
	movl	4728(%r12,%rcx,4), %ecx
	addl	8(%r14), %ecx
	movl	%ecx, 4728(%r12,%rax,4)
	addl	$4, 4992(%r12)
	movq	16(%r14), %rax
	addq	$16, %r14
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteB
_XrExecuteB:                            ## @XrExecuteB
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	8(%r14), %eax
	addl	%eax, 4992(%r12)
	movq	%r13, %rbx
	movq	152(%r13), %r13
	testq	%r13, %r13
	je	LBB134_1
LBB134_7:
	movzbl	208(%rbx), %eax
	addl	%eax, 5008(%r12)
	movzbl	4996(%r12), %eax
	leal	1(%rax), %ecx
	movb	%cl, 4996(%r12)
	testb	$31, %al
	je	LBB134_8
## %bb.9:
	leaq	216(%r13), %r14
	movq	216(%r13), %rax
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB134_1:
	movq	%r12, %rdi
	movq	%rbx, %rsi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB134_2
## %bb.3:
	movq	%rax, %r13
	leaq	152(%rbx), %rax
	movzbl	209(%r13), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 209(%r13)
	andl	$3, %ecx
	movl	%ecx, %edx
	movq	168(%r13,%rdx,8), %rdx
	testq	%rdx, %rdx
	je	LBB134_6
## %bb.4:
	cmpq	%r13, (%rdx)
	jne	LBB134_6
## %bb.5:
	movq	$0, (%rdx)
LBB134_6:
	movq	%r13, (%rax)
	movq	%rax, 168(%r13,%rcx,8)
	jmp	LBB134_7
LBB134_8:
	popq	%rbp
	retq
LBB134_2:
	subq	%rbx, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteBeq
_XrExecuteBeq:                          ## @XrExecuteBeq
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movzbl	12(%r14), %eax
	cmpl	$0, 4728(%r12,%rax,4)
	je	LBB135_2
## %bb.1:
	movl	$160, %ebx
	movl	$4, %eax
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	jne	LBB135_10
	jmp	LBB135_4
LBB135_2:
	movl	8(%r14), %eax
	movl	$152, %ebx
	addl	%eax, 4992(%r12)
	movq	(%r13,%rbx), %rax
	testq	%rax, %rax
	je	LBB135_4
LBB135_10:
	movzbl	208(%r13), %ecx
	addl	%ecx, 5008(%r12)
	movzbl	4996(%r12), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 4996(%r12)
	testb	$31, %cl
	je	LBB135_11
## %bb.12:
	leaq	216(%rax), %r14
	movq	216(%rax), %rcx
	movq	%rax, %r13
	popq	%rbp
	jmpq	*%rcx                           ## TAILCALL
LBB135_4:
	movq	%r12, %rdi
	movq	%r13, %rsi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB135_5
## %bb.6:
	addq	%r13, %rbx
	movzbl	209(%rax), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 209(%rax)
	andl	$3, %ecx
	movl	%ecx, %edx
	movq	168(%rax,%rdx,8), %rdx
	testq	%rdx, %rdx
	je	LBB135_9
## %bb.7:
	cmpq	%rax, (%rdx)
	jne	LBB135_9
## %bb.8:
	movq	$0, (%rdx)
LBB135_9:
	movq	%rax, (%rbx)
	movq	%rbx, 168(%rax,%rcx,8)
	jmp	LBB135_10
LBB135_11:
	popq	%rbp
	retq
LBB135_5:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteJ
_XrExecuteJ:                            ## @XrExecuteJ
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%r13, %rbx
	movl	8(%r14), %eax
	movl	%eax, 4992(%r12)
	movq	152(%r13), %r13
	testq	%r13, %r13
	je	LBB136_1
LBB136_7:
	movzbl	208(%rbx), %eax
	addl	%eax, 5008(%r12)
	movzbl	4996(%r12), %eax
	leal	1(%rax), %ecx
	movb	%cl, 4996(%r12)
	testb	$31, %al
	je	LBB136_8
## %bb.9:
	leaq	216(%r13), %r14
	movq	216(%r13), %rax
	popq	%rbp
	jmpq	*%rax                           ## TAILCALL
LBB136_1:
	movq	%r12, %rdi
	movq	%rbx, %rsi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB136_2
## %bb.3:
	movq	%rax, %r13
	leaq	152(%rbx), %rax
	movzbl	209(%r13), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 209(%r13)
	andl	$3, %ecx
	movl	%ecx, %edx
	movq	168(%r13,%rdx,8), %rdx
	testq	%rdx, %rdx
	je	LBB136_6
## %bb.4:
	cmpq	%r13, (%rdx)
	jne	LBB136_6
## %bb.5:
	movq	$0, (%rdx)
LBB136_6:
	movq	%r13, (%rax)
	movq	%rax, 168(%r13,%rcx,8)
	jmp	LBB136_7
LBB136_8:
	popq	%rbp
	retq
LBB136_2:
	subq	%rbx, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.p2align	4                               ## -- Begin function XrExecuteJal
_XrExecuteJal:                          ## @XrExecuteJal
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	4992(%r12), %eax
	addl	$4, %eax
	movl	%eax, 4852(%r12)
	movl	8(%r14), %eax
	movl	%eax, 4992(%r12)
	movq	152(%r13), %rax
	testq	%rax, %rax
	je	LBB137_1
LBB137_7:
	movzbl	208(%r13), %ecx
	addl	%ecx, 5008(%r12)
	movzbl	4996(%r12), %ecx
	leal	1(%rcx), %edx
	movb	%dl, 4996(%r12)
	testb	$31, %cl
	je	LBB137_8
## %bb.9:
	leaq	216(%rax), %r14
	movq	216(%rax), %rcx
	movq	%rax, %r13
	popq	%rbp
	jmpq	*%rcx                           ## TAILCALL
LBB137_1:
	movq	%r12, %rdi
	movq	%r13, %rsi
	callq	_XrDecodeInstructions
	testq	%rax, %rax
	je	LBB137_2
## %bb.3:
	leaq	152(%r13), %rcx
	movzbl	209(%rax), %edx
	leal	1(%rdx), %esi
	movb	%sil, 209(%rax)
	andl	$3, %edx
	movl	%edx, %esi
	movq	168(%rax,%rsi,8), %rsi
	testq	%rsi, %rsi
	je	LBB137_6
## %bb.4:
	cmpq	%rax, (%rsi)
	jne	LBB137_6
## %bb.5:
	movq	$0, (%rsi)
LBB137_6:
	movq	%rax, (%rcx)
	movq	%rcx, 168(%rax,%rdx,8)
	jmp	LBB137_7
LBB137_8:
	popq	%rbp
	retq
LBB137_2:
	subq	%r13, %r14
	addq	$-216, %r14
	shrq	$4, %r14
	addl	%r14d, 5008(%r12)
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.section	__DATA,__const
	.p2align	4, 0x0                          ## @XrDecodeLowThree
_XrDecodeLowThree:
	.quad	_XrDecodeMajor
	.quad	_XrDecodeMajor
	.quad	_XrDecodeMajor
	.quad	_XrDecodeMajor
	.quad	_XrDecodeMajor
	.quad	_XrDecodeMajor
	.quad	_XrDecodeJ
	.quad	_XrDecodeJal

	.p2align	4, 0x0                          ## @XrDecodeLowSix
_XrDecodeLowSix:
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeLui
	.quad	_XrDecodeBpo
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeStoreLongImmOffsetImm
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeOri
	.quad	_XrDecodeBpe
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeStoreIntImmOffsetImm
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeXori
	.quad	_XrDecodeBge
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeStoreByteImmOffsetImm
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeAndi
	.quad	_XrDecodeBle
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeSltiSigned
	.quad	_XrDecodeBgt
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecode101001
	.quad	_XrDecodeStoreLongImmOffsetReg
	.quad	_XrDecodeLoadLongImmOffset
	.quad	_XrDecodeSlti
	.quad	_XrDecodeBlt
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecode110001
	.quad	_XrDecodeStoreIntImmOffsetReg
	.quad	_XrDecodeLoadIntImmOffset
	.quad	_XrDecodeSubi
	.quad	_XrDecodeBne
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeJalr
	.quad	_XrDecode111001
	.quad	_XrDecodeStoreByteImmOffsetReg
	.quad	_XrDecodeLoadByteImmOffset
	.quad	_XrDecodeAddi
	.quad	_XrDecodeBeq
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction

	.p2align	4, 0x0                          ## @XrDecodeFunctions101001
_XrDecodeFunctions101001:
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeRfe
	.quad	_XrDecodeHlt
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeMtcr
	.quad	_XrDecodeMfcr

	.p2align	4, 0x0                          ## @XrDecodeFunctions110001
_XrDecodeFunctions110001:
	.quad	_XrDecodeSys
	.quad	_XrDecodeBrk
	.quad	_XrDecodeWmb
	.quad	_XrDecodeMb
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodePause
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeSC
	.quad	_XrDecodeLL
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeMod
	.quad	_XrDecodeDivSigned
	.quad	_XrDecodeDiv
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeMul

	.p2align	4, 0x0                          ## @XrVirtualShiftInstructionTable
_XrVirtualShiftInstructionTable:
	.quad	_XrExecuteVirtualLsh
	.quad	_XrExecuteVirtualRsh
	.quad	_XrExecuteVirtualAsh
	.quad	_XrExecuteVirtualRor

	.p2align	4, 0x0                          ## @XrDecodeFunctions111001
_XrDecodeFunctions111001:
	.quad	_XrDecodeNor
	.quad	_XrDecodeOr
	.quad	_XrDecodeXor
	.quad	_XrDecodeAnd
	.quad	_XrDecodeSltSigned
	.quad	_XrDecodeSlt
	.quad	_XrDecodeSub
	.quad	_XrDecodeAdd
	.quad	_XrDecodeRegShifts
	.quad	_XrDecodeStoreLongRegOffset
	.quad	_XrDecodeStoreIntRegOffset
	.quad	_XrDecodeStoreByteRegOffset
	.quad	_XrDecodeIllegalInstruction
	.quad	_XrDecodeLoadLongRegOffset
	.quad	_XrDecodeLoadIntRegOffset
	.quad	_XrDecodeLoadByteRegOffset

	.p2align	4, 0x0                          ## @XrRegShiftFunctionTable
_XrRegShiftFunctionTable:
	.quad	_XrExecuteLsh
	.quad	_XrExecuteRsh
	.quad	_XrExecuteAsh
	.quad	_XrExecuteRor

	.section	__TEXT,__literal16,16byte_literals
	.p2align	4, 0x0                          ## @.memset_pattern.4
L_.memset_pattern.4:
	.quad	-4503599627370496               ## 0xfff0000000000000
	.quad	-4503599627370496               ## 0xfff0000000000000

.subsections_via_symbols

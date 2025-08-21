/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/* Copyright (c) 2011-2014 PLUMgrid, http://plumgrid.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 */
#ifndef _UAPI__LINUX_BPF_H__
#define _UAPI__LINUX_BPF_H__

#include <linux/types.h>

#include "lib/bpf/bpf_common.h"

/* Extended instruction set based on top of classic BPF */

/* instruction classes */
#define BPF_JMP32	0x06	/* jmp mode in word width */
#define BPF_ALU64	0x07	/* alu mode in double word width */

/* ld/ldx fields */
#define BPF_DW		0x18	/* double word (64-bit) */
#define BPF_MEMSX	0x80	/* load with sign extension */
#define BPF_ATOMIC	0xc0	/* atomic memory ops - op type in immediate */
#define BPF_XADD	0xc0	/* exclusive add - legacy name */

/* alu/jmp fields */
#define BPF_MOV		0xb0	/* mov reg to reg */
#define BPF_ARSH	0xc0	/* sign extending arithmetic shift right */

/* change endianness of a register */
#define BPF_END		0xd0	/* flags for endianness conversion: */
#define BPF_TO_LE	0x00	/* convert to little-endian */
#define BPF_TO_BE	0x08	/* convert to big-endian */
#define BPF_FROM_LE	BPF_TO_LE
#define BPF_FROM_BE	BPF_TO_BE

/* jmp encodings */
#define BPF_JNE		0x50	/* jump != */
#define BPF_JLT		0xa0	/* LT is unsigned, '<' */
#define BPF_JLE		0xb0	/* LE is unsigned, '<=' */
#define BPF_JSGT	0x60	/* SGT is signed '>', GT in x86 */
#define BPF_JSGE	0x70	/* SGE is signed '>=', GE in x86 */
#define BPF_JSLT	0xc0	/* SLT is signed, '<' */
#define BPF_JSLE	0xd0	/* SLE is signed, '<=' */
#define BPF_JCOND	0xe0	/* conditional pseudo jumps: may_goto, goto_or_nop */
#define BPF_CALL	0x80	/* function call */
#define BPF_EXIT	0x90	/* function return */

/* atomic op type fields (stored in immediate) */
#define BPF_FETCH	0x01	/* not an opcode on its own, used to build others */
#define BPF_XCHG	(0xe0 | BPF_FETCH)	/* atomic exchange */
#define BPF_CMPXCHG	(0xf0 | BPF_FETCH)	/* atomic compare-and-write */

#define BPF_LOAD_ACQ	0x100	/* load-acquire */
#define BPF_STORE_REL	0x110	/* store-release */

enum bpf_cond_pseudo_jmp {
	BPF_MAY_GOTO = 0,
};


/* When BPF ldimm64's insn[0].src_reg != 0 then this can have
 * the following extensions:
 *
 * insn[0].src_reg:  BPF_PSEUDO_MAP_[FD|IDX]
 * insn[0].imm:      map fd or fd_idx
 * insn[1].imm:      0
 * insn[0].off:      0
 * insn[1].off:      0
 * ldimm64 rewrite:  address of map
 * verifier type:    CONST_PTR_TO_MAP
 */
 #define BPF_PSEUDO_MAP_FD	1
 #define BPF_PSEUDO_MAP_IDX	5

 /* insn[0].src_reg:  BPF_PSEUDO_MAP_[IDX_]VALUE
  * insn[0].imm:      map fd or fd_idx
  * insn[1].imm:      offset into value
  * insn[0].off:      0
  * insn[1].off:      0
  * ldimm64 rewrite:  address of map[0]+offset
  * verifier type:    PTR_TO_MAP_VALUE
  */
 #define BPF_PSEUDO_MAP_VALUE		2
 #define BPF_PSEUDO_MAP_IDX_VALUE	6

 /* insn[0].src_reg:  BPF_PSEUDO_BTF_ID
  * insn[0].imm:      kernel btd id of VAR
  * insn[1].imm:      0
  * insn[0].off:      0
  * insn[1].off:      0
  * ldimm64 rewrite:  address of the kernel variable
  * verifier type:    PTR_TO_BTF_ID or PTR_TO_MEM, depending on whether the var
  *                   is struct/union.
  */
 #define BPF_PSEUDO_BTF_ID	3
 /* insn[0].src_reg:  BPF_PSEUDO_FUNC
  * insn[0].imm:      insn offset to the func
  * insn[1].imm:      0
  * insn[0].off:      0
  * insn[1].off:      0
  * ldimm64 rewrite:  address of the function
  * verifier type:    PTR_TO_FUNC.
  */
 #define BPF_PSEUDO_FUNC		4

 /* when bpf_call->src_reg == BPF_PSEUDO_CALL, bpf_call->imm == pc-relative
  * offset to another bpf function
  */
 #define BPF_PSEUDO_CALL		1
 /* when bpf_call->src_reg == BPF_PSEUDO_KFUNC_CALL,
  * bpf_call->imm == btf_id of a BTF_KIND_FUNC in the running kernel
  */
 #define BPF_PSEUDO_KFUNC_CALL	2

/* Register numbers */
enum {
	BPF_REG_0 = 0,
	BPF_REG_1,
	BPF_REG_2,
	BPF_REG_3,
	BPF_REG_4,
	BPF_REG_5,
	BPF_REG_6,
	BPF_REG_7,
	BPF_REG_8,
	BPF_REG_9,
	BPF_REG_10,
	__MAX_BPF_REG,
};

/* BPF has 10 general purpose 64-bit registers and stack frame. */
#define MAX_BPF_REG	__MAX_BPF_REG

struct bpf_insn {
	__u8	code;		/* opcode */
	__u8	dst_reg:4;	/* dest register */
	__u8	src_reg:4;	/* source register */
	__s16	off;		/* signed offset */
	__s32	imm;		/* signed immediate constant */
};

/* BPF program can access up to 512 bytes of stack space. */
#define MAX_BPF_STACK	512

/* Helper macros for filter block array initializers. */

/* ALU ops on registers, bpf_add|sub|...: dst_reg += src_reg */

#define BPF_ALU64_REG_OFF(OP, DST, SRC, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_OP(OP) | BPF_X,	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

#define BPF_ALU64_REG(OP, DST, SRC)				\
	BPF_ALU64_REG_OFF(OP, DST, SRC, 0)

#define BPF_ALU32_REG_OFF(OP, DST, SRC, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_OP(OP) | BPF_X,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

#define BPF_ALU32_REG(OP, DST, SRC)				\
	BPF_ALU32_REG_OFF(OP, DST, SRC, 0)

/* ALU ops on immediates, bpf_add|sub|...: dst_reg += imm32 */

#define BPF_ALU64_IMM_OFF(OP, DST, IMM, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_OP(OP) | BPF_K,	\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = OFF,					\
		.imm   = IMM })
#define BPF_ALU64_IMM(OP, DST, IMM)				\
	BPF_ALU64_IMM_OFF(OP, DST, IMM, 0)

#define BPF_ALU32_IMM_OFF(OP, DST, IMM, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_OP(OP) | BPF_K,		\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = OFF,					\
		.imm   = IMM })
#define BPF_ALU32_IMM(OP, DST, IMM)				\
	BPF_ALU32_IMM_OFF(OP, DST, IMM, 0)

/* Endianess conversion, cpu_to_{l,b}e(), {l,b}e_to_cpu() */

#define BPF_ENDIAN(TYPE, DST, LEN)				\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_END | BPF_SRC(TYPE),	\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = LEN })

/* Byte Swap, bswap16/32/64 */

#define BPF_BSWAP(DST, LEN)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_END | BPF_SRC(BPF_TO_LE),	\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = LEN })

/* Short form of mov, dst_reg = src_reg */

#define BPF_MOV64_REG(DST, SRC)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_MOV | BPF_X,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = 0 })

#define BPF_MOV32_REG(DST, SRC)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_MOV | BPF_X,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = 0 })

#define BPF_MOV64_IMM(DST, IMM)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_MOV | BPF_K,		\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

#define BPF_MOV32_IMM(DST, IMM)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_MOV | BPF_K,		\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

/* Conditional jumps against registers, if (dst_reg 'op' src_reg) goto pc + off16 */

#define BPF_JMP_REG(OP, DST, SRC, OFF)				\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_OP(OP) | BPF_X,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

/* Conditional jumps against immediates, if (dst_reg 'op' imm32) goto pc + off16 */

#define BPF_JMP_IMM(OP, DST, IMM, OFF)				\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_OP(OP) | BPF_K,		\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = OFF,					\
		.imm   = IMM })

/* Like BPF_JMP_REG, but with 32-bit wide operands for comparison. */

#define BPF_JMP32_REG(OP, DST, SRC, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_JMP32 | BPF_OP(OP) | BPF_X,	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

/* Like BPF_JMP_IMM, but with 32-bit wide operands for comparison. */

#define BPF_JMP32_IMM(OP, DST, IMM, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_JMP32 | BPF_OP(OP) | BPF_K,	\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = OFF,					\
		.imm   = IMM })

/* Unconditional jumps, goto pc + off16 */

#define BPF_JMP_A(OFF)						\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_JA,			\
		.dst_reg = 0,					\
		.src_reg = 0,					\
		.off   = OFF,					\
		.imm   = 0 })

/* Unconditional jumps, gotol pc + imm32 */

#define BPF_JMP32_A(IMM)					\
	((struct bpf_insn) {					\
		.code  = BPF_JMP32 | BPF_JA,			\
		.dst_reg = 0,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

#define BPF_EXIT_INSN()						\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_EXIT,			\
		.dst_reg = 0,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = 0 })

/* Relative call */
#define BPF_CALL_REL(TGT)					\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_CALL,			\
		.dst_reg = 0,					\
		.src_reg = BPF_PSEUDO_CALL,			\
		.off   = 0,					\
		.imm   = TGT })

#endif /* _UAPI__LINUX_BPF_H__ */

#ifndef ALU_H
#define ALU_H

#include "vm_types.h"

// Instrucciones de control / sin operando
void inst_stop(VM *vm, const Instruction *inst);

// Instrucciones de 1 operando
void inst_sys(VM *vm, const Instruction *inst);
void inst_jmp(VM *vm, const Instruction *inst);
void inst_jp(VM *vm, const Instruction *inst);
void inst_jn(VM *vm, const Instruction *inst);
void inst_jz(VM *vm, const Instruction *inst);
void inst_jc(VM *vm, const Instruction *inst);
void inst_jv(VM *vm, const Instruction *inst);
void inst_jnp(VM *vm, const Instruction *inst);
void inst_jnn(VM *vm, const Instruction *inst);
void inst_jnz(VM *vm, const Instruction *inst);
void inst_not(VM *vm, const Instruction *inst);

// Instrucciones de 2 operandos (ALU / transferencia)
void inst_mov(VM *vm, const Instruction *inst);
void inst_add(VM *vm, const Instruction *inst);
void inst_sub(VM *vm, const Instruction *inst);
void inst_mul(VM *vm, const Instruction *inst);
void inst_div(VM *vm, const Instruction *inst);
void inst_cmp(VM *vm, const Instruction *inst);
void inst_and(VM *vm, const Instruction *inst);
void inst_or(VM *vm, const Instruction *inst);
void inst_xor(VM *vm, const Instruction *inst);
void inst_swap(VM *vm, const Instruction *inst);
void inst_shl(VM *vm, const Instruction *inst);
void inst_shr(VM *vm, const Instruction *inst);
void inst_sar(VM *vm, const Instruction *inst);
void inst_ldl(VM *vm, const Instruction *inst);
void inst_ldh(VM *vm, const Instruction *inst);
void inst_rnd(VM *vm, const Instruction *inst);

#endif // ALU_H


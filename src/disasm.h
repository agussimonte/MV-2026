#ifndef DISASM_H
#define DISASM_H

#include "vm_types.h"

// Imprime una única instrucción con el formato:
// [0000] XX XX ... | MNEM OP_A, OP_B[cite: 1]
void disasm_print_instruction(const VM *vm, uint16_t physical_addr, const Instruction *inst);

// Recorre todo el segmento de código e imprime el desensamblado completo[cite: 1]
void disasm_all(const VM *vm);

#endif // DISASM_H
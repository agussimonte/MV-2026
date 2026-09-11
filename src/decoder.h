#ifndef DECODER_H
#define DECODER_H

#include "vm_types.h"

// Decodifica la instrucción ubicada en la dirección física indicada
// Extrae opcode, cantidad y tipos de operandos, y sus valores crudos
Instruction decode_instruction(const VM *vm, uint16_t physical_addr);

#endif // DECODER_H
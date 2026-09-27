#ifndef DECODER_H
#define DECODER_H

#include "vm_types.h"

// Decodifica la instruccion ubicada en la direccion fisica indicada
// Extrae opcode, cantidad y tipos de operandos, y sus valores crudos
Instruction decode_instruction(const VM *vm, uint16_t physical_addr);

#endif // DECODER_H
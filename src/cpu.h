#ifndef CPU_H
#define CPU_H

#include "vm_types.h"

// Tipo de puntero a función para la tabla de despacho
typedef void (*InstructionHandler)(VM *vm, const Instruction *inst);

// Inicializa la tabla de despacho mapeando cada opcode a su función correspondiente
void cpu_init(void);

// Ejecuta un ciclo completo: fetch, decode, avance de IP y ejecución de la operación[cite: 1]
void cpu_step(VM *vm);

// Bucle principal: corre cpu_step() hasta que ocurra STOP o IP salga del segmento de código[cite: 1]
void cpu_run(VM *vm);

// Utilidades para consultar y encender/apagar los 4 bits altos del registro CC (N, Z, C, V)[cite: 1]
void cpu_update_flags(VM *vm, int64_t result, int32_t op_a, int32_t op_b, bool is_sub);

#endif // CPU_H
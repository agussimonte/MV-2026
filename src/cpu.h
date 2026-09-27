#ifndef CPU_H
#define CPU_H

#include "vm_types.h"

// Tipo de puntero a funcion para la tabla de despacho
typedef void (*InstructionHandler)(VM *vm, const Instruction *inst);

// Inicializa la tabla de despacho mapeando cada opcode a su funcion correspondiente
void cpu_init(void);

// Ejecuta un ciclo completo: fetch, decode, avance de IP y ejecucion de la operacion
void cpu_step(VM *vm);

// Bucle principal: corre cpu_step() hasta que ocurra STOP o IP salga del segmento de codigo
void cpu_run(VM *vm);

// Utilidades para consultar y encender/apagar los 4 bits altos del registro CC (N, Z, C, V)
void cpu_update_flags(VM *vm, int64_t result, int32_t op_a, int32_t op_b, bool is_sub);

#endif // CPU_H

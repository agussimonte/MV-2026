#ifndef MMU_H
#define MMU_H

#include "vm_types.h"

// Inicializa la memoria RAM en 0 y la tabla de segmentos con valores por defecto
void mmu_init(VM *vm);

// Traduce una direccion logica (Segmento:Offset) a fisica
// Retorna true si la direccion es valida dentro del segmento, o false si hay "Fallo de segmento"
bool mmu_logical_to_physical(const VM *vm, uint32_t logical_addr, uint8_t access_size, uint16_t *physical_addr);

// Lectura de memoria de datos (1, 2 o 4 bytes)
// Actualiza LAR, MAR y MBR. Si la direccion es invalida, aborta la ejecucion
uint32_t mem_read(VM *vm, uint32_t logical_addr, uint8_t size);

// Escritura de memoria de datos (1, 2 o 4 bytes)
// Actualiza LAR, MAR y MBR. Si la direccion es invalida, aborta la ejecucion
void mem_write(VM *vm, uint32_t logical_addr, uint32_t value, uint8_t size);

#endif // MMU_H
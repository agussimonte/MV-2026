#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mmu.h"

void mmu_init(VM *vm) {
    if (vm == NULL) {
        return;
    }

    // Inicializar memoria fisica en cero
    memset(vm->memory, 0, sizeof(vm->memory));

    // Inicializar tabla de descriptores de segmentos:
    // Las 8 entradas quedan sin utilizar con el valor -1 (0xFFFF en base y tamaño)
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        vm->segments[i].base = 0xFFFF;
        vm->segments[i].size = 0xFFFF;
    }
}

bool mmu_logical_to_physical(const VM *vm, uint32_t logical_addr, uint8_t access_size, uint16_t *physical_addr) {
    if (vm == NULL || physical_addr == NULL) {
        return false;
    }

    // Descomponer la direccion logica:
    // 16 bits más significativos: indice de segmento en la tabla de descriptores
    // 16 bits menos significativos: desplazamiento (offset) dentro del segmento
    uint16_t segment_base_dir = (uint16_t)((logical_addr >> 16) & 0xFFFF);
    uint16_t offset = (uint16_t)(logical_addr & 0xFFFF);

    // Valida que el indice no supere la cantidad de segmentos (0 a 7)
    if (segment_base_dir >= NUM_SEGMENTS) {
        return false;
    }

    const SegmentDescriptor *seg = &vm->segments[segment_base_dir]; 

    // Valida si el segmento esta sin utilizar (0xFFFF = -1)
    if (seg->base == 0xFFFF && seg->size == 0xFFFF) {
        return false;
    }

    // Validar limites del segmento: offset + access_size <= tamaño del segmento

    // Es lo mismo que (Base+Offset)+acces_size > Base + Tamaño   (Desbordamiento)
    if (access_size == 0 || (uint32_t)offset + access_size > seg->size) {
        return false;
    }

    // Validar que la direccion fisica calculada no sobrepase la RAM 
    uint32_t phys = (uint32_t)seg->base + offset;
    if (phys + access_size > RAM_SIZE) {
        return false;
    }

    *physical_addr = (uint16_t)phys;
    return true;
}

uint32_t mem_read(VM *vm, uint32_t logical_addr, uint8_t size) {
    if (vm == NULL) {
        fprintf(stderr, "Fallo de segmento\n");
        exit(1); // Se termino el programa debido a un error
    }

    // Cada acceso a memoria por datos debe cargar obligatoriamente:
    //  LAR: direccion logica solicitada
    //  MAR: parte alta con la cantidad de bytes a acceder
    vm->registers[REG_LAR] = (int32_t)logical_addr;
    vm->registers[REG_MAR] = ((int32_t)size << 16); // Si size = 4, queda 0x0100/0000

    uint16_t physical_addr = 0;
    if (!mmu_logical_to_physical(vm, logical_addr, size, &physical_addr)) {
        fprintf(stderr, "Fallo de segmento\n");
        exit(1);
    }

    // Actualizar parte baja de MAR con la direccion fisica que vino de mmu_logical_to...
    vm->registers[REG_MAR] |= (int32_t)physical_addr;

    //Lectura de los bytes de la RAM 
    uint32_t val = 0;
    if (size == 1) {
        val = (uint32_t)vm->memory[physical_addr];
    } else if (size == 2) {
        val = ((uint32_t)vm->memory[physical_addr] << 8) |  (uint32_t)vm->memory[physical_addr + 1];
    } else if (size == 4) {
        val = ((uint32_t)vm->memory[physical_addr] << 24) |
              ((uint32_t)vm->memory[physical_addr + 1] << 16) |
              ((uint32_t)vm->memory[physical_addr + 2] << 8) |
              (uint32_t)vm->memory[physical_addr + 3];
    } else {
        fprintf(stderr, "Fallo de segmento\n");
        exit(1);
    }

    // MBR almacena el valor obtenido despues de la lectura
    vm->registers[REG_MBR] = (int32_t)val;

    return val;
}

void mem_write(VM *vm, uint32_t logical_addr, uint32_t value, uint8_t size) {
    if (vm == NULL) {
        fprintf(stderr, "Fallo de segmento\n");
        exit(1);
    }

    // Cada acceso a memoria por datos debe cargar obligatoriamente:
    //   LAR: direccion logica solicitada
    //   MAR: parte alta con la cantidad de bytes a acceder
    //   MBR: valor que se desea almacenar
    vm->registers[REG_LAR] = (int32_t)logical_addr;
    vm->registers[REG_MAR] = ((int32_t)size << 16);
    vm->registers[REG_MBR] = (int32_t)value;

    uint16_t physical_addr = 0;
    if (!mmu_logical_to_physical(vm, logical_addr, size, &physical_addr)) {
        fprintf(stderr, "Fallo de segmento\n");
        exit(1);
    }

    // Actualiza la parte baja del MAR con la direccion fisica 
    vm->registers[REG_MAR] |= (int32_t)physical_addr;

    // Escritura en memoria 
    if (size == 1) {
        vm->memory[physical_addr] = (uint8_t)(value & 0xFF);
    } else if (size == 2) {
        vm->memory[physical_addr]     = (uint8_t)((value >> 8) & 0xFF);
        vm->memory[physical_addr + 1] = (uint8_t)(value & 0xFF);
    } else if (size == 4) {
        vm->memory[physical_addr]     = (uint8_t)((value >> 24) & 0xFF);
        vm->memory[physical_addr + 1] = (uint8_t)((value >> 16) & 0xFF);
        vm->memory[physical_addr + 2] = (uint8_t)((value >> 8) & 0xFF);
        vm->memory[physical_addr + 3] = (uint8_t)(value & 0xFF);
    } else {
        fprintf(stderr, "Fallo de segmento\n");
        exit(1);
    }
}


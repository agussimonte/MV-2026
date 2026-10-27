#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "mmu.h"
#include "decoder.h"
#include "alu.h"

// Tabla de despacho indexada por opcode (0x00 a 0x1F)
static InstructionHandler dispatch_table[32];

// Flags CC (N: bit 31, Z: bit 30, C: bit 29, V: bit 28)
void cpu_update_flags(VM *vm, int64_t result, int32_t op_a, int32_t op_b, bool is_sub) {
    int32_t truncated = (int32_t)(result & 0xFFFFFFFF);
    int32_t cc = 0;

    // N: bit 31 (resultado negativo)
    if (truncated < 0)
        cc |= (1 << 31);

    // Z: bit 30 (resultado cero)
    if (truncated == 0)
        cc |= (1 << 30);

    // C: bit 29 (acarreo / carry / borrow)
    if (is_sub) {
        // En resta / CMP: carry si el resultado es negativo o requirio prestamo
        if (result < 0 || (uint32_t)op_a < (uint32_t)op_b)
            cc |= (1 << 29);
    } else if (op_b != 0) {
        // En suma / MUL: carry si excede capacidad sin signo
        uint64_t u_sum = (uint64_t)(uint32_t)op_a + (uint64_t)(uint32_t)op_b;
        if (u_sum > 0xFFFFFFFFULL || (uint64_t)result > 0xFFFFFFFFULL)
            cc |= (1 << 29);
    }

    // V: bit 28 (desbordamiento con signo)
    if (is_sub) {
        if (((op_a ^ op_b) & 0x80000000) && ((op_a ^ truncated) & 0x80000000))
            cc |= (1 << 28);
    } else if (op_b != 0) {
        if (result < -2147483648LL || result > 2147483647LL ||
            (!((op_a ^ op_b) & 0x80000000) && ((op_a ^ truncated) & 0x80000000))) {
            cc |= (1 << 28);
        }
    }

    vm->registers[REG_CC] = cc;
}

// Inicializar tabla de despacho
void cpu_init(void) {
    memset(dispatch_table, 0, sizeof(dispatch_table));

    // Un operando
    dispatch_table[0x00] = inst_sys;
    dispatch_table[0x01] = inst_jmp;
    dispatch_table[0x02] = inst_jp;
    dispatch_table[0x03] = inst_jn;
    dispatch_table[0x04] = inst_jz;
    dispatch_table[0x05] = inst_jc;
    dispatch_table[0x06] = inst_jv;
    dispatch_table[0x07] = inst_jnp;
    dispatch_table[0x08] = inst_jnn;
    dispatch_table[0x09] = inst_jnz;
    dispatch_table[0x0A] = inst_not;
    dispatch_table[0x0B] = inst_push;
    dispatch_table[0x0C] = inst_pop;
    dispatch_table[0x0D] = inst_call;

    // Sin operandos
    dispatch_table[0x0E] = inst_ret;
    dispatch_table[0x0F] = inst_stop;

    // Dos operandos
    dispatch_table[0x10] = inst_mov;
    dispatch_table[0x11] = inst_add;
    dispatch_table[0x12] = inst_sub;
    dispatch_table[0x13] = inst_mul;
    dispatch_table[0x14] = inst_div;
    dispatch_table[0x15] = inst_cmp;
    dispatch_table[0x16] = inst_and;
    dispatch_table[0x17] = inst_or;
    dispatch_table[0x18] = inst_xor;
    dispatch_table[0x19] = inst_swap;
    dispatch_table[0x1A] = inst_shl;
    dispatch_table[0x1B] = inst_shr;
    dispatch_table[0x1C] = inst_sar;
    dispatch_table[0x1D] = inst_ldl;
    dispatch_table[0x1E] = inst_ldh;
    dispatch_table[0x1F] = inst_rnd;
}

// Un ciclo de instruccion: fetch, decode, cargar registros de control, avanzar IP y ejecutar
void cpu_step(VM *vm) {
    uint16_t phys_ip;
    if (!mmu_logical_to_physical(vm, (uint32_t)vm->registers[REG_IP], 1, &phys_ip)) {
        vm->running = false;
        return;
    }

    Instruction inst = decode_instruction(vm, phys_ip);

    // Cargar registros de control
    vm->registers[REG_OPC] = (int32_t)inst.opcode;
    vm->registers[REG_OP1] = inst.raw_op_a;
    vm->registers[REG_OP2] = inst.raw_op_b;

    // Avanzar IP
    vm->registers[REG_IP] += inst.size_in_bytes;

    // Validar opcode y despachar
    if (inst.opcode > 0x1F || dispatch_table[inst.opcode] == NULL) {
        fprintf(stderr, "Instruccion invalida\n");
        exit(1);
    }

    dispatch_table[inst.opcode](vm, &inst);
}

// Bucle principal de ejecucion
void cpu_run(VM *vm) {
    while (vm->running) {
        if ((uint32_t)vm->registers[REG_IP] == 0xFFFFFFFF) {
            vm->running = false;
            break;
        }

        uint16_t cs_idx = (uint16_t)(((uint32_t)vm->registers[REG_CS] >> 16) & 0xFFFF);
        uint16_t cs_size = (cs_idx < NUM_SEGMENTS && vm->segments[cs_idx].base != 0xFFFF)
                           ? vm->segments[cs_idx].size : vm->segments[0].size;
        uint16_t ip_seg = (uint16_t)(((uint32_t)vm->registers[REG_IP] >> 16) & 0xFFFF);
        uint16_t ip_offset = (uint16_t)(vm->registers[REG_IP] & 0xFFFF);

        if (ip_seg != cs_idx || ip_offset >= cs_size) {
            vm->running = false;
            break;
        }

        cpu_step(vm);
    }
}

// Inicializa SP y carga el frame inicial en la pila (RET=-1, argc, argv)
void cpu_init_stack(VM *vm, int argc, uint32_t argv_ptr) {
    uint32_t ss_val = (uint32_t)vm->registers[REG_SS];
    // Si SS no existe (-1) o no fue inicializado (0 en binarios v1), no hay pila
    if (ss_val == 0xFFFFFFFF || ss_val == 0) {
        return;
    }

    uint16_t ss_idx = (uint16_t)((ss_val >> 16) & 0xFFFF);
    if (ss_idx >= NUM_SEGMENTS || vm->segments[ss_idx].base == 0xFFFF) {
        return; // No hay Stack Segment definido
    }

    uint16_t stack_size = vm->segments[ss_idx].size;
    // Inicialmente SP apunta al byte fuera del Stack Segment (SS + tamano)
    vm->registers[REG_SP] = (int32_t)((uint32_t)vm->registers[REG_SS] + stack_size);

    // 1. Apilar puntero a argv (o -1 si no hay)
    vm->registers[REG_SP] -= 4;
    mem_write(vm, (uint32_t)vm->registers[REG_SP], argv_ptr, 4);

    // 2. Apilar argc
    vm->registers[REG_SP] -= 4;
    mem_write(vm, (uint32_t)vm->registers[REG_SP], (uint32_t)argc, 4);

    // 3. Apilar direccion de retorno (-1) para que RET en main termine el proceso
    vm->registers[REG_SP] -= 4;
    mem_write(vm, (uint32_t)vm->registers[REG_SP], 0xFFFFFFFF, 4);
}

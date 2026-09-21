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
        // En resta, carry se activa si hay prestamo (op_a < op_b como sin signo o res < 0)
        if (result < 0 || (uint32_t)op_a < (uint32_t)op_b)
            cc |= (1 << 29);
    } else {
        // En suma/mult/etc, se activa si el resultado excede 32 bits sin signo
        if ((uint64_t)result > 0xFFFFFFFFULL)
            cc |= (1 << 29);
    }

    // V: bit 28 (desbordamiento con signo en complemento a 2)
    if (result < -2147483648LL || result > 2147483647LL) {
        cc |= (1 << 28);
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

    // Sin operandos
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
        fprintf(stderr, "Instrucción inválida\n");
        exit(1);
    }

    dispatch_table[inst.opcode](vm, &inst);
}

// Bucle principal de ejecucion
void cpu_run(VM *vm) {
    while (vm->running) {
        uint16_t cs_size = vm->segments[0].size;
        uint16_t ip_offset = (uint16_t)(vm->registers[REG_IP] & 0xFFFF);

        if (ip_offset >= cs_size) {
            vm->running = false;
            break;
        }

        cpu_step(vm);
    }
}

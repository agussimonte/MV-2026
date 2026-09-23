#include <stdio.h>
#include <stdlib.h>
#include "alu.h"
#include "cpu.h"
#include "mmu.h"

// Punteros a funcion para acceso a operandos
typedef int32_t (*GetOperandFn)(VM *vm, uint8_t reg, int16_t val);
typedef void (*SetOperandFn)(VM *vm, uint8_t reg, int16_t val, int32_t result);

static int32_t get_none(VM *vm, uint8_t reg, int16_t val) {
    (void)vm; (void)reg; (void)val;
    return 0;
}

static int32_t get_reg(VM *vm, uint8_t reg, int16_t val) {
    (void)val;
    return vm->registers[reg];
}

static int32_t get_imm(VM *vm, uint8_t reg, int16_t val) {
    (void)vm; (void)reg;
    return (int32_t)val;
}

static int32_t get_mem(VM *vm, uint8_t reg, int16_t val) {
    uint32_t base = (uint32_t)vm->registers[reg];
    uint32_t segment = base & 0xFFFF0000;
    uint32_t offset = (base & 0x0000FFFF) + (int32_t)val;
    uint32_t logical = segment | (offset & 0xFFFF);
    return (int32_t)mem_read(vm, logical, 4);
}

static GetOperandFn get_value[] = { get_none, get_reg, get_imm, get_mem };

static void set_none(VM *vm, uint8_t reg, int16_t val, int32_t result) {
    (void)vm; (void)reg; (void)val; (void)result;
}

static void set_reg(VM *vm, uint8_t reg, int16_t val, int32_t result) {
    (void)val;
    vm->registers[reg] = result;
}

static void set_imm(VM *vm, uint8_t reg, int16_t val, int32_t result) {
    (void)vm; (void)reg; (void)val; (void)result;
}

static void set_mem(VM *vm, uint8_t reg, int16_t val, int32_t result) {
    uint32_t base = (uint32_t)vm->registers[reg];
    uint32_t segment = base & 0xFFFF0000;
    uint32_t offset = (base & 0x0000FFFF) + (int32_t)val;
    uint32_t logical = segment | (offset & 0xFFFF);
    mem_write(vm, logical, (uint32_t)result, 4);
}

static SetOperandFn set_value[] = { set_none, set_reg, set_imm, set_mem };

// STOP (0x0F)
void inst_stop(VM *vm, const Instruction *inst) {
    (void)inst;
    vm->registers[REG_IP] = -1;
    vm->running = false;
}

// MOV (0x10): A = B
void inst_mov(VM *vm, const Instruction *inst) {
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, b);
    cpu_update_flags(vm, (int64_t)b, b, 0, false);
}

// ADD (0x11): A = A + B
void inst_add(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    int64_t res = (int64_t)a + (int64_t)b;
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, (int32_t)res);
    cpu_update_flags(vm, res, a, b, false);
}

// SUB (0x12): A = A - B
void inst_sub(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    int64_t res = (int64_t)a - (int64_t)b;
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, (int32_t)res);
    cpu_update_flags(vm, res, a, b, true);
}

// MUL (0x13): A = A * B
void inst_mul(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    int64_t res = (int64_t)a * (int64_t)b;
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, (int32_t)res);
    cpu_update_flags(vm, res, a, b, false);
}

// DIV (0x14): A = A / B
void inst_div(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);

    if (b == 0) {
        fprintf(stderr, "División por cero\n");
        exit(1);
    }

    int64_t res = (int64_t)a / (int64_t)b;
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, (int32_t)res);
    cpu_update_flags(vm, res, a, b, false);
}

// CMP (0x15): compara A con B (como SUB pero sin guardar resultado)
void inst_cmp(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    int64_t res = (int64_t)a - (int64_t)b;
    cpu_update_flags(vm, res, a, b, true);
}

// AND (0x16): A = A & B
void inst_and(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    int32_t res = a & b;
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, res);
    cpu_update_flags(vm, (int64_t)res, a, b, false);
}

// OR (0x17): A = A | B
void inst_or(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    int32_t res = a | b;
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, res);
    cpu_update_flags(vm, (int64_t)res, a, b, false);
}

// XOR (0x18): A = A ^ B
void inst_xor(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    int32_t res = a ^ b;
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, res);
    cpu_update_flags(vm, (int64_t)res, a, b, false);
}

// SWAP (0x19): intercambia A y B
void inst_swap(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, b);
    set_value[inst->type_b](vm, inst->reg_b, inst->val_b, a);
    cpu_update_flags(vm, (int64_t)b, b, 0, false);
}

// SHL (0x1A): shift logico izquierda
void inst_shl(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    int64_t res = (int64_t)((uint32_t)a << b);
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, (int32_t)res);
    cpu_update_flags(vm, res, a, b, false);
}

// SHR (0x1B): shift logico derecha
void inst_shr(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    uint32_t res = (uint32_t)a >> b;
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, (int32_t)res);
    cpu_update_flags(vm, (int64_t)(int32_t)res, a, b, false);
}

// SAR (0x1C): shift aritmetico (mantiene signo)
void inst_sar(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    int32_t res = a >> b;
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, res);
    cpu_update_flags(vm, (int64_t)res, a, b, false);
}

// LDL (0x1D): carga 16 bits bajos
void inst_ldl(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    int32_t res = (a & (int32_t)0xFFFF0000) | (b & 0x0000FFFF);
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, res);
}

// LDH (0x1E): carga 16 bits altos
void inst_ldh(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    int32_t res = (a & 0x0000FFFF) | ((b & 0x0000FFFF) << 16);
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, res);
}

// RND (0x1F): numero aleatorio entre 0 y B
void inst_rnd(VM *vm, const Instruction *inst) {
    int32_t b = get_value[inst->type_b](vm, inst->reg_b, inst->val_b);
    int32_t res = (b != 0) ? (rand() % (b + 1)) : 0;
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, res);
}

// NOT (0x0A): A = ~A
void inst_not(VM *vm, const Instruction *inst) {
    int32_t a = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t res = ~a;
    set_value[inst->type_a](vm, inst->reg_a, inst->val_a, res);
    cpu_update_flags(vm, (int64_t)res, a, 0, false);
}

// JMP (0x01): salto incondicional
void inst_jmp(VM *vm, const Instruction *inst) {
    vm->registers[REG_IP] = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
}

// JP (0x02): positivo (N=0 y Z=0)
void inst_jp(VM *vm, const Instruction *inst) {
    int32_t cc = vm->registers[REG_CC];
    if (!(cc & (1 << 31)) && !(cc & (1 << 30)))
        vm->registers[REG_IP] = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
}

// JN (0x03): negativo (N=1)
void inst_jn(VM *vm, const Instruction *inst) {
    if (vm->registers[REG_CC] & (1 << 31))
        vm->registers[REG_IP] = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
}

// JZ (0x04): cero (Z=1)
void inst_jz(VM *vm, const Instruction *inst) {
    if (vm->registers[REG_CC] & (1 << 30))
        vm->registers[REG_IP] = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
}

// JC (0x05): carry (C=1)
void inst_jc(VM *vm, const Instruction *inst) {
    if (vm->registers[REG_CC] & (1 << 29))
        vm->registers[REG_IP] = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
}

// JV (0x06): overflow (V=1)
void inst_jv(VM *vm, const Instruction *inst) {
    if (vm->registers[REG_CC] & (1 << 28))
        vm->registers[REG_IP] = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
}

// JNP (0x07): no positivo (N=1 o Z=1)
void inst_jnp(VM *vm, const Instruction *inst) {
    int32_t cc = vm->registers[REG_CC];
    if ((cc & (1 << 31)) || (cc & (1 << 30)))
        vm->registers[REG_IP] = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
}

// JNN (0x08): no negativo (N=0)
void inst_jnn(VM *vm, const Instruction *inst) {
    if (!(vm->registers[REG_CC] & (1 << 31)))
        vm->registers[REG_IP] = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
}

// JNZ (0x09): no cero (Z=0)
void inst_jnz(VM *vm, const Instruction *inst) {
    if (!(vm->registers[REG_CC] & (1 << 30)))
        vm->registers[REG_IP] = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
}

// Funcion auxiliar para imprimir en binario
static void print_binary(uint32_t val, uint16_t cell_size) {
    int bits = cell_size * 8;
    for (int i = bits - 1; i >= 0; i--) {
        putchar((val & (1U << i)) ? '1' : '0');
    }
}

// SYS (0x00): llamadas al sistema
// Operando A (inst): 1 = READ, 2 = WRITE
// EAX: formato (bit 0: dec, bit 1: char, bit 2: oct, bit 3: hex, bit 4: bin)
// EDX: direccion logica inicial
// ECX: 16 bits bajos = cantidad de celdas, 16 bits altos = tamano de celda
void inst_sys(VM *vm, const Instruction *inst) {
    int32_t syscall_op = get_value[inst->type_a](vm, inst->reg_a, inst->val_a);
    int32_t format = vm->registers[REG_EAX];
    uint32_t base_addr = (uint32_t)vm->registers[REG_EDX];
    uint16_t count = (uint16_t)(vm->registers[REG_ECX] & 0xFFFF);
    uint16_t cell_size = (uint16_t)((vm->registers[REG_ECX] >> 16) & 0xFFFF);

    if (cell_size != 1 && cell_size != 2 && cell_size != 4) {
        cell_size = 4;
    }

    uint32_t segment = base_addr & 0xFFFF0000;
    uint32_t start_offset = base_addr & 0x0000FFFF;

    for (uint16_t i = 0; i < count; i++) {
        uint32_t offset = start_offset + (uint32_t)(i * cell_size);
        uint32_t logical = segment | (offset & 0xFFFF);

        uint16_t phys;
        if (!mmu_logical_to_physical(vm, logical, cell_size, &phys)) {
            fprintf(stderr, "Fallo de segmento\n");
            exit(1);
        }

        if (syscall_op == 1) {
            // READ: leer del teclado hacia la memoria
            printf("[%04X]: ", phys);

            int32_t input = 0;
            if (format & 0x02) {
                // Caracter
                char c = 0;
                if (scanf(" %c", &c) == 1) {
                    input = (uint8_t)c;
                }
            } else if (format & 0x04) {
                // Octal
                unsigned int oct = 0;
                if (scanf("%o", &oct) == 1) {
                    input = (int32_t)oct;
                }
            } else if (format & 0x08) {
                // Hexadecimal
                unsigned int hex = 0;
                if (scanf("%x", &hex) == 1) {
                    input = (int32_t)hex;
                }
            } else if (format & 0x10) {
                // Binario: lee cadena de 0 y 1
                char bin_str[35];
                if (scanf("%34s", bin_str) == 1) {
                    input = (int32_t)strtol(bin_str, NULL, 2);
                }
            } else {
                // Decimal por defecto (bit 0 o si no coincide ninguno)
                if (scanf("%d", &input) != 1) {
                    input = 0;
                }
            }
            mem_write(vm, logical, (uint32_t)input, cell_size);

        } else if (syscall_op == 2) {
            // WRITE: mostrar de la memoria hacia la pantalla
            uint32_t val = mem_read(vm, logical, cell_size);
            printf("[%04X]: ", phys);

            if (format & 0x02) {
                printf("%c\n", (char)(val & 0xFF));
            } else if (format & 0x04) {
                printf("%o\n", val);
            } else if (format & 0x08) {
                printf("%X\n", val);
            } else if (format & 0x10) {
                print_binary(val, cell_size);
                printf("\n");
            } else {
                // Decimal
                printf("%d\n", (int32_t)val);
            }
        }
    }
}


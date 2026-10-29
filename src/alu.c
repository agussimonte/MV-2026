#include <stdio.h>
#include <stdlib.h>
#include "alu.h"
#include "cpu.h"
#include "mmu.h"
#include "vmi.h"

// Acceso a operandos: lectura con soporte de subregistros y modificadores de tamaño
static int32_t get_operand(VM *vm, OperandType type, uint8_t reg, int16_t val, uint8_t sector, uint8_t mem_size) {
    switch (type) {
        case OP_REG: {
            int32_t r = vm->registers[reg];
            switch (sector) {
                case REG_SECTOR_L: // 4to byte / LSB (AL..FL)
                    return (int32_t)(int8_t)(r & 0xFF);
                case REG_SECTOR_H: // 3er byte / byte alto de AX (AH..FH)
                    return (int32_t)(int8_t)((r >> 8) & 0xFF);
                case REG_SECTOR_X: // 2 bytes menos significativos (AX..FX)
                    return (int32_t)(int16_t)(r & 0xFFFF);
                case REG_SECTOR_FULL:
                default:
                    return r;
            }
        }
        case OP_IMM:
            return (int32_t)val;
        case OP_MEM: {
            uint32_t base = (uint32_t)vm->registers[reg];
            uint32_t segment = base & 0xFFFF0000;
            uint32_t offset = (base & 0x0000FFFF) + (int32_t)val;
            uint32_t logical = segment | (offset & 0xFFFF);
            uint8_t size = (mem_size == 1 || mem_size == 2 || mem_size == 4) ? mem_size : 4;
            uint32_t raw = mem_read(vm, logical, size);
            if (size == 1) {
                return (int32_t)(int8_t)(raw & 0xFF);
            } else if (size == 2) {
                return (int32_t)(int16_t)(raw & 0xFFFF);
            } else {
                return (int32_t)raw;
            }
        }
        case OP_NONE:
        default:
            return 0;
    }
}

// Acceso a operandos: escritura con soporte de subregistros y modificadores de tamaño
static void set_operand(VM *vm, OperandType type, uint8_t reg, int16_t val, uint8_t sector, uint8_t mem_size, int32_t result) {
    switch (type) {
        case OP_REG: {
            switch (sector) {
                case REG_SECTOR_L: // Modifica solo el 4to byte (bits 0..7)
                    vm->registers[reg] = (int32_t)((vm->registers[reg] & ~0x000000FF) | (result & 0xFF));
                    break;
                case REG_SECTOR_H: // Modifica solo el 3er byte (bits 8..15)
                    vm->registers[reg] = (int32_t)((vm->registers[reg] & ~0x0000FF00) | ((result & 0xFF) << 8));
                    break;
                case REG_SECTOR_X: // Modifica solo los 2 bytes menos significativos (bits 0..15)
                    vm->registers[reg] = (int32_t)((vm->registers[reg] & ~0x0000FFFF) | (result & 0xFFFF));
                    break;
                case REG_SECTOR_FULL:
                default:
                    vm->registers[reg] = result;
                    break;
            }
            break;
        }
        case OP_MEM: {
            uint32_t base = (uint32_t)vm->registers[reg];
            uint32_t segment = base & 0xFFFF0000;
            uint32_t offset = (base & 0x0000FFFF) + (int32_t)val;
            uint32_t logical = segment | (offset & 0xFFFF);
            uint8_t size = (mem_size == 1 || mem_size == 2 || mem_size == 4) ? mem_size : 4;
            mem_write(vm, logical, (uint32_t)result, size);
            break;
        }
        case OP_IMM:
        case OP_NONE:
        default:
            break;
    }
}

static inline int32_t get_op_a(VM *vm, const Instruction *inst) {
    return get_operand(vm, inst->type_a, inst->reg_a, inst->val_a, inst->sector_a, inst->mem_size_a);
}

static inline int32_t get_op_b(VM *vm, const Instruction *inst) {
    return get_operand(vm, inst->type_b, inst->reg_b, inst->val_b, inst->sector_b, inst->mem_size_b);
}

static inline void set_op_a(VM *vm, const Instruction *inst, int32_t result) {
    set_operand(vm, inst->type_a, inst->reg_a, inst->val_a, inst->sector_a, inst->mem_size_a, result);
}

static inline void set_op_b(VM *vm, const Instruction *inst, int32_t result) {
    set_operand(vm, inst->type_b, inst->reg_b, inst->val_b, inst->sector_b, inst->mem_size_b, result);
}

// STOP (0x0F)
void inst_stop(VM *vm, const Instruction *inst) {
    (void)inst;
    vm->registers[REG_IP] = -1;
    vm->running = false;
}

// MOV (0x10): A = B
void inst_mov(VM *vm, const Instruction *inst) {
    int32_t b = get_op_b(vm, inst);
    set_op_a(vm, inst, b);
    cpu_update_flags(vm, (int64_t)b, b, 0, false);
}

// ADD (0x11): A = A + B
void inst_add(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    int64_t res = (int64_t)a + (int64_t)b;
    set_op_a(vm, inst, (int32_t)res);
    cpu_update_flags(vm, res, a, b, false);
}

// SUB (0x12): A = A - B
void inst_sub(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    int64_t res = (int64_t)a - (int64_t)b;
    set_op_a(vm, inst, (int32_t)res);
    cpu_update_flags(vm, res, a, b, true);
}

// MUL (0x13): A = A * B
void inst_mul(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    int64_t res = (int64_t)a * (int64_t)b;
    set_op_a(vm, inst, (int32_t)res);
    cpu_update_flags(vm, res, a, b, false);
}

// DIV (0x14): A = A / B
void inst_div(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);

    if (b == 0) {
        fprintf(stderr, "Division por cero\n");
        exit(1);
    }

    int64_t res = (int64_t)a / (int64_t)b;
    vm->registers[REG_AC] = (int32_t)((int64_t)a % (int64_t)b);
    set_op_a(vm, inst, (int32_t)res);
    cpu_update_flags(vm, res, a, b, false);
}

// CMP (0x15): compara A con B (como SUB pero sin guardar resultado)
void inst_cmp(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    int64_t res = (int64_t)a - (int64_t)b;
    cpu_update_flags(vm, res, a, b, true);
}

// AND (0x16): A = A & B
void inst_and(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    int32_t res = a & b;
    set_op_a(vm, inst, res);
    cpu_update_flags(vm, (int64_t)res, a, b, false);
}

// OR (0x17): A = A | B
void inst_or(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    int32_t res = a | b;
    set_op_a(vm, inst, res);
    cpu_update_flags(vm, (int64_t)res, a, b, false);
}

// XOR (0x18): A = A ^ B
void inst_xor(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    int32_t res = a ^ b;
    set_op_a(vm, inst, res);
    cpu_update_flags(vm, (int64_t)res, a, b, false);
}

// SWAP (0x19): intercambia A y B
void inst_swap(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    set_op_a(vm, inst, b);
    set_op_b(vm, inst, a);
    cpu_update_flags(vm, (int64_t)b, b, 0, false);
}

// SHL (0x1A): shift logico izquierda
void inst_shl(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    int64_t res = (int64_t)((uint32_t)a << b);
    set_op_a(vm, inst, (int32_t)res);
    cpu_update_flags(vm, res, a, b, false);
}

// SHR (0x1B): shift logico derecha
void inst_shr(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    uint32_t res = (uint32_t)a >> b;
    set_op_a(vm, inst, (int32_t)res);
    cpu_update_flags(vm, (int64_t)(int32_t)res, a, b, false);
}

// SAR (0x1C): shift aritmetico (mantiene signo)
void inst_sar(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    int32_t res = a >> b;
    set_op_a(vm, inst, res);
    cpu_update_flags(vm, (int64_t)res, a, b, false);
}

// LDL (0x1D): carga 16 bits bajos
void inst_ldl(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    int32_t res = (a & (int32_t)0xFFFF0000) | (b & 0x0000FFFF);
    set_op_a(vm, inst, res);
}

// LDH (0x1E): carga 16 bits altos
void inst_ldh(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t b = get_op_b(vm, inst);
    int32_t res = (a & 0x0000FFFF) | ((b & 0x0000FFFF) << 16);
    set_op_a(vm, inst, res);
}

// RND (0x1F): numero aleatorio entre 0 y B
void inst_rnd(VM *vm, const Instruction *inst) {
    int32_t b = get_op_b(vm, inst);
    int32_t res = (b != 0) ? (rand() % (b + 1)) : 0;
    set_op_a(vm, inst, res);
}

// NOT (0x0A): A = ~A
void inst_not(VM *vm, const Instruction *inst) {
    int32_t a = get_op_a(vm, inst);
    int32_t res = ~a;
    set_op_a(vm, inst, res);
    cpu_update_flags(vm, (int64_t)res, a, 0, false);
}

// JMP (0x01): salto incondicional
void inst_jmp(VM *vm, const Instruction *inst) {
    vm->registers[REG_IP] = get_op_a(vm, inst);
}

// JP (0x02): positivo (N=0 y Z=0)
void inst_jp(VM *vm, const Instruction *inst) {
    int32_t cc = vm->registers[REG_CC];
    if (!(cc & (1 << 31)) && !(cc & (1 << 30)))
        vm->registers[REG_IP] = get_op_a(vm, inst);
}

// JN (0x03): negativo (N=1)
void inst_jn(VM *vm, const Instruction *inst) {
    if (vm->registers[REG_CC] & (1 << 31))
        vm->registers[REG_IP] = get_op_a(vm, inst);
}

// JZ (0x04): cero (Z=1)
void inst_jz(VM *vm, const Instruction *inst) {
    if (vm->registers[REG_CC] & (1 << 30))
        vm->registers[REG_IP] = get_op_a(vm, inst);
}

// JC (0x05): carry (C=1)
void inst_jc(VM *vm, const Instruction *inst) {
    if (vm->registers[REG_CC] & (1 << 29))
        vm->registers[REG_IP] = get_op_a(vm, inst);
}

// JV (0x06): overflow (V=1)
void inst_jv(VM *vm, const Instruction *inst) {
    if (vm->registers[REG_CC] & (1 << 28))
        vm->registers[REG_IP] = get_op_a(vm, inst);
}

// JNP (0x07): no positivo (N=1 o Z=1)
void inst_jnp(VM *vm, const Instruction *inst) {
    int32_t cc = vm->registers[REG_CC];
    if ((cc & (1 << 31)) || (cc & (1 << 30)))
        vm->registers[REG_IP] = get_op_a(vm, inst);
}

// JNN (0x08): no negativo (N=0)
void inst_jnn(VM *vm, const Instruction *inst) {
    if (!(vm->registers[REG_CC] & (1 << 31)))
        vm->registers[REG_IP] = get_op_a(vm, inst);
}

// JNZ (0x09): no cero (Z=0)
void inst_jnz(VM *vm, const Instruction *inst) {
    if (!(vm->registers[REG_CC] & (1 << 30)))
        vm->registers[REG_IP] = get_op_a(vm, inst);
}

// Limite superior de la pila (SS + tamano de segmento)
static uint32_t get_stack_max(const VM *vm) {
    uint16_t ss_index = (uint16_t)(((uint32_t)vm->registers[REG_SS] >> 16) & 0xFFFF);
    if (ss_index >= NUM_SEGMENTS || vm->segments[ss_index].base == 0xFFFF) {
        return (uint32_t)vm->registers[REG_SS];
    }
    return (uint32_t)vm->registers[REG_SS] + vm->segments[ss_index].size;
}

// PUSH (0x0B): apilar dato de 4 bytes
void inst_push(VM *vm, const Instruction *inst) {
    vm->registers[REG_SP] -= 4;
    if ((uint32_t)vm->registers[REG_SP] < (uint32_t)vm->registers[REG_SS]) {
        fprintf(stderr, "Stack overflow\n");
        exit(1);
    }
    int32_t val = get_op_a(vm, inst);
    mem_write(vm, (uint32_t)vm->registers[REG_SP], (uint32_t)val, 4);
}

// POP (0x0C): desapilar dato de 4 bytes
void inst_pop(VM *vm, const Instruction *inst) {
    uint32_t sp_max = get_stack_max(vm);
    if ((uint32_t)vm->registers[REG_SP] + 4 > sp_max) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    uint32_t val = mem_read(vm, (uint32_t)vm->registers[REG_SP], 4);
    set_op_a(vm, inst, (int32_t)val);
    vm->registers[REG_SP] += 4;
}

// CALL (0x0D): llamada a subrutina (PUSH IP y salto a CS + operando)
void inst_call(VM *vm, const Instruction *inst) {
    vm->registers[REG_SP] -= 4;
    if ((uint32_t)vm->registers[REG_SP] < (uint32_t)vm->registers[REG_SS]) {
        fprintf(stderr, "Stack overflow\n");
        exit(1);
    }
    mem_write(vm, (uint32_t)vm->registers[REG_SP], (uint32_t)vm->registers[REG_IP], 4);

    int32_t target = get_op_a(vm, inst);
    uint32_t cs_segment = (uint32_t)vm->registers[REG_CS] & 0xFFFF0000;
    vm->registers[REG_IP] = (int32_t)(cs_segment | ((uint32_t)target & 0x0000FFFF));
}

// RET (0x0E): retorno de subrutina (POP IP)
void inst_ret(VM *vm, const Instruction *inst) {
    (void)inst;
    uint32_t sp_max = get_stack_max(vm);
    if ((uint32_t)vm->registers[REG_SP] + 4 > sp_max) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    uint32_t ret_ip = mem_read(vm, (uint32_t)vm->registers[REG_SP], 4);
    vm->registers[REG_SP] += 4;
    vm->registers[REG_IP] = (int32_t)ret_ip;

    if (ret_ip == 0xFFFFFFFF) {
        vm->running = false;
    }
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
// ECX: 16 bits bajos = cantidad de celdas, 16 bits altos = tamaño de celda
void inst_sys(VM *vm, const Instruction *inst) {
    int32_t syscall_op = get_op_a(vm, inst);

    if (syscall_op == 0x00) {
        // CLEAR SCREEN (0x00): limpia la pantalla de la consola
        printf("\033[H\033[2J\033[3J");
        fflush(stdout);
        return;
    }

    if (syscall_op == 0x03) {
        // STRING READ (0x03): lee del teclado y guarda en [EDX] hasta '\0'
        uint32_t base_addr = (uint32_t)vm->registers[REG_EDX];
        uint32_t segment = base_addr & 0xFFFF0000;
        uint32_t start_offset = base_addr & 0x0000FFFF;
        int32_t max_chars = vm->registers[REG_ECX]; // -1 = sin limite

        int c;
        int32_t read_count = 0;
        while ((c = getchar()) != EOF && c != '\n') {
            if (max_chars != -1 && read_count >= max_chars) {
                // Descartar el resto de la linea
                while (c != '\n' && (c = getchar()) != EOF);
                break;
            }
            uint32_t offset = (start_offset + (uint32_t)read_count) & 0xFFFF;
            mem_write(vm, segment | offset, (uint8_t)c, 1);
            read_count++;
        }
        // Guardar terminador '\0'
        uint32_t null_offset = (start_offset + (uint32_t)read_count) & 0xFFFF;
        mem_write(vm, segment | null_offset, 0x00, 1);
        return;
    }

    if (syscall_op == 0x04) {
        // STRING WRITE (0x04): imprime string desde [EDX] hasta encontrar '\0'
        uint32_t base_addr = (uint32_t)vm->registers[REG_EDX];
        uint32_t segment = base_addr & 0xFFFF0000;
        uint32_t offset = base_addr & 0x0000FFFF;

        while (1) {
            uint32_t byte_val = mem_read(vm, segment | (offset & 0xFFFF), 1);
            char ch = (char)(byte_val & 0xFF);
            if (ch == '\0') {
                break;
            }
            putchar(ch);
            offset++;
        }
        fflush(stdout);
        return;
    }

    if (syscall_op == 0x0F) {
        // BREAKPOINT (0x0F): pausa y genera imagen .vmi para depuracion
        vmi_handle_breakpoint(vm);
        return;
    }

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
            // Escribo en memoria
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

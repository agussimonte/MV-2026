#include <stdio.h>
#include <string.h>
#include "disasm.h"
#include "decoder.h"

//tabla de nombres de registros (32 registros en total)
//facil de actualizar para la segunda parte
static const char *register_names[NUM_REGISTERS] = {
    [REG_IP]  = "IP",  [REG_OPC] = "OPC", [REG_OP1] = "OP1", [REG_OP2] = "OP2",
    [REG_LAR] = "LAR", [REG_MAR] = "MAR", [REG_MBR] = "MBR",
    [REG_EAX] = "EAX", [REG_EBX] = "EBX", [REG_ECX] = "ECX",
    [REG_EDX] = "EDX", [REG_EEX] = "EEX", [REG_EFX] = "EFX",
    [REG_AC]  = "AC",  [REG_CC]  = "CC",
    [REG_CS]  = "CS",  [REG_DS]  = "DS"
};

//tabla de nombres de mnemonicos (0x00 a 0x1F)
static const char *mnemonic_names[32] = {
    [OP_SYS]  = "SYS",  [OP_JMP]  = "JMP",  [OP_JP]   = "JP",   [OP_JN]   = "JN",
    [OP_JZ]   = "JZ",   [OP_JC]   = "JC",   [OP_JV]   = "JV",   [OP_JNP]  = "JNP",
    [OP_JNN]  = "JNN",  [OP_JNZ]  = "JNZ",  [OP_NOT]  = "NOT",  [OP_STOP] = "STOP",
    [OP_MOV]  = "MOV",  [OP_ADD]  = "ADD",  [OP_SUB]  = "SUB",  [OP_MUL]  = "MUL",
    [OP_DIV]  = "DIV",  [OP_CMP]  = "CMP",  [OP_AND]  = "AND",  [OP_OR]   = "OR",
    [OP_XOR]  = "XOR",  [OP_SWAP] = "SWAP", [OP_SHL]  = "SHL",  [OP_SHR]  = "SHR",
    [OP_SAR]  = "SAR",  [OP_LDL]  = "LDL",  [OP_LDH]  = "LDH",  [OP_RND]  = "RND"
};

static const char *get_register_name(uint8_t reg){
    if (reg < NUM_REGISTERS && register_names[reg] != NULL){
        return register_names[reg];
    }
    return "???"; //para registros invalidos o aun no aplicados
}

static const char *get_mnemonic(uint8_t opcode){
    if (opcode < 32 && mnemonic_names[opcode] != NULL){
        return mnemonic_names[opcode];
    }
    return "???";
}

//imprime un operando individual en formato Assembler
static void print_operand(OperandType type, uint8_t reg, int16_t val){
    if (type == OP_REG){
        printf("%s", get_register_name(reg)); //[EAX]
    }else if (type == OP_IMM){
        printf("%d", val);
    }else if (type == OP_MEM){ 
        if (reg != 0){
            if (val > 0){
                printf("[%s+%d]", get_register_name(reg), val); //ej: [DS+5]
            }else if (val < 0){
                printf("[%s%d]", get_register_name(reg), val); //[DS-5]
            }else{
                printf("[%s]", get_register_name(reg)); //[DS]
            }
        }else{
            printf("[%d]", val); //[8]
        }
    }
}

void disasm_print_instruction(const VM *vm, uint16_t physical_addr, const Instruction *inst) {
    if (vm == NULL || inst == NULL){
        return;
    }

    // 1.Imprime la direccion física de 4 dígitos hexadecimales: [XXXX]
    printf("[%04X] ", physical_addr);

    // 2.Imprime los bytes hexadecimales de la instruccion completa
    for (int i = 0; i < inst->size_in_bytes; i++){ // recore la instrucciion completa 
        printf("%02X ", vm->memory[physical_addr + i]);
    }

    // alinea la columna de la barra '|' (el tamaño maximo de instruccion son 6 bytes = 18 caracteres)
    int printed_chars = inst->size_in_bytes * 3;
    for (int i = printed_chars; i < 19; i++){
        putchar(' ');
    }

    // 3.Imprime la barra separadora y el mnemonico
    printf("| %s", get_mnemonic(inst->opcode));

    // 4.Imprime operandos en orden(A y luego B)
    if (inst->num_operands == 1){
        printf(" ");
        print_operand(inst->type_a, inst->reg_a, inst->val_a);
    }else if (inst->num_operands == 2){
        printf(" ");
        print_operand(inst->type_a, inst->reg_a, inst->val_a);
        printf(", ");
        print_operand(inst->type_b, inst->reg_b, inst->val_b);
    }

    printf("\n");
}

void disasm_all(const VM *vm){
    if (vm == NULL) {
        return;
    }

    uint16_t addr = 0;
    uint16_t fin_codigo = vm->segments[0].size;

    while (addr < fin_codigo){
        Instruction inst = decode_instruction(vm, addr);
        disasm_print_instruction(vm, addr, &inst);
        //[0000] B1 00 0A 00 05 9B ∣ ADD [DS+5], 10
        addr += inst.size_in_bytes; //addr += 6
    }
}


#include <stdio.h>
#include <string.h>
#include "disasm.h"
#include "decoder.h"

//tabla de nombres de registros (32 registros en total)
//facil de actualizar para la segunda parte
static const char *register_names[NUM_REGISTERS] = {
    [REG_IP]  = "IP",  [REG_OPC] = "OPC", [REG_OP1] = "OP1", [REG_OP2] = "OP2",
    [REG_LAR] = "LAR", [REG_MAR] = "MAR", [REG_MBR] = "MBR", [REG_SP] = "SP",
    [REG_EAX] = "EAX", [REG_EBX] = "EBX", [REG_ECX] = "ECX", [REG_BP] = "BP",
    [REG_EDX] = "EDX", [REG_EEX] = "EEX", [REG_EFX] = "EFX", [REG_ES] = "ES",
    [REG_AC]  = "AC",  [REG_CC]  = "CC",  [REG_SS] = "SS"  , [REG_KS] = "KS",
    [REG_CS]  = "CS",  [REG_DS]  = "DS",  [REG_PS] = "PS"
};

//tabla de nombres de mnemonicos (0x00 a 0x1F)
static const char *mnemonic_names[32] = {
    [OP_SYS]  = "SYS",  [OP_JMP]  = "JMP",  [OP_JP]   = "JP",   [OP_JN]   = "JN",
    [OP_JZ]   = "JZ",   [OP_JC]   = "JC",   [OP_JV]   = "JV",   [OP_JNP]  = "JNP",
    [OP_JNN]  = "JNN",  [OP_JNZ]  = "JNZ",  [OP_NOT]  = "NOT",  [OP_STOP] = "STOP",
    [OP_MOV]  = "MOV",  [OP_ADD]  = "ADD",  [OP_SUB]  = "SUB",  [OP_MUL]  = "MUL",
    [OP_DIV]  = "DIV",  [OP_CMP]  = "CMP",  [OP_AND]  = "AND",  [OP_OR]   = "OR",
    [OP_XOR]  = "XOR",  [OP_SWAP] = "SWAP", [OP_SHL]  = "SHL",  [OP_SHR]  = "SHR",
    [OP_SAR]  = "SAR",  [OP_LDL]  = "LDL",  [OP_LDH]  = "LDH",  [OP_RND]  = "RND",
    [OP_PUSH] = "PUSH", [OP_POP]  = "POP",  [OP_CALL] = "CALL", [OP_RET]  = "RET"

};

static const char *gpr_sector_names[6][4] = { //tabla de nombres de subregistros
    [0] = {"EAX", "AL", "AH", "AX"},
    [1] = {"EBX", "BL", "BH", "BX"},
    [2] = {"ECX", "CL", "CH", "CX"},
    [3] = {"EDX", "DL", "DH", "DX"},
    [4] = {"EEX", "EL", "EH", "EX"},
    [5] = {"EFX", "FL", "FH", "FX"}
};

static const char *get_register_name(uint8_t reg, uint8_t sector){
    if (reg >= REG_EAX && reg <= REG_EFX && sector < 4){
        return gpr_sector_names[reg - REG_EAX][sector];
    }
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
static void print_operand(OperandType type, uint8_t reg, int16_t val, uint8_t sector, uint8_t mem_size){ //recibimos el sector nuevo del decoder y el mem size para operandos de memoria
    if (type == OP_REG){
        printf("%s", get_register_name(reg, sector)); //[EAX]
    }else if (type == OP_IMM){
        printf("%d", val);
    }else if (type == OP_MEM){ 
        const char *pfx = "";
        if (mem_size == 1) pfx = "b";
        else if (mem_size == 2) pfx = "w";
        else if (mem_size == 4) pfx = "l";

        if (reg != 0){
            if (val >= 0){
                printf("%s[%s+%d]", pfx, get_register_name(reg, 0), val); //ej: b[DS+0]
            }else{
                printf("%s[%s%d]", pfx, get_register_name(reg, 0), val); //ej: b[DS-5]
            }
        }else{
            printf("%s[%d]", pfx, val); //[8]
        }
    }
}

void disasm_print_instruction(const VM *vm, uint16_t physical_addr, const Instruction *inst){
    if (vm == NULL || inst == NULL){
        return;
    }

    //marcamos el entry point '>' si coincide con el entry point 
    uint16_t entry_phys = 0xFFFF;
    if (vm->registers[REG_CS] != -1){
        uint16_t cs_idx = (uint16_t)((vm->registers[REG_CS] >> 16) & 0xFFFF);
        if (cs_idx < NUM_SEGMENTS && vm->segments[cs_idx].base != 0xFFFF){
            entry_phys = vm->segments[cs_idx].base + (uint16_t)(vm->registers[REG_IP] & 0xFFFF);
        }
    }

    if (physical_addr == entry_phys){
        printf(">[%04X] ", physical_addr);
    }else{
        printf(" [%04X] ", physical_addr);
    }

    //imprime los bytes hexadecimales de la instruccion completa
    for (int i = 0; i < inst->size_in_bytes; i++){ // recore la instrucciion completa 
        printf("%02X ", vm->memory[physical_addr + i]);
    }

    // alinea la columna de la barra '|' (el tamaño maximo de instruccion son 6 bytes = 18 caracteres)
    int printed_chars = inst->size_in_bytes * 3;
    for (int i = printed_chars; i < 19; i++){
        putchar(' ');
    }

    //imprime la barra separadora y el mnemonico
    printf("| %s", get_mnemonic(inst->opcode));

    //imprime operandos en orden(A y luego B)
    if (inst->num_operands == 1){
        printf(" ");
        print_operand(inst->type_a, inst->reg_a, inst->val_a, inst->sector_a, inst->mem_size_a);
    }else if (inst->num_operands == 2){
        printf(" ");
        print_operand(inst->type_a, inst->reg_a, inst->val_a, inst->sector_a, inst->mem_size_a);
        printf(", ");
        print_operand(inst->type_b, inst->reg_b, inst->val_b, inst->sector_b, inst->mem_size_b);
    }

    printf("\n");
}

void disasm_all(const VM *vm){
    if (vm == NULL) {
        return;
    }

    //muestra contenido del Const Segment(si existe)
    if (vm->registers[REG_KS] != -1){
        uint16_t ks_idx = (uint16_t)((vm->registers[REG_KS] >> 16) & 0xFFFF);
        if (ks_idx < NUM_SEGMENTS && vm->segments[ks_idx].base != 0xFFFF && vm->segments[ks_idx].size > 0){
            uint16_t ks_base = vm->segments[ks_idx].base;
            uint16_t ks_size = vm->segments[ks_idx].size;
            uint16_t offset = 0;

            while (offset < ks_size){
                uint16_t str_start = offset;
                while (offset < ks_size && vm->memory[ks_base + offset] != 0){
                    offset++;
                }
                if (offset < ks_size && vm->memory[ks_base + offset] == 0){
                    offset++; // incluir '\0'
                }
                uint16_t str_len = offset - str_start;

                printf(" [%04X] ", ks_base + str_start);

                int printed_chars = 0;
                if (str_len <= 7){
                    for (int i = 0; i < str_len; i++){
                        printf("%02X ", vm->memory[ks_base + str_start + i]);
                        printed_chars += 3;
                    }
                }else{
                    for (int i = 0; i < 6; i++){
                        printf("%02X ", vm->memory[ks_base + str_start + i]);
                        printed_chars += 3;
                    }
                    printf(".. ");
                    printed_chars += 3;
                }

                for (int i = printed_chars; i < 19; i++){
                    putchar(' ');
                }

                printf("| \"");
                for (int i = 0; i < str_len - 1; i++){
                    uint8_t c = vm->memory[ks_base + str_start + i];
                    if (c >= 32 && c <= 126){
                        putchar(c);
                    }else{
                        putchar('.');
                    }
                }
                printf("\"\n");
            }
        }
    }

    // 2. Mostrar instrucciones del Code Segment
    if (vm->registers[REG_CS] == -1) return;
    uint16_t cs_idx = (uint16_t)((vm->registers[REG_CS] >> 16) & 0xFFFF);
    uint16_t addr = vm->segments[cs_idx].base;
    uint16_t fin_codigo = addr + vm->segments[cs_idx].size;

    while (addr < fin_codigo){
        Instruction inst = decode_instruction(vm, addr);
        disasm_print_instruction(vm, addr, &inst);
        addr += inst.size_in_bytes;
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decoder.h"

Instruction decode_instruction(const VM *vm, uint16_t physical_addr){
    Instruction inst;

    memset(&inst, 0, sizeof(Instruction)); // inicializa el reg en 0

  
    if(vm == NULL || physical_addr >= RAM_SIZE){
        fprintf(stderr, "Fallo de segmento\n");
        exit(1);
    }

    //physical_addr < RAM_size, physical_addr + offset < RAM_size
    
    uint8_t primer_byte = vm->memory[physical_addr];
    int offset = 1; 

    //Leer y desarmar el primer byte

    if(primer_byte == 0x0F){
        inst.opcode = OP_STOP;
        inst.num_operands = 0;
        inst.type_a = OP_NONE;
        inst.type_b = OP_NONE;
    }
    else{
        if (((primer_byte >> 4) & 0x03) == 0){
            inst.num_operands = 1;
            inst.opcode = primer_byte & 0x1F;
            inst.type_a = (primer_byte >> 6) & 0x03;
            inst.type_b = OP_NONE;
        }
        else{
            inst.type_b = (primer_byte >> 6) & 0x03;
            inst.type_a = (primer_byte >> 4) & 0x03;
            inst.num_operands = 2;
            inst.opcode = 0x10 | (primer_byte & 0x0F);  
        }
    }
    // Extraer los operandos de la RAM

    if(inst.num_operands == 2){
        if(inst.type_b == OP_REG){
            uint8_t byte_reg = vm->memory[physical_addr+offset];
            inst.reg_b = byte_reg & 0x1F; //cod 0..31
            inst.val_b = 0;
            inst.raw_op_b = (inst.type_b << 24) | byte_reg;

            offset+=1;
        }
        else if(inst.type_b == OP_IMM){
            uint8_t b1 = vm->memory[physical_addr + offset]; 
            //Necesito b1yb2 porque los inmediatos son de 16bits
            uint8_t b2 = vm->memory[physical_addr + offset + 1];
            inst.reg_b = 0;
            inst.val_b = (int16_t)(((uint16_t)b1 << 8) | b2); // Valor con signo
            inst.raw_op_b = ((int32_t)inst.type_b << 24) | ((int32_t)b1 << 8) | b2;
            offset += 2;
        }
        else if(inst.type_b == OP_MEM){
            uint8_t b1 = vm->memory[physical_addr + offset];
            uint8_t b2 = vm->memory[physical_addr + offset + 1];
            uint8_t b3 = vm->memory[physical_addr + offset + 2];
            inst.val_b = (int16_t) (((uint16_t)b1 << 8) | b2);
            inst.reg_b = b3 & 0x1F;
            inst.raw_op_b = ((int32_t)inst.type_b << 24) | ((int32_t)b1 << 16) | ((int32_t)b2 << 8) | b3;

            offset += 3;
        }

    }
    if (inst.num_operands >= 1){
        if(inst.type_a == OP_REG){
            uint8_t byte_reg = vm->memory[physical_addr+offset];
            inst.reg_a = byte_reg & 0x1F; //cod 0..31
            inst.val_a = 0;
            inst.raw_op_a = (inst.type_a << 24) | byte_reg;

            offset+=1;
        }
        else if(inst.type_a == OP_IMM){
            uint8_t b1 = vm->memory[physical_addr + offset]; 
            //Necesito b1yb2 porque los inmediatos son de 16bits
            uint8_t b2 = vm->memory[physical_addr + offset + 1];
            inst.reg_a = 0;
            inst.val_a = (int16_t)(((uint16_t)b1 << 8) | b2); // Valor con signo
            inst.raw_op_a = ((int32_t)inst.type_a << 24) | ((int32_t)b1 << 8) | b2;
            offset += 2;
        }
        else if(inst.type_a == OP_MEM){
            uint8_t b1 = vm->memory[physical_addr + offset];
            uint8_t b2 = vm->memory[physical_addr + offset + 1];
            uint8_t b3 = vm->memory[physical_addr + offset + 2];
            inst.val_a = (int16_t) (((uint16_t)b1 << 8) | b2);
            inst.reg_a = b3 & 0x1F;
            inst.raw_op_a = ((int32_t)inst.type_a << 24) | ((int32_t)b1 << 16) | ((int32_t)b2 << 8) | b3;

            offset += 3;
        }
    }

    inst.size_in_bytes = offset;


    return inst;
}
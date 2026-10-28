#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decoder.h"

Instruction decode_instruction(const VM *vm, uint16_t physical_addr){
    Instruction inst;

    memset(&inst, 0, sizeof(Instruction)); // inicializa el reg en 0
    inst.mem_size_a = 4;
    inst.mem_size_b = 4;

  
    if(vm == NULL || physical_addr >= vm->ram_size){
        fprintf(stderr, "Fallo de segmento\n");
        exit(1);
    }

    //physical_addr < RAM_size, physical_addr + offset < RAM_size <- ahora la ram_size puede variar
    
    uint8_t primer_byte = vm->memory[physical_addr];
    uint32_t offset = 1; 

    //Leer y desarmar el primer byte

    if(primer_byte == 0x0F || primer_byte == 0x0E){ //ahora tenemos que preguntar por el ret
        inst.opcode = primer_byte;
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
            if (physical_addr + offset >= vm->ram_size) { fprintf(stderr, "Fallo de segmento\n"); exit(1); }
            uint8_t byte_reg = vm->memory[physical_addr+offset];
            inst.sector_b = (byte_reg >> 5) & 0x03; //sector del registro -> completo, el 4to byte, el 3ro o los 2 bytes menos significativos
            inst.reg_b = byte_reg & 0x1F; //cod 0..31
            inst.val_b = 0;
            inst.raw_op_b = (inst.type_b << 24) | byte_reg;

            offset+=1;
        }
        else if(inst.type_b == OP_IMM){
            if (physical_addr + offset + 1 >= vm->ram_size) { fprintf(stderr, "Fallo de segmento\n"); exit(1); }
            uint8_t b1 = vm->memory[physical_addr + offset]; 
            //Necesito b1yb2 porque los inmediatos son de 16bits
            uint8_t b2 = vm->memory[physical_addr + offset + 1];
            inst.reg_b = 0;
            inst.val_b = (int16_t)(((uint16_t)b1 << 8) | b2); // Valor con signo
            inst.raw_op_b = ((int32_t)inst.type_b << 24) | ((int32_t)b1 << 8) | b2; //inst.raw_op_b = ((int32_t)inst.type_b << 24) || (int32_t)inst.val_b
            offset += 2;
        }
        else if(inst.type_b == OP_MEM){
            if (physical_addr + offset + 2 >= vm->ram_size) { fprintf(stderr, "Fallo de segmento\n"); exit(1); }
            uint8_t b1 = vm->memory[physical_addr + offset];
            uint8_t b2 = vm->memory[physical_addr + offset + 1];
            uint8_t b3 = vm->memory[physical_addr + offset + 2];
            inst.val_b = (int16_t) (((uint16_t)b1 << 8) | b2);
            uint8_t mod = (b3 >> 5) & 0x07; //extraemos el modificador de taamaño de memoria -> b,w,l
            inst.mem_size_b = (mod == 1) ? 1 : ((mod == 2) ? 2 : 4);
            inst.reg_b = b3 & 0x1F;
            inst.raw_op_b = ((int32_t)inst.type_b << 24) | ((int32_t)b1 << 16) | ((int32_t)b2 << 8) | b3;

            offset += 3;
        }

    }
    if (inst.num_operands >= 1){
        if(inst.type_a == OP_REG){
            if (physical_addr + offset >= vm->ram_size) { fprintf(stderr, "Fallo de segmento\n"); exit(1); }
            uint8_t byte_reg = vm->memory[physical_addr+offset];
            inst.sector_a = (byte_reg >> 5) & 0x03;
            inst.reg_a = byte_reg & 0x1F; //cod 0..31
            inst.val_a = 0;
            inst.raw_op_a = (inst.type_a << 24) | byte_reg; // tipo de op 8 bits y valor en parte baja 24 bits

            offset+=1;
        }
        else if(inst.type_a == OP_IMM){
            if (physical_addr + offset + 1 >= vm->ram_size) { fprintf(stderr, "Fallo de segmento\n"); exit(1); }
            uint8_t b1 = vm->memory[physical_addr + offset]; 
            //Necesito b1yb2 porque los inmediatos son de 16bits
            uint8_t b2 = vm->memory[physical_addr + offset + 1];
            inst.reg_a = 0;
            inst.val_a = (int16_t)(((uint16_t)b1 << 8) | b2); // Valor con signo
            inst.raw_op_a = ((int32_t)inst.type_a << 24) | ((int32_t)b1 << 8) | b2;
            offset += 2;
        }
        else if(inst.type_a == OP_MEM){
            if (physical_addr + offset + 2 >= vm->ram_size) { fprintf(stderr, "Fallo de segmento\n"); exit(1); }
            uint8_t b1 = vm->memory[physical_addr + offset];
            uint8_t b2 = vm->memory[physical_addr + offset + 1];
            uint8_t b3 = vm->memory[physical_addr + offset + 2];
            inst.val_a = (int16_t) (((uint16_t)b1 << 8) | b2);
            uint8_t mod = (b3 >> 5) & 0x07;
            inst.mem_size_a = (mod == 1) ? 1 : ((mod == 2) ? 2 : 4);
            inst.reg_a = b3 & 0x1F;
            inst.raw_op_a = ((int32_t)inst.type_a << 24) | ((int32_t)b1 << 16) | ((int32_t)b2 << 8) | b3;

            offset += 3;
        }
    }

    inst.size_in_bytes = offset;


    return inst;
}
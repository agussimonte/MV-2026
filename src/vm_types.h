#ifndef VM_TYPES_H
#define VM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Dimensiones de hardware simulado
#define RAM_SIZE 16384       // 16 KiB exactos
#define NUM_REGISTERS 32      // 32 registros de 4 bytes
#define NUM_SEGMENTS 8        // 8 entradas de descriptores de segmento[cite: 1]

// 1. Identificadores de Registros (página 3 del documento)[cite: 1]
typedef enum {
    REG_IP  = 0,  REG_OPC = 1,  REG_OP1 = 2,  REG_OP2 = 3,
    REG_LAR = 4,  REG_MAR = 5,  REG_MBR = 6,
    REG_EAX = 10, REG_EBX = 11, REG_ECX = 12,
    REG_EDX = 13, REG_EEX = 14, REG_EFX = 15,
    REG_AC  = 16, REG_CC  = 17,
    REG_CS  = 26, REG_DS  = 27
} RegisterIndex;


// 1.5 Codigos de operacion
typedef enum{
    // un operando  
    OP_SYS = 0x00, OP_JMP  = 0x01, 
    OP_JP = 0x02, OP_JN = 0x03,
    OP_JZ = 0x04, OP_JC = 0x05, 
    OP_JV = 0x06, OP_JNP = 0x07,
    OP_JNN = 0x08, OP_JNZ = 0x09, 
    OP_NOT = 0x0A,

    // sin operandos  
    OP_STOP = 0x0F,

    // dos operandos  
    OP_MOV = 0x10, OP_ADD = 0x11, 
    OP_SUB = 0x12, OP_MUL = 0x13,
    OP_DIV = 0x14, OP_CMP = 0x15, 
    OP_AND = 0x16, OP_OR = 0x17,
    OP_XOR = 0x18, OP_SWAP = 0x19, 
    OP_SHL = 0x1A, OP_SHR = 0x1B,
    OP_SAR = 0x1C, OP_LDL = 0x1D, 
    OP_LDH = 0x1E, OP_RND = 0x1F
}Opcode;

// 2. Tipos de Operandos (página 6 del documento)[cite: 1]
typedef enum {
    OP_NONE = 0x00, // 0 bytes[cite: 1]
    OP_REG  = 0x01, // 1 byte[cite: 1]
    OP_IMM  = 0x02, // 2 bytes[cite: 1]
    OP_MEM  = 0x03  // 3 bytes[cite: 1]
} OperandType;

// 3. Entrada de la Tabla de Descriptores de Segmentos (página 2 del documento)[cite: 1]
// 32 bits totales: 2 bytes de base y 2 bytes de tamaño[cite: 1]
typedef struct {
    uint16_t base;
    uint16_t size;
} SegmentDescriptor;

// 4. Estructura de una Instrucción Decodificada
// Este es el puente directo: Persona 1 la llena y Persona 2 la ejecuta
typedef struct {
    uint8_t opcode;         // Código numérico de operación (0x00 a 0x1F)[cite: 1]
    uint8_t num_operands;   // 0, 1 o 2[cite: 1]
    OperandType type_a;     // Tipo del operando A[cite: 1]
    OperandType type_b;     // Tipo del operando B[cite: 1]
    int32_t raw_op_a;       // Valor crudo para cargar en OP1 (byte alto tipo, 3 bytes valor)[cite: 1]
    int32_t raw_op_b;       // Valor crudo para cargar en OP2[cite: 1]
    int16_t val_a;          // Inmediato o desplazamiento con signo[cite: 1]
    int16_t val_b;          // Inmediato o desplazamiento con signo[cite: 1]
    uint8_t reg_a;          // Índice del registro usado por el operando A[cite: 1]
    uint8_t reg_b;          // Índice del registro usado por el operando B[cite: 1]
    uint8_t size_in_bytes;  // Cantidad de bytes leídos de la memoria para avanzar IP[cite: 1]
} Instruction;

// 5. Estado global de la Máquina Virtual
typedef struct {
    uint8_t memory[RAM_SIZE];                  // Memoria principal[cite: 1]
    int32_t registers[NUM_REGISTERS];          // Registros con signo de 32 bits[cite: 1]
    SegmentDescriptor segments[NUM_SEGMENTS];  // Tabla de descriptores de segmentos[cite: 1]
    bool running;                              // Control del ciclo del procesador
    bool disasm_mode;                          // Flag -d activado[cite: 1]
} VM;

#endif
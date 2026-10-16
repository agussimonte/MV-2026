#ifndef VM_TYPES_H
#define VM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Dimensiones de hardware simulado
#define DEFAULT_RAM_SIZE 16384       // 16 KiB por defecto
#define RAM_SIZE DEFAULT_RAM_SIZE
#define NUM_REGISTERS 32      // 32 registros de 4 bytes
#define NUM_SEGMENTS 8        // 8 entradas de descriptores de segmento

// 1. Identificadores de Registros
typedef enum {
    REG_IP  = 0,  REG_OPC = 1,  REG_OP1 = 2,  REG_OP2 = 3,
    REG_LAR = 4,  REG_MAR = 5,  REG_MBR = 6,
    REG_SP  = 7,  REG_BP  = 8,
    REG_EAX = 10, REG_EBX = 11, REG_ECX = 12,
    REG_EDX = 13, REG_EEX = 14, REG_EFX = 15,
    REG_AC  = 16, REG_CC  = 17,
    REG_CS  = 26, REG_DS  = 27,
    REG_ES  = 28, REG_SS  = 29,
    REG_KS  = 30, REG_PS  = 31
} RegisterIndex;


// 1.5 Codigos de operacion
typedef enum {
    // un operando  
    OP_SYS  = 0x00, OP_JMP  = 0x01, 
    OP_JP   = 0x02, OP_JN   = 0x03,
    OP_JZ   = 0x04, OP_JC   = 0x05, 
    OP_JV   = 0x06, OP_JNP  = 0x07,
    OP_JNN  = 0x08, OP_JNZ  = 0x09, 
    OP_NOT  = 0x0A,
    OP_PUSH = 0x0B, OP_POP  = 0x0C,
    OP_CALL = 0x0D,

    // sin operandos  
    OP_RET  = 0x0E,
    OP_STOP = 0x0F,

    // dos operandos  
    OP_MOV  = 0x10, OP_ADD  = 0x11, 
    OP_SUB  = 0x12, OP_MUL  = 0x13,
    OP_DIV  = 0x14, OP_CMP  = 0x15, 
    OP_AND  = 0x16, OP_OR   = 0x17,
    OP_XOR  = 0x18, OP_SWAP = 0x19, 
    OP_SHL  = 0x1A, OP_SHR  = 0x1B,
    OP_SAR  = 0x1C, OP_LDL  = 0x1D, 
    OP_LDH  = 0x1E, OP_RND  = 0x1F
} Opcode;

// 2. Tipos de Operandos
typedef enum {
    OP_NONE = 0x00, // 0 bytes
    OP_REG  = 0x01, // 1 byte
    OP_IMM  = 0x02, // 2 bytes
    OP_MEM  = 0x03  // 3 bytes
} OperandType;

// Sectores de registro (bits previos al codigo de registro)
typedef enum {
    REG_SECTOR_FULL = 0, // 00: 4 bytes completos (EAX..EFX)
    REG_SECTOR_L    = 1, // 01: 4to byte / LSB (AL..FL)
    REG_SECTOR_H    = 2, // 10: 3er byte / byte alto de AX (AH..FH)
    REG_SECTOR_X    = 3  // 11: 2 bytes menos significativos (AX..FX)
} RegSector;

// Modificador de tamano para operandos de memoria
typedef enum {
    MEM_SIZE_DEFAULT = 4, // Sin modificador: 4 bytes (l)
    MEM_SIZE_BYTE    = 1, // 001: 1 byte (b)
    MEM_SIZE_WORD    = 2, // 010: 2 bytes (w)
    MEM_SIZE_LONG    = 4  // 100: 4 bytes (l)
} MemAccessSize;

// 3. Entrada de la Tabla de Descriptores de Segmentos
// 32 bits totales: 2 bytes de base y 2 bytes de tamaño
typedef struct {
    uint16_t base;
    uint16_t size;
} SegmentDescriptor;

// 4. Estructura de una Instruccion Decodificada
typedef struct {
    uint8_t opcode;         // Codigo numerico de operacion (0x00 a 0x1F)
    uint8_t num_operands;   // 0, 1 o 2
    OperandType type_a;     // Tipo del operando A
    OperandType type_b;     // Tipo del operando B
    int32_t raw_op_a;       // Valor crudo para cargar en OP1 (byte alto tipo, 3 bytes valor)
    int32_t raw_op_b;       // Valor crudo para cargar en OP2
    int16_t val_a;          // Inmediato o desplazamiento con signo
    int16_t val_b;          // Inmediato o desplazamiento con signo
    uint8_t reg_a;          // Indice del registro usado por el operando A
    uint8_t reg_b;          // Indice del registro usado por el operando B
    uint8_t sector_a;       // Sector del registro A (RegSector: FULL, L, H, X)
    uint8_t sector_b;       // Sector del registro B (RegSector: FULL, L, H, X)
    uint8_t mem_size_a;     // Tamano de acceso a memoria A en bytes (1, 2 o 4)
    uint8_t mem_size_b;     // Tamano de acceso a memoria B en bytes (1, 2 o 4)
    uint8_t size_in_bytes;  // Cantidad de bytes leidos de la memoria para avanzar IP
} Instruction;

// 5. Estado global de la Maquina Virtual
typedef struct {
    uint8_t *memory;                           // Memoria principal alocada dinamicamente
    uint32_t ram_size;                         // Tamano de RAM en bytes
    int32_t registers[NUM_REGISTERS];          // Registros con signo de 32 bits
    SegmentDescriptor segments[NUM_SEGMENTS];  // Tabla de descriptores de segmentos
    bool running;                              // Control del ciclo del procesador
    bool disasm_mode;                          // Flag -d activado
    const char *vmi_path;                      // Ruta del archivo .vmi (o NULL si no se especifico)
} VM;

#endif

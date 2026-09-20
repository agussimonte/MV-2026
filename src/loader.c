#include <stdio.h>
#include <string.h>
#include "loader.h"
#include "mmu.h"

int load_vmx(VM *vm, const char *filepath) {
    if (vm == NULL || filepath == NULL) {
        fprintf(stderr, "Error: Argumentos invalidos pasados al cargador.\n");
        return 0;
    }

    FILE *file;
    if ((file = fopen(filepath, "rb")) == NULL) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'.\n", filepath);
        return 0;
    }
    else{
        
        // Bytes 0-4: "VMX26"
        // Byte 5: Version (1)
        // Bytes 6-7: Tamano del codigo (Big-Endian)
        
        uint8_t header[8]; // Cabecera: 8 bytes
        size_t bytes_read;

        bytes_read = fread(header, 1, sizeof(header), file); //Cantidad de elementos leidos
        if (bytes_read < sizeof(header)) {
            fprintf(stderr, "Error: Archivo '%s' corrupto o incompleto (cabecera trunca).\n", filepath);
            fclose(file);
            return 0;
        }

        if (header[0] != 'V' || header[1] != 'M' || header[2] != 'X' || header[3] != '2' || header[4] != '6') {                           
            fprintf(stderr, "Error: Identificador invalido (esperado 'VMX26').\n");
            fclose(file);
            return 0;
        }

        if (header[5] != 1) {
            fprintf(stderr, "Error: Version de binario no soportada (%u).\n", header[5]);
            fclose(file);
            return 0;
        }

        uint16_t code_size = ((uint16_t)header[6] << 8) | (uint16_t)header[7];
        //header[6] tiene la parte alta y header[7] la parte baja, desplazamos 6 hacia la izquierda
        //hacemos un or con 7 para tener el tamaño del codigo
        if (code_size > RAM_SIZE) {
            fprintf(stderr, "Error: El tamano del codigo (%u bytes) excede la memoria principal (%d bytes).\n",
                    code_size, RAM_SIZE);
            fclose(file);
            return 0;
        }

        // inicializa memoria principal y tabla de segmentos mediante la MMU
        mmu_init(vm);

        // inicializa registros en cero
        memset(vm->registers, 0, sizeof(vm->registers)); //memset <string.h>

        // Cargar codigo en memoria principal a partir de la direccion fisica 0
        if (code_size > 0) {
            bytes_read = fread(vm->memory, 1, code_size, file); //fread guarda el codigo en vm->memory
            // bytes_read = cantidad de bytes del codigo
            if (bytes_read != code_size) {
                fprintf(stderr, "Error: No se pudo leer el bloque completo de codigo (%zu de %u bytes leidos).\n", bytes_read, code_size);
                fclose(file);
                return 0;
            }
        }

        fclose(file);


        //Registros definidos en vm_types.h


        // Configurar tabla de descriptores de segmentos (8 entradas)
        // Entrada 0: Segmento de Codigo (CS)
        vm->segments[0].base = 0;
        vm->segments[0].size = code_size;

        // Entrada 1: Segmento de Datos (DS)
        vm->segments[1].base = code_size;
        vm->segments[1].size = (uint16_t)(RAM_SIZE - code_size); //Evitamos warnings con el casteo, ya que size es de 16 bits

 

        // Inicializar registros base
        vm->registers[REG_CS] = 0x00000000; 
        vm->registers[REG_DS] = 0x00010000; // Primeros 4 bits (pos. Segmentos) y ultimos 4 (offset)
        vm->registers[REG_IP] = vm->registers[REG_CS]; //Primera instruccion en CS

        vm->running = true;

        return 1;
    }

    
}


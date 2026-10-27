#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "loader.h"
#include "mmu.h"

//extrae el nombre del archivo de una ruta (ej: "prog.vmx" de "C:/test/prog.vmx")
static const char *get_basename(const char *path){
    if (!path) return "";
    const char *p1 = strrchr(path, '/'); //para agus
    const char *p2 = strrchr(path, '\\'); //win
    const char *p = (p1 > p2) ? p1 : p2;
    return p ? (p + 1) : path;
}

//escribe un entero de 32 bits en memoria en formato big endian
static void write_be32(uint8_t *dest, uint32_t val){
    dest[0] = (uint8_t)((val >> 24) & 0xFF);
    dest[1] = (uint8_t)((val >> 16) & 0xFF);
    dest[2] = (uint8_t)((val >> 8) & 0xFF);
    dest[3] = (uint8_t)(val & 0xFF);
}

int load_vmx(VM *vm, const char *filepath, bool has_p, int param_count, char *param_values[]){
    if (vm == NULL || filepath == NULL){
        fprintf(stderr, "Error: Argumentos invalidos pasados al cargador.\n");
        return 0;
    }

    FILE *file = fopen(filepath, "rb");
    if (file == NULL){
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'.\n", filepath);
        return 0;
    }

    //leemos los primeros 8 bytes de cabecera comunes a version 1 y version 2
    uint8_t header[18];
    size_t bytes_read = fread(header, 1, 8, file);
    if (bytes_read < 8){
        fprintf(stderr, "Error: Archivo '%s' corrupto o incompleto (cabecera trunca).\n", filepath);
        fclose(file);
        return 0;
    }

    //valida "VMX26"
    if (header[0] != 'V' || header[1] != 'M' || header[2] != 'X' || header[3] != '2' || header[4] != '6'){
        fprintf(stderr, "Error: Identificador invalido (esperado 'VMX26').\n");
        fclose(file);
        return 0;
    }

    uint8_t version = header[5];
    if (version != 1 && version != 2){
        fprintf(stderr, "Error: Version de binario no soportada (%u).\n", version);
        fclose(file);
        return 0;
    }

    //inicializo las variables para almacenar los tamaños de los segmentos y el punto de entrada
    uint16_t code_size = 0;
    uint16_t data_size = 0;
    uint16_t extra_size = 0;
    uint16_t stack_size = 0;
    uint16_t const_size = 0;
    uint16_t entry_point = 0;

    if (version == 1){ //solo viene el tamaño del codigo, el data segmente ocupa todo lo demas
        code_size = ((uint16_t)header[6] << 8) | (uint16_t)header[7];
        if (code_size > vm->ram_size){
            fprintf(stderr, "Memoria insuficiente\n");
            fclose(file);
            return 0;
        }
        data_size = (uint16_t)(vm->ram_size - code_size);
    } else{
        //version 2:leemos los 10 bytes restantes de la cabecera(total 18 bytes)
        size_t rem = fread(&header[8], 1, 10, file);
        if (rem < 10){
            fprintf(stderr, "Error: Cabecera version 2 incompleta.\n");
            fclose(file);
            return 0;
        }
        code_size   = ((uint16_t)header[6]  << 8) | (uint16_t)header[7];
        data_size   = ((uint16_t)header[8]  << 8) | (uint16_t)header[9];
        extra_size  = ((uint16_t)header[10] << 8) | (uint16_t)header[11];
        stack_size  = ((uint16_t)header[12] << 8) | (uint16_t)header[13];
        const_size  = ((uint16_t)header[14] << 8) | (uint16_t)header[15];
        entry_point = ((uint16_t)header[16] << 8) | (uint16_t)header[17];
    }

    //calculamos tamaño del Param Segment (si se incluyo el flag -p)
    uint16_t param_size = 0;
    uint16_t strings_bytes = 0;
    int argc_val = 0;
    const char *prog_name = "";

    if (has_p && filepath != NULL){
        prog_name = get_basename(filepath);
        strings_bytes = (uint16_t)(strlen(prog_name) + 1);
        for (int i = 0; i < param_count; i++) {
            strings_bytes += (uint16_t)(strlen(param_values[i]) + 1);
        }
        argc_val = 1 + param_count; //total de argumentos
        param_size = strings_bytes + (uint16_t)(argc_val * 4); //total de bytes + tamaño del arreglo de punteros
    }

    //validacion de memoria requerida contra RAM disponible
    uint32_t total_required = (uint32_t)param_size + const_size + code_size + data_size + extra_size + stack_size;
    if (total_required > vm->ram_size){
        fprintf(stderr, "Memoria insuficiente\n");
        fclose(file);
        return 0;
    }

    //inicializamos memoria RAM y tabla de descriptores
    mmu_init(vm);

    //inicio registros en 0
    for (int r = 0; r < NUM_REGISTERS; r++){
        vm->registers[r] = 0;
    }
    //registros base por defecto en -1 
    vm->registers[REG_CS] = -1;
    vm->registers[REG_DS] = -1;
    vm->registers[REG_ES] = -1;
    vm->registers[REG_SS] = -1;
    vm->registers[REG_KS] = -1;
    vm->registers[REG_PS] = -1;




    //disposicion fisica contigua: Param -> Const -> Code -> Data -> Extra -> Stack
    uint16_t current_phys = 0;

    uint16_t param_phys = 0;
    if (param_size > 0){
        param_phys = current_phys;
        current_phys += param_size;
    }
    uint16_t const_phys = 0;
    if (const_size > 0){
        const_phys = current_phys;
        current_phys += const_size;
    }
    uint16_t code_phys = 0;
    if (code_size > 0){
        code_phys = current_phys;
        current_phys += code_size;
    }
    uint16_t data_phys = 0;
    if (data_size > 0){
        data_phys = current_phys;
        current_phys += data_size;
    }
    uint16_t extra_phys = 0;
    if (extra_size > 0){
        extra_phys = current_phys;
        current_phys += extra_size;
    }
    uint16_t stack_phys = 0;
    if (stack_size > 0){
        stack_phys = current_phys;
        current_phys += stack_size;
    }




    //construccion dinamica de la tabla de descriptores(solo segmentos con tamaño > 0)
    int seg_idx = 0;

    int param_seg_idx = -1;
    if (param_size > 0){
        vm->segments[seg_idx].base = param_phys;
        vm->segments[seg_idx].size = param_size;
        param_seg_idx = seg_idx++;
    }

    int const_seg_idx = -1;
    if (const_size > 0){
        vm->segments[seg_idx].base = const_phys;
        vm->segments[seg_idx].size = const_size;
        const_seg_idx = seg_idx++;
    }

    int code_seg_idx = -1;
    if (code_size > 0){
        vm->segments[seg_idx].base = code_phys;
        vm->segments[seg_idx].size = code_size;
        code_seg_idx = seg_idx++;
    }

    int data_seg_idx = -1;
    if (data_size > 0){
        vm->segments[seg_idx].base = data_phys;
        vm->segments[seg_idx].size = data_size;
        data_seg_idx = seg_idx++;
    }

    int extra_seg_idx = -1;
    if (extra_size > 0){
        vm->segments[seg_idx].base = extra_phys;
        vm->segments[seg_idx].size = extra_size;
        extra_seg_idx = seg_idx++;
    }

    int stack_seg_idx = -1;
    if (stack_size > 0){
        vm->segments[seg_idx].base = stack_phys;
        vm->segments[seg_idx].size = stack_size;
        stack_seg_idx = seg_idx++;
    }


    //carga bloques de codigo y constantes desde el archivo
    if (code_size > 0){
        bytes_read = fread(vm->memory + code_phys, 1, code_size, file);
        if (bytes_read != code_size){
            fprintf(stderr, "Error: No se pudo leer el bloque completo de codigo (%zu de %u bytes leidos).\n", bytes_read, code_size);
            fclose(file);
            return 0;
        }
    }
    if (const_size > 0){
        bytes_read = fread(vm->memory + const_phys, 1, const_size, file);
        if (bytes_read != const_size){
            fprintf(stderr, "Error: No se pudo leer el bloque completo de constantes (%zu de %u bytes leidos).\n", bytes_read, const_size);
            fclose(file);
            return 0;
        }
    }
    fclose(file);






    //construimos el contenido del Param Segment en memoria
    if (param_size > 0){
        uint8_t *p_mem = vm->memory + param_phys;
        uint16_t curr_str_offset = 0;

        uint16_t *offsets = (uint16_t *)malloc(sizeof(uint16_t) * argc_val);
        if (offsets == NULL){
            fprintf(stderr, "Error: No se pudo alocar memoria temporal para parametros.\n");
            return 0;
        }
        // 1 - guardar nombre del archivo .vmx
        offsets[0] = curr_str_offset;
        size_t len = strlen(prog_name) + 1;
        memcpy(p_mem + curr_str_offset, prog_name, len);
        curr_str_offset += (uint16_t)len;

        // 2 - guardar strings de parametros
        for (int i = 0; i < param_count; i++){
            offsets[1 + i] = curr_str_offset;
            len = strlen(param_values[i]) + 1;
            memcpy(p_mem + curr_str_offset, param_values[i], len);
            curr_str_offset += (uint16_t)len;
        }

        // 3 - arreglo argv de punteros de 4 bytes
        uint16_t argv_offset = strings_bytes;
        for (int i = 0; i < argc_val; i++){
            uint32_t ptr_val = ((uint32_t)param_seg_idx << 16) | (uint32_t)offsets[i];
            write_be32(p_mem + argv_offset + (i * 4), ptr_val); //los grabamos en big endian
        }
        free(offsets); 
    }

    //inicializamos registros base con punteros logicos (indice_tabla << 16)
    if (param_seg_idx >= 0) vm->registers[REG_PS] = ((int32_t)param_seg_idx << 16);
    if (const_seg_idx >= 0) vm->registers[REG_KS] = ((int32_t)const_seg_idx << 16);
    if (code_seg_idx >= 0)  vm->registers[REG_CS] = ((int32_t)code_seg_idx  << 16);
    if (data_seg_idx >= 0)  vm->registers[REG_DS] = ((int32_t)data_seg_idx  << 16);
    if (extra_seg_idx >= 0) vm->registers[REG_ES] = ((int32_t)extra_seg_idx << 16);
    if (stack_seg_idx >= 0) vm->registers[REG_SS] = ((int32_t)stack_seg_idx << 16);

    //inicializamos IP
    if (code_seg_idx >= 0){
        vm->registers[REG_IP] = vm->registers[REG_CS] | (int32_t)entry_point;
    } else{
        vm->registers[REG_IP] = -1;
    }



    //inicializamos la pila y el stack frame inicial
    if (stack_seg_idx >= 0){
        if (stack_size >= 12){
            uint8_t *s_mem = vm->memory + stack_phys;
            uint16_t sp_offset = stack_size - 12;

            //retorno de la subrutina principal: -1 (0xFFFFFFFF)
            write_be32(s_mem + sp_offset, 0xFFFFFFFF);

            //cantidad de argumentos (argc)
            uint32_t argc_stack = has_p ? (uint32_t)argc_val : 0;
            write_be32(s_mem + sp_offset + 4, argc_stack);

            //puntero al arreglo argv (o -1 si no hay parametros)
            uint32_t argv_stack = 0xFFFFFFFF;
            if (has_p && param_seg_idx >= 0){
                argv_stack = ((uint32_t)param_seg_idx << 16) | (uint32_t)strings_bytes;
            }
            write_be32(s_mem + sp_offset + 8, argv_stack);

            //sp apunta al retorno en el tope de la pila
            vm->registers[REG_SP] = ((int32_t)stack_seg_idx << 16) | (int32_t)sp_offset;
        } else{
            //tamaño insuficiente 
            vm->registers[REG_SP] = ((int32_t)stack_seg_idx << 16) | (int32_t)stack_size;
        }
    }

    vm->running = true;
    return 1;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 
#include "vm_types.h"
#include "mmu.h"
#include "loader.h"
#include "cpu.h"
#include "disasm.h"

//comprueba sufijo ignorando mayusculas/minusculas, detecta .vmx o .vmi
static bool ends_with(const char *str, const char *suffix){ //*suffix recibe ".vmx" o ".vmi"
    if (!str || !suffix) return false;
    size_t str_len = strlen(str);
    size_t suf_len = strlen(suffix);
    if (str_len < suf_len) return false;
    const char *p = str + (str_len - suf_len); //el puntero apunta directo a los ultimos 4 caracteres
    while (*p && *suffix){ //compara caracter a caracter con el sufijo
        char c1 = (*p >= 'A' && *p <= 'Z') ? (*p + 32) : *p;
        char c2 = (*suffix >= 'A' && *suffix <= 'Z') ? (*suffix + 32) : *suffix;
        if (c1 != c2) return false;
        p++;
        suffix++;
    }
    return true;
}

int main(int argc, char *argv[]){
    const char *vmx_path = NULL;
    const char *vmi_path = NULL;
    bool disasm_flag = false;
    uint32_t ram_size_kib = 16; //16 KiB por defecto
    int param_count = 0;
    char **param_values = NULL;

    //parseo de argumentos
    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "-d") == 0){
            disasm_flag = true;
        } else if (strncmp(argv[i], "m=", 2) == 0 || strncmp(argv[i], "M=", 2) == 0){
            int m_val = atoi(argv[i] + 2);
            if (m_val <= 0) {
                fprintf(stderr, "Error: Tamanio de memoria invalido: %s\n", argv[i]);
                return 1;
            }
            ram_size_kib = (uint32_t)m_val; //actualizo el nuevo tamaño de ram
        } else if (strcmp(argv[i], "-p") == 0){
            //todos los argumentos que le siguen son parametros del programa
            param_count = argc - (i + 1);
            if (param_count > 0) {
                param_values = &argv[i + 1];
            }
            break; // metemos el break porque -p siempre va al final, sandra no veas esto
        } else if (ends_with(argv[i], ".vmx")){
            vmx_path = argv[i];
        } else if (ends_with(argv[i], ".vmi")){
            vmi_path = argv[i];
        } else{
            fprintf(stderr, "Parametro no reconocido: %s\n", argv[i]);
            return 1;
        }
    }

    //validacion obligatoria, debe existir al menos uno de los dos
    if (vmx_path == NULL && vmi_path == NULL){
        fprintf(stderr, "Uso: %s [filename.vmx] [filename.vmi] [m=M] [-d] [-p param1 param2 ...]\n", argv[0]); //muestra el uso correcto
        return 1;
    }

    //si no hay .vmx, se ignoran los parametros -p
    if (vmx_path == NULL) {
        param_count = 0;
        param_values = NULL;
    }

    VM vm;
    memset(&vm, 0, sizeof(VM)); //limpiamos toda la estructura vm
    vm.ram_size = ram_size_kib * 1024;
    vm.memory = (uint8_t *)malloc(vm.ram_size); //pedimos un bloque de memoria del tamaño solicitado
    if (vm.memory == NULL) {
        fprintf(stderr, "Error: No se pudo alocar memoria RAM (%u bytes).\n", vm.ram_size);
        return 1;
    }
    memset(vm.memory, 0, vm.ram_size);//inicializamos los bytes de la memoria 
    vm.disasm_mode = disasm_flag;
    vm.vmi_path = vmi_path;


    int exit_code = 0;

    if (vmx_path != NULL){
        //carga programa en la memoria
        if (!load_vmx(&vm, vmx_path)){
            free(vm.memory);
            return 1;
        }

        //modo desensamblado (-d) o ejecucion normal
        if (disasm_flag){
            disasm_all(&vm);
        }else{
            cpu_init();
            uint32_t argv_ptr = 0xFFFFFFFF; // Por defecto si no hay params
            (void)param_values;              // Silenciar warning hasta que se arme el Param Segment
            cpu_init_stack(&vm, param_count, argv_ptr);
            cpu_run(&vm);
        }
    }else{
        //(parte de agus?  / vmi.c)
        fprintf(stderr, "modulo VMI.\n");
        exit_code = 1;
    }

    free(vm.memory);
    return exit_code;
}

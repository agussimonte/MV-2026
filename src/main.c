#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm_types.h"
#include "loader.h"
#include "cpu.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <filename.vmx> [-d]\n", argv[0]);
        return 1;
    }

    const char *filepath = NULL;
    bool disasm_flag = false;

    // Parseo de argumentos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            disasm_flag = true;
        } else if (filepath == NULL) {
            filepath = argv[i];
        }
    }

    if (filepath == NULL) {
        fprintf(stderr, "Uso: %s <filename.vmx> [-d]\n", argv[0]);
        return 1;
    }

    VM vm;
    memset(&vm, 0, sizeof(VM));
    vm.disasm_mode = disasm_flag;

    // Cargar programa en la memoria
    if (!load_vmx(&vm, filepath)) {
        return 1;
    }

    // Modo desensamblado (-d) o ejecucion normal
    if (disasm_flag) {
        printf("Modo desensamblador pendiente de implementacion.\n");
    } else {
        cpu_init();
        cpu_run(&vm);
    }

    return 0;
}


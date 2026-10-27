#ifndef LOADER_H
#define LOADER_H

#include <stdbool.h>
#include "vm_types.h"

// Lee el archivo binario .vmx (versiones 1 y 2), valida el encabezado ("VMX26"),
// carga los segmentos contiguos en RAM, construye la tabla de descriptores
// y configura los registros base (CS, DS, ES, SS, KS, PS), IP y el stack frame inicial.
// Retorna 1 si la carga fue exitosa, o 0 si hubo error.
int load_vmx(VM *vm, const char *filepath, bool has_p, int param_count, char *param_values[]);

#endif // LOADER_H

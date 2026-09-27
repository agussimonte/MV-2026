#ifndef LOADER_H
#define LOADER_H

#include "vm_types.h"
 

// Lee el archivo binario .vmx, valida el encabezado ("VMX26", version 1),
// carga el codigo en RAM, inicializa la tabla de segmentos y los registros CS, DS e IP
// Retorna 1 si la carga fue exitosa, o 0 si hubo error.
int load_vmx(VM *vm, const char *filepath);

#endif // LOADER_H

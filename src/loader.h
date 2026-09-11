#ifndef LOADER_H
#define LOADER_H

#include "vm_types.h"

// Lee el archivo binario .vmx, valida el encabezado ("VMX26", versión 1),
// carga el código en RAM, inicializa la tabla de segmentos y los registros CS, DS e IP[cite: 1]
// Retorna 0 si la carga fue exitosa, o un valor negativo si hubo error.
int load_vmx(VM *vm, const char *filepath);

#endif // LOADER_H
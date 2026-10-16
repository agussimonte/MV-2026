#ifndef VMI_H
#define VMI_H

#include "vm_types.h"

// Guarda el estado actual de la maquina virtual en un archivo .vmi
// Header "VMI26" (8 bytes) + Registros (128 bytes) + Tabla descriptores (32 bytes) + RAM completa
// Retorna 1 si fue exitoso, 0 si hubo error
int vmi_save(const VM *vm, const char *filepath);

// Carga el estado de la maquina virtual desde un archivo .vmi para continuar la ejecucion
// Retorna 1 si fue exitoso, 0 si hubo error
int vmi_load(VM *vm, const char *filepath);

// Maneja la ejecucion interactiva de un breakpoint (SYS 0x0F)
// Guarda la imagen y procesa los comandos ('g', 'q', Enter)
void vmi_handle_breakpoint(VM *vm);

#endif // VMI_H


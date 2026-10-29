#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vmi.h"

int vmi_save(const VM *vm, const char *filepath) {
    if (!vm || !filepath) {
        return 0;
    }

    FILE *f = fopen(filepath, "wb");
    if (!f) {
        fprintf(stderr, "Error: No se pudo abrir el archivo de imagen '%s' para escritura\n", filepath);
        return 0;
    }

    // 1. Header (8 bytes): "VMI26" + version(1) + ram_size_kib(2 bytes, Big-Endian)
    uint8_t header[8];
    header[0] = 'V';
    header[1] = 'M';
    header[2] = 'I';
    header[3] = '2';
    header[4] = '6';
    header[5] = 1;
    uint16_t ram_kib = (uint16_t)(vm->ram_size / 1024);
    header[6] = (uint8_t)((ram_kib >> 8) & 0xFF);
    header[7] = (uint8_t)(ram_kib & 0xFF);

    if (fwrite(header, 1, 8, f) != 8) {
        fclose(f);
        return 0;
    }

    // 2. Registros (32 x 4 = 128 bytes, Big-Endian)
    uint8_t reg_buf[128];
    for (int i = 0; i < NUM_REGISTERS; i++) {
        uint32_t val = (uint32_t)vm->registers[i];
        reg_buf[i * 4 + 0] = (uint8_t)((val >> 24) & 0xFF);
        reg_buf[i * 4 + 1] = (uint8_t)((val >> 16) & 0xFF);
        reg_buf[i * 4 + 2] = (uint8_t)((val >> 8) & 0xFF);
        reg_buf[i * 4 + 3] = (uint8_t)(val & 0xFF);
    }
    if (fwrite(reg_buf, 1, 128, f) != 128) {
        fclose(f);
        return 0;
    }

    // 3. Tabla de descriptores de segmentos (8 x 4 = 32 bytes, Big-Endian)
    uint8_t seg_buf[32];
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        uint16_t base = vm->segments[i].base;
        uint16_t size = vm->segments[i].size;
        seg_buf[i * 4 + 0] = (uint8_t)((base >> 8) & 0xFF);
        seg_buf[i * 4 + 1] = (uint8_t)(base & 0xFF);
        seg_buf[i * 4 + 2] = (uint8_t)((size >> 8) & 0xFF);
        seg_buf[i * 4 + 3] = (uint8_t)(size & 0xFF);
    }
    if (fwrite(seg_buf, 1, 32, f) != 32) {
        fclose(f);
        return 0;
    }

    // 4. Memoria principal (variable: vm->ram_size bytes)
    if (vm->memory && vm->ram_size > 0) {
        if (fwrite(vm->memory, 1, vm->ram_size, f) != vm->ram_size) {
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return 1;
}

int vmi_load(VM *vm, const char *filepath) {
    if (!vm || !filepath) {
        return 0;
    }

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "Error: No se pudo abrir la imagen '%s'\n", filepath);
        return 0;
    }

    // 1. Leer y validar Header (8 bytes)
    uint8_t header[8];
    if (fread(header, 1, 8, f) != 8) {
        fprintf(stderr, "Error: Archivo de imagen '%s' corrupto o incompleto\n", filepath);
        fclose(f);
        return 0;
    }

    if (header[0] != 'V' || header[1] != 'M' || header[2] != 'I' ||
        header[3] != '2' || header[4] != '6') {
        fprintf(stderr, "Error: Identificador invalido de imagen (esperado 'VMI26')\n");
        fclose(f);
        return 0;
    }

    if (header[5] != 1) {
        fprintf(stderr, "Error: Version de imagen no soportada (%u)\n", header[5]);
        fclose(f);
        return 0;
    }

    uint16_t ram_kib = ((uint16_t)header[6] << 8) | (uint16_t)header[7];
    uint32_t needed_ram = (uint32_t)ram_kib * 1024;

    // Asegurar que la memoria RAM este alocada con el tamaño de la imagen
    if (vm->memory == NULL || vm->ram_size != needed_ram) {
        if (vm->memory) {
            free(vm->memory);
        }
        vm->ram_size = needed_ram;
        vm->memory = (uint8_t *)malloc(vm->ram_size);
        if (!vm->memory) {
            fprintf(stderr, "Error: Memoria insuficiente para cargar la imagen\n");
            fclose(f);
            return 0;
        }
    }

    // 2. Leer registros (128 bytes)
    uint8_t reg_buf[128];
    if (fread(reg_buf, 1, 128, f) != 128) {
        fprintf(stderr, "Error: No se pudieron leer los registros de la imagen\n");
        fclose(f);
        return 0;
    }
    for (int i = 0; i < NUM_REGISTERS; i++) {
        uint32_t val = ((uint32_t)reg_buf[i * 4 + 0] << 24) |
                       ((uint32_t)reg_buf[i * 4 + 1] << 16) |
                       ((uint32_t)reg_buf[i * 4 + 2] << 8)  |
                       ((uint32_t)reg_buf[i * 4 + 3]);
        vm->registers[i] = (int32_t)val;
    }

    // 3. Leer tabla de descriptores de segmentos (32 bytes)
    uint8_t seg_buf[32];
    if (fread(seg_buf, 1, 32, f) != 32) {
        fprintf(stderr, "Error: No se pudo leer la tabla de descriptores de la imagen\n");
        fclose(f);
        return 0;
    }
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        vm->segments[i].base = ((uint16_t)seg_buf[i * 4 + 0] << 8) | (uint16_t)seg_buf[i * 4 + 1];
        vm->segments[i].size = ((uint16_t)seg_buf[i * 4 + 2] << 8) | (uint16_t)seg_buf[i * 4 + 3];
    }

    // 4. Leer memoria principal
    if (fread(vm->memory, 1, vm->ram_size, f) != vm->ram_size) {
        fprintf(stderr, "Error: No se pudo leer la memoria principal de la imagen\n");
        fclose(f);
        return 0;
    }

    fclose(f);
    vm->running = true;
    return 1;
}

void vmi_handle_breakpoint(VM *vm) {
    if (!vm || vm->vmi_path == NULL) {
        return; // Sin archivo .vmi, se ignora el breakpoint
    }

    // Generar archivo de imagen con el estado actual
    if (!vmi_save(vm, vm->vmi_path)) {
        fprintf(stderr, "Error al generar imagen '%s'\n", vm->vmi_path);
    }

    // Esperar accion del usuario: 'g' (go), 'q' (quit), o Enter (paso a paso)
    char line[128];
    while (1) {
        if (!fgets(line, sizeof(line), stdin)) {
            vm->running = false;
            return;
        }

        // Si solo se presiono Enter ('\n' o "\r\n")
        if (line[0] == '\n' || line[0] == '\r') {
            vm->step_mode = true;
            return; // Ejecuta la siguiente instruccion y vuelve a pausar
        }

        if (line[0] == 'g' || line[0] == 'G') {
            vm->step_mode = false;
            return; // Continua ejecucion normal
        }

        if (line[0] == 'q' || line[0] == 'Q') {
            vm->running = false;
            exit(0); // Aborta la ejecucion dejando intacto el archivo .vmi
        }
    }
}


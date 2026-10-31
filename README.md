# MV-2026 v2.0

Máquina Virtual - TP Arquitectura de Computadoras

**Integrantes:** Nicolas Cano - Agustin Simonte Del Russo

## Requisitos

- gcc
- make

## Compilación y ejecución

```bash
make
./vmx <archivo.vmx>
./vmx <archivo.vmx> -d     # modo desensamblado
./vmx <archivo.vmx> <archivo.vmi>  # guardar imagen en los breakpoints
./vmx <archivo.vmi>                # reanudar desde una imagen
./vmx <archivo.vmx> m=32 -p arg1 arg2  # RAM en KiB y parámetros
```

## Limpieza

```bash
make clean
```


# TP Sistemas Operativos 1er Cuatrimestre 2024 - C-Comenta

Este proyecto es un trabajo práctico que simula un sistema distribuido, implementando conceptos clave de sistemas operativos como planificación de procesos, administración de memoria y sistema de archivos.

Incluye los siguientes módulos:
- Kernel:
  - Planificación de procesos (algoritmos FIFO, Round Robin y Virtual Round Robin)
  - Manejo de interfaces
  - Manejo de recursos (semaforos)
- CPU:
  - Ejecución de instrucciones
  - MMU
- Memoria:
  - Asignación de memoria a procesos mediante paginación simple
- Interfaces de I/O
  - Sleep
  - STDIN
  - STDOUT
  - Filesystem (asignación contigua de bloques)

Enunciado: [C - Comenta](docs/enunciado.pdf)

## Instalación

### Dependencias

Para poder compilar y ejecutar el proyecto, es necesario tener instalada la
biblioteca [so-commons-library]:

```bash
git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
make debug
make install
```

### Compilación

Cada módulo del proyecto se compila de forma independiente a través de un
archivo `makefile`. Para compilar un módulo, es necesario ejecutar el comando
`make` desde la carpeta correspondiente.

El ejecutable resultante se guardará en la carpeta `bin` del módulo.

## Uso

1. Configurar las rutas a las carpetas de scripts en los archivos `.config` de kernel y memoria.
2. Modificar las IPs de los módulos en caso de ser ejecutado en múltiples computadoras.
3. Ejecutar los módulos en el orden: Memoria -> CPU -> Kernel -> Interfaces 

[so-commons-library]: https://github.com/sisoputnfrba/so-commons-library

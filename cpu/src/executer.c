#include "main.h"

static uint32_t get_registro(t_registro registro);
static void set_registro(t_registro registro, uint32_t valor, bool *incrementar_pc);
static size_t tam_registro(t_registro registro);

// Funciones para ops
static void exec_set(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_mov_in(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_mov_out(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_sum(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_sub(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_jnz(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_resize(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_copy_string(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_wait(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_signal(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_io_gen_sleep(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_io_stdin_read(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_io_stdout_write(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_io_fs_create(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_io_fs_delete(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_io_fs_truncate(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_io_fs_write(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_io_fs_read(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);
static void exec_exit(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch);

void execute(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    switch (instruccion.opcode) {
    case SET:
        exec_set(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case MOV_IN:
        exec_mov_in(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case MOV_OUT:
        exec_mov_out(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case SUM:
        exec_sum(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case SUB:
        exec_sub(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case JNZ:
        exec_jnz(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case RESIZE:
        exec_resize(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case COPY_STRING:
        exec_copy_string(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case WAIT:
        exec_wait(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case SIGNAL:
        exec_signal(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case IO_GEN_SLEEP:
        exec_io_gen_sleep(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case IO_STDIN_READ:
        exec_io_stdin_read(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case IO_STDOUT_WRITE:
        exec_io_stdout_write(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case IO_FS_CREATE:
        exec_io_fs_create(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case IO_FS_DELETE:
        exec_io_fs_delete(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case IO_FS_TRUNCATE:
        exec_io_fs_truncate(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case IO_FS_WRITE:
        exec_io_fs_write(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case IO_FS_READ:
        exec_io_fs_read(instruccion, incrementar_pc, conexion_dispatch);
        break;
    case EXIT:
        exec_exit(instruccion, incrementar_pc, conexion_dispatch);
        break;
    default:
        abort(); // Unreachable
    }
}

static uint32_t get_registro(t_registro registro)
{
    switch (registro) {
    case PC:
        return pcb->program_counter;
    case AX:
        return pcb->registros.ax;
    case BX:
        return pcb->registros.bx;
    case CX:
        return pcb->registros.cx;
    case DX:
        return pcb->registros.dx;
    case EAX:
        return pcb->registros.eax;
    case EBX:
        return pcb->registros.ebx;
    case ECX:
        return pcb->registros.ecx;
    case EDX:
        return pcb->registros.edx;
    case SI:
        return pcb->registros.si;
    case DI:
        return pcb->registros.di;
    default:
        abort(); // Unreachable
    }
}

static void set_registro(t_registro registro, uint32_t valor, bool *incrementar_pc)
{
    // Ver que no nos pasamos del tamanio maximo
    // TODO ver si es necesario checkear overflows
    assert(tam_registro(registro) > sizeof(uint8_t) || valor <= UINT8_MAX);

    switch (registro) {
    case PC:
        pcb->program_counter = valor;
        *incrementar_pc = false; // Si modificamos PC no por IO hay que autoincrementarlo.
        break;
    case AX:
        pcb->registros.ax = valor;
        break;
    case BX:
        pcb->registros.bx = valor;
        break;
    case CX:
        pcb->registros.cx = valor;
        break;
    case DX:
        pcb->registros.dx = valor;
        break;
    case EAX:
        pcb->registros.eax = valor;
        break;
    case EBX:
        pcb->registros.ebx = valor;
        break;
    case ECX:
        pcb->registros.ecx = valor;
        break;
    case EDX:
        pcb->registros.edx = valor;
        break;
    case SI:
        pcb->registros.si = valor;
        break;
    case DI:
        pcb->registros.di = valor;
        break;
    default:
        abort(); // Unreachable.
    }
}

// Devuelve el tamanio en bytes de un registro
static size_t tam_registro(t_registro registro)
{
    switch (registro) {
    case AX:
    case BX:
    case CX:
    case DX:
        return sizeof(uint8_t);
    case PC:
    case EAX:
    case EBX:
    case ECX:
    case EDX:
    case SI:
    case DI:
        return sizeof(uint32_t);
    default:
        abort(); // Unreachable.
    }
}

/*
** Funciones de operaciones
*/

static void exec_set(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    set_registro(instruccion.parametros[0].registro, instruccion.parametros[1].num, incrementar_pc);
}

static void exec_mov_in(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    t_registro reg_datos = instruccion.parametros[0].registro;
    size_t dir_logica = get_registro(instruccion.parametros[1].registro);

    void *buffer_lectura = leer_espacio_usuario(pcb->pid, dir_logica, tam_registro(reg_datos));

    uint32_t valor_leido =
        (tam_registro(reg_datos) == sizeof(uint32_t)) ? *(uint32_t *)buffer_lectura : *(uint8_t *)buffer_lectura;
    set_registro(reg_datos, valor_leido, incrementar_pc);

    free(buffer_lectura);
}

static void exec_mov_out(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    size_t dir_logica = get_registro(instruccion.parametros[0].registro);
    t_registro reg_datos = instruccion.parametros[1].registro;

    uint32_t valor_datos = get_registro(reg_datos);

    escribir_espacio_usuario(pcb->pid, dir_logica, &valor_datos, tam_registro(reg_datos));
}

static void exec_sum(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    uint32_t valor_destino = get_registro(instruccion.parametros[0].registro);
    uint32_t valor_origen = get_registro(instruccion.parametros[1].registro);
    set_registro(instruccion.parametros[0].registro, valor_destino + valor_origen, incrementar_pc);
}

static void exec_sub(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    uint32_t valor_destino = get_registro(instruccion.parametros[0].registro);
    uint32_t valor_origen = get_registro(instruccion.parametros[1].registro);
    set_registro(instruccion.parametros[0].registro,
                 valor_destino - valor_origen, // TODO: Que pasa si la resta produce un valor negativo
                 incrementar_pc);
}

static void exec_jnz(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    uint32_t valor_registro = get_registro(instruccion.parametros[0].registro);
    if (valor_registro != 0) {
        set_registro(PC, (uint32_t)instruccion.parametros[1].num, incrementar_pc);
    }
}

static void exec_resize(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    uint32_t tamanio = (uint32_t)instruccion.parametros[0].num;

    // Enviar opcode
    enviar_int(OPCODE_AJUSTAR_TAMANIO_PROCESO, conexion_memoria);

    // Enviar pid y nuevo tamanio
    t_paquete *p = crear_paquete();
    agregar_a_paquete(p, &(pcb->pid), sizeof(uint32_t));
    agregar_a_paquete(p, &tamanio, sizeof(uint32_t));
    enviar_paquete(p, conexion_memoria);
    eliminar_paquete(p);

    // Esperar la respuesta de memoria
    bool err = false;
    t_respuesta_resize respuesta = recibir_int(conexion_memoria, &err);
    if (err) {
        log_error(debug_logger, "Hubo un error en la conexion con memoria");
        abort();
    }
    if (respuesta == R_RESIZE_OUT_OF_MEMORY) {
        log_info(debug_logger, "Out of memory, enviando proceso a exit");
        devolver_pcb(OUT_OF_MEMORY, conexion_dispatch);
        return;
    } else if (respuesta == R_RESIZE_SUCCESS) {
        log_info(debug_logger, "Se amplio el proceso a %u bytes", tamanio);
    } else {
        log_error(debug_logger, "Memoria no envio un msg correcto");
        abort();
    }
}

static void exec_copy_string(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    size_t tamanio = (size_t)instruccion.parametros[0].num;
    size_t inicio_lectura = (size_t)get_registro(SI);
    size_t inicio_escritura = (size_t)get_registro(DI);

    void *buf_string = leer_espacio_usuario(pcb->pid, inicio_lectura, tamanio);
    escribir_espacio_usuario(pcb->pid, inicio_escritura, buf_string, tamanio);

    free(buf_string);
}

static void exec_wait(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    char *recurso = instruccion.parametros[0].str;

    // TODO mover estos pc++ a otro lado
    // Siempre antes de devolver el pcb por IO hay que incrementar el pc
    pcb->program_counter++;
    devolver_pcb(WAIT_RECURSO, conexion_dispatch);

    // Enviar el nombre del recurso
    enviar_str(recurso, conexion_dispatch);
}

static void exec_signal(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    char *recurso = instruccion.parametros[0].str;

    // Siempre antes de devolver el pcb por IO hay que incrementar el pc
    pcb->program_counter++;
    devolver_pcb(SIGNAL_RECURSO, conexion_dispatch);

    // Enviar el nombre del recurso
    enviar_str(recurso, conexion_dispatch);
}

//
// Funciones IO
//

static void exec_io_gen_sleep(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    char *nombre_interfaz = instruccion.parametros[0].str;
    uint32_t unidades_trabajo = instruccion.parametros[1].num;

    // Siempre antes de devolver el pcb por IO hay que incrementar el pc
    pcb->program_counter++;
    devolver_pcb(IO, conexion_dispatch);

    // Enviar nombre de interfaz y unidades de trabajo.
    t_paquete *paquete = crear_paquete();
    t_operacion_io op = GEN_SLEEP;
    agregar_a_paquete(paquete, nombre_interfaz, strlen(nombre_interfaz) + 1);
    agregar_a_paquete(paquete, &op, sizeof(t_operacion_io));
    agregar_a_paquete(paquete, &unidades_trabajo, sizeof(uint32_t));

    enviar_paquete(paquete, conexion_dispatch);

    eliminar_paquete(paquete);
}

static void exec_io_stdin_read(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    char *nombre_interfaz = instruccion.parametros[0].str;
    size_t dir_logica_inicio = get_registro(instruccion.parametros[1].registro);
    size_t tamanio_total = get_registro(instruccion.parametros[2].registro);

    t_list *bloques = obtener_bloques(pcb->pid, dir_logica_inicio, tamanio_total);

    // Siempre antes de devolver el pcb por IO hay que incrementar el pc
    pcb->program_counter++;
    devolver_pcb(IO, conexion_dispatch);

    t_paquete *p = crear_paquete();
    t_operacion_io op = STDIN_READ;
    agregar_a_paquete(p, nombre_interfaz, strlen(nombre_interfaz) + 1); // Nombre Interfaz
    agregar_a_paquete(p, &op, sizeof(t_operacion_io));                  // Operacion
    agregar_a_paquete(p, &tamanio_total, sizeof(size_t));               // Tamanio Total
    agregar_lista_homogenea_a_paquete(p, bloques, sizeof(t_bloque));    // Bloques

    enviar_paquete(p, conexion_dispatch);
    eliminar_paquete(p);

    list_destroy_and_destroy_elements(bloques, free);
}

static void exec_io_stdout_write(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    char *nombre_interfaz = instruccion.parametros[0].str;
    size_t dir_logica_inicio = get_registro(instruccion.parametros[1].registro);
    size_t tamanio_total = get_registro(instruccion.parametros[2].registro);

    t_list *bloques = obtener_bloques(pcb->pid, dir_logica_inicio, tamanio_total);

    // Siempre antes de devolver el pcb por IO hay que incrementar el pc
    pcb->program_counter++;
    devolver_pcb(IO, conexion_dispatch);

    t_paquete *p = crear_paquete();
    t_operacion_io op = STDOUT_WRITE;
    agregar_a_paquete(p, nombre_interfaz, strlen(nombre_interfaz) + 1); // Nombre Interfaz
    agregar_a_paquete(p, &op, sizeof(t_operacion_io));                  // Operacion
    agregar_a_paquete(p, &tamanio_total, sizeof(size_t));               // Tamanio Total
    agregar_lista_homogenea_a_paquete(p, bloques, sizeof(t_bloque));    // Bloques

    enviar_paquete(p, conexion_dispatch);
    eliminar_paquete(p);

    list_destroy_and_destroy_elements(bloques, free);
}

static void exec_io_fs_create(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    char *nombre_interfaz = instruccion.parametros[0].str;
    char *nombre_archivo = instruccion.parametros[1].str;

    // Siempre antes de devolver el pcb por IO hay que incrementar el pc
    pcb->program_counter++;
    devolver_pcb(IO, conexion_dispatch);

    t_paquete *p = crear_paquete();

    t_operacion_io op = FS_CREATE;
    agregar_a_paquete(p, nombre_interfaz, strlen(nombre_interfaz) + 1); // Nombre Interfaz
    agregar_a_paquete(p, &op, sizeof(t_operacion_io));                  // Operacion
    agregar_a_paquete(p, nombre_archivo, strlen(nombre_archivo) + 1);   // Nombre Archivo

    enviar_paquete(p, conexion_dispatch);

    eliminar_paquete(p);
}

static void exec_io_fs_delete(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    char *nombre_interfaz = instruccion.parametros[0].str;
    char *nombre_archivo = instruccion.parametros[1].str;

    // Siempre antes de devolver el pcb por IO hay que incrementar el pc
    pcb->program_counter++;
    devolver_pcb(IO, conexion_dispatch);

    t_paquete *p = crear_paquete();

    t_operacion_io op = FS_DELETE;
    agregar_a_paquete(p, nombre_interfaz, strlen(nombre_interfaz) + 1); // Nombre Interfaz
    agregar_a_paquete(p, &op, sizeof(t_operacion_io));                  // Operacion
    agregar_a_paquete(p, nombre_archivo, strlen(nombre_archivo) + 1);   // Nombre Archivo

    enviar_paquete(p, conexion_dispatch);

    eliminar_paquete(p);
}

static void exec_io_fs_truncate(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    char *nombre_interfaz = instruccion.parametros[0].str;
    char *nombre_archivo = instruccion.parametros[1].str;
    size_t tamanio = get_registro(instruccion.parametros[2].registro);

    // Siempre antes de devolver el pcb por IO hay que incrementar el pc
    pcb->program_counter++;
    devolver_pcb(IO, conexion_dispatch);

    t_paquete *p = crear_paquete();

    t_operacion_io op = FS_TRUNCATE;
    agregar_a_paquete(p, nombre_interfaz, strlen(nombre_interfaz) + 1); // Nombre Interfaz
    agregar_a_paquete(p, &op, sizeof(t_operacion_io));                  // Operacion
    agregar_a_paquete(p, nombre_archivo, strlen(nombre_archivo) + 1);   // Nombre Archivo
    agregar_a_paquete(p, &tamanio, sizeof(size_t));                     // Tamanio nuevo

    enviar_paquete(p, conexion_dispatch);

    eliminar_paquete(p);
}

static void exec_io_fs_write(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    char *nombre_interfaz = instruccion.parametros[0].str;
    char *nombre_archivo = instruccion.parametros[1].str;
    size_t dir_logica_inicio = get_registro(instruccion.parametros[2].registro);
    size_t tamanio = get_registro(instruccion.parametros[3].registro);
    size_t dir_archivo_inicio = get_registro(instruccion.parametros[4].registro);

    t_list *bloques = obtener_bloques(pcb->pid, dir_logica_inicio, tamanio);

    // Siempre antes de devolver el pcb por IO hay que incrementar el pc
    pcb->program_counter++;
    devolver_pcb(IO, conexion_dispatch);

    t_paquete *p = crear_paquete();

    t_operacion_io op = FS_WRITE;
    agregar_a_paquete(p, nombre_interfaz, strlen(nombre_interfaz) + 1); // Nombre Interfaz
    agregar_a_paquete(p, &op, sizeof(t_operacion_io));                  // Operacion
    agregar_a_paquete(p, nombre_archivo, strlen(nombre_archivo) + 1);   // Nombre Archivo
    agregar_a_paquete(p, &dir_archivo_inicio, sizeof(size_t));          // Direccion de inicio del archivo
    agregar_a_paquete(p, &tamanio, sizeof(size_t));                     // Tamanio total
    agregar_lista_homogenea_a_paquete(p, bloques, sizeof(t_bloque));    // Bloques

    enviar_paquete(p, conexion_dispatch);
    eliminar_paquete(p);

    list_destroy_and_destroy_elements(bloques, free);
}

static void exec_io_fs_read(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    char *nombre_interfaz = instruccion.parametros[0].str;
    char *nombre_archivo = instruccion.parametros[1].str;
    size_t dir_logica_inicio = get_registro(instruccion.parametros[2].registro);
    size_t tamanio = get_registro(instruccion.parametros[3].registro);
    size_t dir_archivo_inicio = get_registro(instruccion.parametros[4].registro);

    t_list *bloques = obtener_bloques(pcb->pid, dir_logica_inicio, tamanio);

    // Siempre antes de devolver el pcb por IO hay que incrementar el pc
    pcb->program_counter++;
    devolver_pcb(IO, conexion_dispatch);

    t_paquete *p = crear_paquete();

    t_operacion_io op = FS_READ;
    agregar_a_paquete(p, nombre_interfaz, strlen(nombre_interfaz) + 1); // Nombre Interfaz
    agregar_a_paquete(p, &op, sizeof(t_operacion_io));                  // Operacion
    agregar_a_paquete(p, nombre_archivo, strlen(nombre_archivo) + 1);   // Nombre Archivo
    agregar_a_paquete(p, &dir_archivo_inicio, sizeof(size_t));          // Direccion de inicio del archivo
    agregar_a_paquete(p, &tamanio, sizeof(size_t));                     // Tamanio total
    agregar_lista_homogenea_a_paquete(p, bloques, sizeof(t_bloque));    // Bloques

    enviar_paquete(p, conexion_dispatch);
    eliminar_paquete(p);

    list_destroy_and_destroy_elements(bloques, free);
}

static void exec_exit(t_instruccion instruccion, bool *incrementar_pc, int conexion_dispatch)
{
    devolver_pcb(FIN_PROCESO, conexion_dispatch);
}

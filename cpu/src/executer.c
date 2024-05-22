#include "executer.h"

void execute(t_instruccion instruccion_a_ejecutar, bool *incrementar_pc, int conexion_dispatch)
{

    switch (instruccion_a_ejecutar.opcode) {
    case SET: {
        set_registro(instruccion_a_ejecutar.parametros[0].registro,
                     instruccion_a_ejecutar.parametros[1].valor_numerico,
                     incrementar_pc);
        break;
    }
    case SUM: {
        uint32_t valor_destino = get_registro(instruccion_a_ejecutar.parametros[0].registro);
        uint32_t valor_origen = get_registro(instruccion_a_ejecutar.parametros[1].registro);
        set_registro(instruccion_a_ejecutar.parametros[0].registro, valor_destino + valor_origen, incrementar_pc);
        break;
    }
    case SUB: {
        uint32_t valor_destino = get_registro(instruccion_a_ejecutar.parametros[0].registro);
        uint32_t valor_origen = get_registro(instruccion_a_ejecutar.parametros[1].registro);
        set_registro(instruccion_a_ejecutar.parametros[0].registro,
                     valor_destino - valor_origen, // TODO: Que pasa si la resta produce un valor negativo
                     incrementar_pc);
        break;
    }
    case JNZ: {
        uint32_t valor_registro = get_registro(instruccion_a_ejecutar.parametros[0].registro);
        if (valor_registro != 0) {
            set_registro(PC, (uint32_t)instruccion_a_ejecutar.parametros[1].valor_numerico, incrementar_pc);
        }
        break;
    }
    case IO_GEN_SLEEP: {
        char *nombre_interfaz = instruccion_a_ejecutar.parametros[0].str;
        uint32_t unidades_trabajo = instruccion_a_ejecutar.parametros[1].valor_numerico;

        devolver_pcb(IO, conexion_dispatch);

        // Enviar nombre de interfaz y unidades de trabajo.
        t_paquete *paquete = crear_paquete();
        agregar_a_paquete(paquete, nombre_interfaz, strlen(nombre_interfaz) + 1);
        t_operacion_io op = GEN_SLEEP;
        agregar_a_paquete(paquete, &op, sizeof(t_operacion_io));
        agregar_a_paquete(paquete, &unidades_trabajo, sizeof(uint32_t));

        enviar_paquete(paquete, conexion_dispatch);

        eliminar_paquete(paquete);

        break;
    }
    case EXIT: {
        devolver_pcb(FIN_PROCESO, conexion_dispatch);
        break;
    }
    default: {
        log_error(debug_logger, "Funcion no implementada");
        exit(1);
        break;
    }
    }
}

uint32_t get_registro(t_registro registro)
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
        assert(false && "Registro invalido");
    }
}

void set_registro(t_registro registro, int valor, bool *incrementar_pc)
{
    switch (registro) {
    case PC:
        pcb->program_counter = valor;
        *incrementar_pc = false; // Si modificamos PC no hay que autoincrementarlo.
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
    }
}

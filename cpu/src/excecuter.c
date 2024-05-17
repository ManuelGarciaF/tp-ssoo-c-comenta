#include "excecuter.h"

void excecute(t_instruccion instruccion_a_ejecutar)
{
    switch (instruccion_a_ejecutar.opcode) {
    case SET: {
        set_registro(instruccion_a_ejecutar.parametros[0].registro,
                     instruccion_a_ejecutar.parametros[1].valor_numerico);
        break;
    }
    case SUM: {
        uint32_t valor_destino_sum = get_registro(instruccion_a_ejecutar.parametros[0].registro);
        uint32_t valor_origen_sum = get_registro(instruccion_a_ejecutar.parametros[1].registro);
        set_registro(instruccion_a_ejecutar.parametros[0].registro, valor_destino_sum + valor_origen_sum);
        break;
    }
    case SUB: {
        uint32_t valor_destino_sub = get_registro(instruccion_a_ejecutar.parametros[0].registro);
        uint32_t valor_origen_sub = get_registro(instruccion_a_ejecutar.parametros[1].registro);
        set_registro(instruccion_a_ejecutar.parametros[0].registro,
                     valor_destino_sub - valor_origen_sub); // TODO: Que pasa si la resta produce un valor negativo
        break;
    }
    case JNZ: {
        uint32_t valor_registro = get_registro(instruccion_a_ejecutar.parametros[0].registro);
        if (valor_registro != 0) {
            pcb->program_counter = instruccion_a_ejecutar.parametros[1].valor_numerico;
        }
        break;
    }

    case IO_GEN_SLEEP: {
        // Completar
        break;
    }

    default: {
        log_error(debug_logger, "Funcion no implementada");
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
    }
}

void set_registro(t_registro registro, int valor)
{
    switch (registro) {
    case PC:
        pcb->program_counter = valor;
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

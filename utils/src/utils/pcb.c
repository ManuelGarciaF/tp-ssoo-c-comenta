#include "pcb.h"

t_pcb *pcb_create(uint32_t pid)
{
    t_pcb *pcb = malloc(sizeof(t_pcb));
    if (pcb == NULL) {
        log_error(debug_logger, "No se pudo alojar memoria");
        abort();
    }

    // Settear todo en 0.
    memset(pcb, 0, sizeof(t_pcb));
    // Usar el pcb dado.
    pcb->pid = pid;

    return pcb;
}

void pcb_destroy(t_pcb *pcb)
{
    free(pcb);
}

void pcb_send(t_pcb *pcb, int socket_conexion)
{
    t_paquete *paquete = crear_paquete();
    agregar_a_paquete(paquete, pcb, sizeof(t_pcb));

    enviar_paquete(paquete, socket_conexion);

    eliminar_paquete(paquete);
}

t_pcb *pcb_receive(int socket_conexion)
{
    bool err = false;
    t_list *contenido = recibir_paquete(socket_conexion, &err);
    if (err) {
        return NULL;
    }

    t_pcb *pcb = list_get(contenido, 0);
    list_destroy(contenido);
    return pcb;
}

void pcb_debug_print(t_pcb *pcb)
{
    log_info(debug_logger,
             "PCB: {PID: %u, PC %u, Q: %ld, registros:{AX:%u, BX:%u, CX:%u, DX:%u, EAX:%u, EBX:%u, ECX:%u, "
             "EDX:%u, SI:%u, DI:%u}}",
             pcb->pid,
             pcb->program_counter,
             pcb->quantum,
             pcb->registros.ax,
             pcb->registros.bx,
             pcb->registros.cx,
             pcb->registros.dx,
             pcb->registros.eax,
             pcb->registros.ebx,
             pcb->registros.ecx,
             pcb->registros.edx,
             pcb->registros.si,
             pcb->registros.di);
}

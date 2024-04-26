#include "pcb.h"

t_pcb *pcb_create(uint32_t pid)
{
    t_registros registros = {0};

    t_pcb *pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->quantum = 0;
    pcb->registros = registros;

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
    t_list *contenido = recibir_paquete(socket_conexion);
    t_pcb *pcb = list_get(contenido, 0);
    list_destroy(contenido);
    return pcb;
}

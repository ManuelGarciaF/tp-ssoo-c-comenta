#include "sockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
** Lado del cliente
*/

void *serializar_paquete(t_paquete *paquete, int bytes)
{
    void *magic = malloc(bytes);
    int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento,
           paquete->buffer->stream,
           paquete->buffer->size);
    desplazamiento += paquete->buffer->size;

    return magic;
}

int crear_conexion(char *ip, char *puerto)
{
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ip, puerto, &hints, &server_info) != 0) {
        // FIXME: Arreglar logging
        /* log_error(logger, "No se pudo crear server_info"); */
        exit(1);
    }

    // Ahora vamos a crear el socket.
    int socket_cliente = socket(server_info->ai_family,
                                server_info->ai_socktype,
                                server_info->ai_protocol);

    // Ahora que tenemos el socket, vamos a conectarlo
    if (connect(socket_cliente,
                server_info->ai_addr,
                server_info->ai_addrlen) != 0) {
        // FIXME: Arreglar logging
        /* log_error(logger, "No se pudo conectar al servidor"); */
        exit(1);
    }

    freeaddrinfo(server_info);

    return socket_cliente;
}

void enviar_mensaje(char *mensaje, int socket_cliente)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = MENSAJE;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = strlen(mensaje) + 1;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

    int bytes = paquete->buffer->size + 2 * sizeof(int);

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
    eliminar_paquete(paquete);
}

void crear_buffer(t_paquete *paquete)
{
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = 0;
    paquete->buffer->stream = NULL;
}

t_paquete *crear_paquete(void)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = PAQUETE;
    crear_buffer(paquete);
    return paquete;
}

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
{
    paquete->buffer->stream =
        realloc(paquete->buffer->stream,
                paquete->buffer->size + tamanio + sizeof(int));

    memcpy(paquete->buffer->stream + paquete->buffer->size,
           &tamanio,
           sizeof(int));
    memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int),
           valor,
           tamanio);

    paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete *paquete, int socket_cliente)
{
    int bytes = paquete->buffer->size + 2 * sizeof(int);
    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
}

void eliminar_paquete(t_paquete *paquete)
{
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void liberar_conexion(int socket_cliente)
{
    close(socket_cliente);
}

/*
** Lado del servidor
*/

int iniciar_servidor(char *puerto)
{
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, puerto, &hints, &servinfo) != 0) {
        // FIXME: Arreglar logging
        /* log_error(logger, "No se pudo crear servinfo"); */
        exit(1);
    }

    // Creamos el socket de escucha del servidor
    int socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype,
                                 servinfo->ai_protocol);

    // Asociamos el socket a un puerto
    if (bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen) != 0) {
        // FIXME: Arreglar logging
        /* log_error(logger, "No se pudo bindear el socket."); */
        exit(1);
    }

    // Escuchamos las conexiones entrantes
    listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);
    // FIXME: Arreglar logging
    /* log_trace(logger, "Listo para escuchar a mi cliente"); */

    return socket_servidor;
}

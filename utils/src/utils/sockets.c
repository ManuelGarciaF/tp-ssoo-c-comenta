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
        log_error(debug_logger, "No se pudo crear server_info");
        exit(1);
    }

    // Ahora vamos a crear el socket.
    int socket_conexion = socket(server_info->ai_family,
                                 server_info->ai_socktype,
                                 server_info->ai_protocol);

    // Ahora que tenemos el socket, vamos a conectarlo
    if (connect(socket_conexion,
                server_info->ai_addr,
                server_info->ai_addrlen) != 0) {
        log_error(debug_logger, "No se pudo conectar al servidor");
        exit(1);
    }

    freeaddrinfo(server_info);

    return socket_conexion;
}

void enviar_mensaje(char *mensaje, int socket_conexion)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = MENSAJE;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = strlen(mensaje) + 1;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

    int bytes = paquete->buffer->size + 2 * sizeof(int);

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_conexion, a_enviar, bytes, 0);

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

void enviar_paquete(t_paquete *paquete, int socket_conexion)
{
    int bytes = paquete->buffer->size + 2 * sizeof(int);
    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_conexion, a_enviar, bytes, 0);

    free(a_enviar);
}

void eliminar_paquete(t_paquete *paquete)
{
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void liberar_conexion(int socket_conexion)
{
    close(socket_conexion);
}

/*
** Lado del servidor
*/

int iniciar_servidor(char *puerto)
{
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, puerto, &hints, &servinfo) != 0) {
        log_error(debug_logger, "No se pudo crear servinfo");
        exit(1);
    }

    // Creamos el socket de escucha del servidor
    int socket_escucha = socket(servinfo->ai_family,
                                servinfo->ai_socktype,
                                servinfo->ai_protocol);

    // Asociamos el socket a un puerto
    if (bind(socket_escucha, servinfo->ai_addr, servinfo->ai_addrlen) != 0) {
        log_error(debug_logger, "No se pudo bindear el socket.");
        exit(1);
    }

    // Escuchamos las conexiones entrantes
    listen(socket_escucha, SOMAXCONN);

    freeaddrinfo(servinfo);

    return socket_escucha;
}

int esperar_cliente(int socket_escucha)
{
    // Aceptamos un nuevo cliente
    int socket_conexion = accept(socket_escucha, NULL, NULL);
    if (socket_conexion < 0) {
        log_error(debug_logger, "Hubo un error aceptando la conexión");
        exit(1);
    }
    log_trace(debug_logger, "Se conecto un cliente");

    return socket_conexion;
}

int recibir_operacion(int socket_conexion)
{
    int cod_op;
    if (recv(socket_conexion, &cod_op, sizeof(int), MSG_WAITALL) > 0)
        return cod_op;
    else {
        close(socket_conexion);
        return -1;
    }
}

void *recibir_buffer(int *size, int socket_conexion)
{
    void *buffer;

    recv(socket_conexion, size, sizeof(int), MSG_WAITALL);
    buffer = malloc(*size);
    recv(socket_conexion, buffer, *size, MSG_WAITALL);

    return buffer;
}

char *recibir_mensaje(int socket_conexion)
{
    int size;
    char *buffer = recibir_buffer(&size, socket_conexion);
    return buffer;
}

t_list *recibir_paquete(int socket_conexion)
{
    int size;
    int desplazamiento = 0;
    void *buffer;
    t_list *valores = list_create();
    int tamanio;

    buffer = recibir_buffer(&size, socket_conexion);
    while (desplazamiento < size) {
        memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
        desplazamiento += sizeof(int);
        char *valor = malloc(tamanio);
        memcpy(valor, buffer + desplazamiento, tamanio);
        desplazamiento += tamanio;
        list_add(valores, valor);
    }
    free(buffer);
    return valores;
}

bool realizar_handshake(int socket_conexion)
{
    size_t bytes;

    // Enviar handshake
    uint32_t msg = MENSAJE_HANDSHAKE;
    bytes = send(socket_conexion, &msg, sizeof(uint32_t), 0);
    if (bytes <= 0) {
        log_error(debug_logger, "No se pudo enviar el handshake");
        exit(1);
    }
    uint32_t respuesta;
    recv(socket_conexion, &respuesta, sizeof(uint32_t), MSG_WAITALL);

    // Verifico que la respuesta sea la correcta
    return respuesta == RESPUESTA_HANDSHAKE_OK;
}

bool recibir_handshake(int socket_conexion)
{
    uint32_t mensaje_recibido;
    ssize_t bytes =
        recv(socket_conexion, &mensaje_recibido, sizeof(uint32_t), MSG_WAITALL);
    if (bytes <= 0) {
        log_error(debug_logger, "Hubo un error recibiendo el handshake");
        exit(1);
    }

    // Si el mensaje recibido es correcto
    uint32_t msg = (mensaje_recibido == MENSAJE_HANDSHAKE)
                       ? RESPUESTA_HANDSHAKE_OK
                       : RESPUESTA_HANDSHAKE_ERROR;
    bytes = send(socket_conexion, &msg, sizeof(uint32_t), 0);
    if (bytes <= 0) {
        log_error(debug_logger, "No se pudo enviar la respuesta al handshake");
        exit(1);
    }
    return mensaje_recibido == MENSAJE_HANDSHAKE;
}

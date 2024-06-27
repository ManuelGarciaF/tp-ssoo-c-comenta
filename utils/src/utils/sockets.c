#include "sockets.h"

#include <commons/log.h>
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
    memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
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
    int socket_conexion = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

    // Ahora que tenemos el socket, vamos a conectarlo
    if (connect(socket_conexion, server_info->ai_addr, server_info->ai_addrlen) != 0) {
        log_error(debug_logger, "No se pudo conectar al servidor");
        exit(1);
    }

    freeaddrinfo(server_info);

    return socket_conexion;
}

void enviar_str(char *mensaje, int socket_conexion)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = OP_MENSAJE_STR;
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

void enviar_int(uint32_t mensaje, int socket_conexion)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = OP_MENSAJE_INT;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = sizeof(uint32_t);
    paquete->buffer->stream = malloc(sizeof(uint32_t));
    *((uint32_t *)paquete->buffer->stream) = mensaje;

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
    paquete->codigo_operacion = OP_PAQUETE;
    crear_buffer(paquete);
    return paquete;
}

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
{
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

    memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
    memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

    paquete->buffer->size += tamanio + sizeof(int);
}

int enviar_paquete(t_paquete *paquete, int socket_conexion)
{
    int bytes = paquete->buffer->size + 2 * sizeof(int);
    void *a_enviar = serializar_paquete(paquete, bytes);

    int bytes_enviados = send(socket_conexion, a_enviar, bytes, 0);

    free(a_enviar);

    return bytes_enviados;
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
    int socket_escucha = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    // Evita errores en bind despues de un crash
    int reuse = 1;
    if (setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) != 0) {
        log_error(debug_logger, "No se pudo configurar la opcion SO_REUSEADDR en el socket");
        exit(1);
    }
    if (setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(int)) != 0) {
        log_error(debug_logger, "No se pudo configurar la opcion SO_REUSEPORT en el socket");
        exit(1);
    }

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

op_code recibir_operacion(int socket_conexion)
{
    int cod_op;
    if (recv(socket_conexion, &cod_op, sizeof(int), MSG_WAITALL) <= 0) {
        log_warning(debug_logger, "La conexion fue cerrada");
        close(socket_conexion);
        return -1;
    }

    return cod_op;
}

void *recibir_buffer(int *size, int socket_conexion)
{
    void *buffer;

    recv(socket_conexion, size, sizeof(int), MSG_WAITALL);
    buffer = malloc(*size);
    recv(socket_conexion, buffer, *size, MSG_WAITALL);

    return buffer;
}

char *recibir_str(int socket_conexion, bool *error)
{
    // Verificar que se envió un string
    if (recibir_operacion(socket_conexion) != OP_MENSAJE_STR) {
        log_warning(debug_logger, "recibir_str: Error al recibir la operacion");
        *error = true;
        return NULL;
    }

    int size;
    char *buffer = recibir_buffer(&size, socket_conexion);
    *error = false;
    return buffer;
}

uint32_t recibir_int(int socket_conexion, bool *error)
{
    // Verificar que se envió un int
    if (recibir_operacion(socket_conexion) != OP_MENSAJE_INT) {
        log_warning(debug_logger, "recibir_int: Error al recibir la operacion");
        *error = true;
        return -1;
    }

    int size;
    uint32_t *buffer = recibir_buffer(&size, socket_conexion);
    uint32_t valor = *buffer;
    free(buffer);

    *error = false;
    return valor;
}

t_list *recibir_paquete(int socket_conexion, bool *error)
{
    // Verificar que se envió un paquete
    if (recibir_operacion(socket_conexion) != OP_PAQUETE) {
        log_warning(debug_logger, "recibir_paquete: Error al recibir la operacion");
        *error = true;
        return NULL;
    }

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
    *error = false;
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
    ssize_t bytes = recv(socket_conexion, &mensaje_recibido, sizeof(uint32_t), MSG_WAITALL);
    if (bytes <= 0) {
        log_error(debug_logger, "Hubo un error recibiendo el handshake");
        exit(1);
    }

    // Si el mensaje recibido es correcto
    bool mensaje_correcto = mensaje_recibido == MENSAJE_HANDSHAKE;

    uint32_t msg = mensaje_correcto ? RESPUESTA_HANDSHAKE_OK : RESPUESTA_HANDSHAKE_ERROR;
    bytes = send(socket_conexion, &msg, sizeof(uint32_t), 0);
    if (bytes <= 0) {
        log_error(debug_logger, "No se pudo enviar la respuesta al handshake");
        exit(1);
    }
    return mensaje_correcto;
}

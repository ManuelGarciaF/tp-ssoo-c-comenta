#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <commons/log.h>
#include <commons/collections/list.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

// Debe estar definido
extern t_log *logger;

typedef enum { MENSAJE, PAQUETE } op_code;

typedef struct {
    int size;
    void *stream;
} t_buffer;

typedef struct {
    op_code codigo_operacion;
    t_buffer *buffer;
} t_paquete;

/* Devuelve un socket a la ip y puerto proveidos */
int crear_conexion(char *ip, char *puerto);
void liberar_conexion(int socket_cliente);

void enviar_mensaje(char *mensaje, int socket_cliente);

t_paquete *crear_paquete(void);
void eliminar_paquete(t_paquete *paquete);

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
void enviar_paquete(t_paquete *paquete, int socket_cliente);


// TODO Comentar
void *recibir_buffer(int *size, int socket_cliente);
int iniciar_servidor(char *puerto);
int esperar_cliente(int socket_servidor);
int recibir_operacion(int socket_cliente);
t_list *recibir_paquete(int socket_cliente);
void recibir_mensaje(int socket_cliente);

#endif // SOCKETS_H_

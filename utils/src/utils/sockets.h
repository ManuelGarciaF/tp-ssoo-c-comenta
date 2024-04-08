#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <commons/collections/list.h>
#include <commons/log.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

// Definiciones
#define MENSAJE_HANDSHAKE "42"
#define RESPUESTA_HANDSHAKE_OK "OK"
#define RESPUESTA_HANDSHAKE_ERROR "NO OK"

// Debe estar definido
extern t_log *debug_logger;

typedef enum {
    MENSAJE, // String simple
    PAQUETE, // Paquete compuesto de varios campos
} op_code;

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

/* Envia un string */
void enviar_mensaje(char *mensaje, int socket_cliente);

/* Devuelve un paquete vacio */
t_paquete *crear_paquete(void);
void eliminar_paquete(t_paquete *paquete);

/* Agrega tamanio bytes a un paquete */
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);

/* Envia paquete */
void enviar_paquete(t_paquete *paquete, int socket_cliente);

/* Inicia un socket como servidor en un puerto dado y lo devuelve */
int iniciar_servidor(char *puerto);

/* Bloqueante, espera un cliente y devuelve su socket */
int esperar_cliente(int socket_servidor);

/* Devuelve el codigo de operacion al inicio de un paquete
 * Debe ser llamado antes de recibir_paquete o recibir_mensaje */
int recibir_operacion(int socket_cliente);

/* Lee size bytes del socket */
void *recibir_buffer(int *size, int socket_cliente);

/* Recibe el buffer de un paquete y devuelve una lista con sus elementos */
t_list *recibir_paquete(int socket_cliente);

/* Recibe un mensaje simple y lo devuelve */
char *recibir_mensaje(int socket_cliente);

/* Realiza un handshake con el servidor y devuelve true si el servidor devuelve
 * "OK" */
bool realizar_handshake(int socket_servidor);

/* Devuelve un mensaje de "OK" al cliente si este envio un handshake valido */
void recibir_handshake(int socket_cliente);

#endif // SOCKETS_H_

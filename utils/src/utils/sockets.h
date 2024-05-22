#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <commons/collections/list.h>
#include <commons/log.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

// Definiciones
#define MENSAJE_HANDSHAKE 42
#define RESPUESTA_HANDSHAKE_OK 1
#define RESPUESTA_HANDSHAKE_ERROR 0

// Debe estar definido
extern t_log *debug_logger;

typedef enum {
    OP_MENSAJE_STR,
    OP_MENSAJE_INT,
    OP_PAQUETE, // Paquete compuesto de varios campos
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
void liberar_conexion(int conexion_cliente);

void enviar_str(char *mensaje, int socket_conexion);
void enviar_int(uint32_t mensaje, int socket_conexion);

/* Devuelve un paquete vacio */
t_paquete *crear_paquete(void);
void eliminar_paquete(t_paquete *paquete);

/* Agrega tamanio bytes a un paquete */
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);

/* Envia paquete */
void enviar_paquete(t_paquete *paquete, int socket_conexion);

/* Inicia un socket como servidor en un puerto dado y lo devuelve */
int iniciar_servidor(char *puerto);

/* Bloqueante, espera un cliente y devuelve su socket */
int esperar_cliente(int socket_escucha);

/* Recibe el codigo de operacion al inicio de un paquete */
int recibir_operacion(int socket_conexion);

/* Lee size bytes del socket */
void *recibir_buffer(int *size, int socket_conexion);

/* Recibe el buffer de un paquete y devuelve una lista con sus elementos */
t_list *recibir_paquete(int socket_conexion);

/* Recibe un mensaje simple y lo retorna */
char *recibir_str(int socket_conexion);
uint32_t recibir_int(int socket_conexion);

/* Realiza un handshake con el servidor y devuelve true si el servidor devuelve
 * 1 */
bool realizar_handshake(int socket_conexion);

/* Retorna true y envia 1 al cliente si este envio un handshake valido */
bool recibir_handshake(int socket_conexion);

#endif // SOCKETS_H_

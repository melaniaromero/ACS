#include <arpa/inet.h>  // Librería para trabajar con sockets y direcciones de red.
#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     // Librería para manipulación de cadenas 
#include <unistd.h>     // Librería para funciones POSIX como close().

/**
 * Muestra el mensaje de uso del programa y termina la ejecución.
 */
void showHelp(const char* programName) {
    printf(
        "\n %s <USUARIO>/<IP>:<PUERTO>\n\n" // Instrucciones de uso.
        "\n  <USUARIO> Usuario del servidor.\n"
        " <IP> IP del servidor.\n  <PUERTO> Puerto del servidor.\n"
        "Para salir, ingrese 'exit'.\n", programName); 
    exit(EXIT_SUCCESS); // Termina el programa correctamente.
}

/**
 *  Verifica y extrae usuario, IP y puerto del argumento proporcionado.
 * input Cadena de entrada en formato <USUARIO>/<IP>:<PUERTO>.
 *  user Buffer para almacenar el usuario.
 * ip Buffer para almacenar la dirección IP.
 * port Puntero para almacenar el puerto.
 * regresa int Devuelve 1 si el formato es válido, 0 si no lo es.
 */
int parseArguments(const char* input, char* user, char* ip, int* port) {
    return sscanf(input, "%49[^/]/%49[^:]:%d", user, ip, port) == 3; // Extrae datos y devuelve 1 si tiene éxito.
}

/**
 *  Función principal del programa.
 * argc Número de argumentos proporcionados.
 * argv Array con los argumentos (incluido el nombre del programa).
 * regesa 0 si finaliza correctamente.
 */
int main(int argc, char const* argv[]) {
    // Si no se proporcionan argumentos suficientes o se solicita ayuda, muestra la ayuda.
    if (argc < 2 || strcmp(argv[1], "-h") == 0) showHelp(argv[0]);

    // Declaración de variables.
    char user[50], ip[50], buffer[20000], response[20000]; // Buffers para usuario, IP, mensajes y respuestas.
    int port, socket_fd; // Puerto y descriptor del socket.

    // Validar y parsear los argumentos del usuario.
    if (!parseArguments(argv[1], user, ip, &port)) {
        fprintf(stderr, "Formato incorrecto. Intenta con el siguiente formato:\n %s <USUARIO>/<IP>:<PUERTO>\n", argv[0]); // Mensaje de error.
        exit(EXIT_FAILURE); // Termina el programa con error.
    }

    // Configuración inicial del socket y la dirección del servidor.
    struct sockaddr_in server_address = {
        .sin_family = AF_INET,       // Familia de direcciones: IPv4.
        .sin_port = htons(port)      // Conversión del puerto a formato de red.
    };

    // Creación del socket.
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket"); // Mensaje de error si falla.
        exit(EXIT_FAILURE);
    }

    // Conversión de la IP a formato binario.
    if (inet_pton(AF_INET, ip, &server_address.sin_addr) <= 0) {
        perror("Dirección IP inválida"); // Mensaje de error si falla.
        close(socket_fd); // Cierra el socket antes de salir.
        exit(EXIT_FAILURE);
    }

    // Conexión al servidor.
    if (connect(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Error al conectar al servidor"); // Mensaje de error si falla.
        close(socket_fd); // Cierra el socket antes de salir.
        exit(EXIT_FAILURE);
    }

    printf("Conectado a %s/%s:%d\n", user, ip, port); // Mensaje confirmando la conexión.

    // Bucle principal para enviar y recibir mensajes.
    while (1) {
        // Mostrar el prompt con información del usuario, IP y puerto.
        printf("%s/%s:%d> ", user, ip, port);
        
        // Leer entrada del usuario.
        if (!fgets(buffer, sizeof(buffer), stdin)) break; // Si ocurre un error, salir del bucle.

        buffer[strcspn(buffer, "\n")] = 0; // Eliminar el salto de línea al final del comando.
        if (strcmp(buffer, "exit") == 0) break; // Salir si el usuario escribe "exit".

        // Preparar el mensaje para enviar al servidor.
        snprintf(response, sizeof(response), "%s: %s", user, buffer);

        // Enviar el mensaje al servidor.
        if (send(socket_fd, response, strlen(response), 0) < 0) {
            perror("Error al enviar el mensaje"); // Mensaje de error si falla.
            break; // Salir del bucle.
        }

        // Recibir respuesta del servidor.
        ssize_t bytes_received = recv(socket_fd, response, sizeof(response) - 1, 0);
        if (bytes_received <= 0) break; // Salir si no se reciben datos.

        response[bytes_received] = '\0'; // Terminar la cadena con '\0'.
        printf("%s\n", response); // Mostrar la respuesta recibida.
    }

    // Cierre del socket y mensaje de finalización.
    close(socket_fd); // Cierra el socket.
    printf("Conexión cerrada.\n"); // Mensaje al usuario.
    return 0; // Salida exitosa.
}

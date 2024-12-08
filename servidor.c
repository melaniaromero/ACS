#include <arpa/inet.h>   // Biblioteca para funciones relacionadas con sockets
#include <ctype.h>       // Biblioteca para verificar caracteres
#include <stdio.h>      
#include <stdlib.h>    
#include <string.h>      // Biblioteca para manipulación de cadenas
#include <sys/wait.h>    // Biblioteca para manejo de procesos y espera de procesos hijos.
#include <unistd.h>      // Biblioteca para llamadas al sistema como fork(), pipe()

#define BUFFER_SIZE 30000 // Tamaño máximo del buffer para mensajes.

///  Muestra un mensaje de ayuda sobre cómo ejecutar el programa.
void showHelp(const char* programName) {
    printf(
        "Arquitectura Cliente-Servidor\n\n"
        "Para iniciar:\n"
        "  %s              Inicia el servidor en el puerto 8080 en localhost (127.0.0.1).\n"
        "  %s <PUERTO>     Inicia el servidor en el puerto especificado.\n"
        "  %s -h           Muestra este mensaje de ayuda.\n\n",
        programName, programName, programName);
    exit(EXIT_SUCCESS); // Termina la ejecución después de mostrar la ayuda.
}

/// Verifica si una cadena representa un número válido.
int verifyNum(const char* numero) {
    for (int i = (numero[0] == '-') ? 1 : 0; numero[i] != '\0'; i++) { // Revisa cada carácter.
        if (!isdigit(numero[i])) // Si no es un dígito, regresa 0.
            return 0;
    }
    return 1; // regresa 1 si todos los caracteres son números.
}

/// Configura el servidor y devuelve el descriptor de socket.
int setupServer(int port) {
    int servidor_fd; // Descriptor de archivo para el servidor.
    struct sockaddr_in address = { // Configura la dirección del servidor.
        .sin_family = AF_INET,         // IPv4.
        .sin_addr.s_addr = INADDR_ANY, // Acepta conexiones de cualquier IP.
        .sin_port = htons(port)        // Convierte el puerto al formato de red.
    };

    // Crea el socket.
    if ((servidor_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("No pude crear el socket, intenta de nuevo");
        exit(EXIT_FAILURE); // Termina si hay error al crear el socket.
    }

    // Asocia el socket a la dirección y puerto configurados.
    if (bind(servidor_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("No te pude enlazar, intenta de nuevo");
        exit(EXIT_FAILURE); // Termina si no se puede enlazar.
    }

    // Configura el socket para escuchar conexiones entrantes.
    if (listen(servidor_fd, 3) < 0) {
        perror("No escucho, intenta de nuevo");
        exit(EXIT_FAILURE); // Termina si no se puede poner en modo de escucha.
    }

    printf("Servidor en el port %d\n", port); // Exito
    return servidor_fd; // Regresa el descriptor del socket del servidor.
}

/// Procesa comandos recibidos del cliente.
void processCommand(int socket_fd, const char* user, const char* command) {
    if (strcmp(command, "exit") == 0) { // Si el comando es "exit", termina la conexión.
        printf("%s ha salido \n", user);
        exit(EXIT_SUCCESS);
    }

    printf("%s ha ejecutado el comando %s\n", user, command); // Muestra el comando recibido.

    int pipefd[2]; // Pipe para capturar la salida del comando.
    if (pipe(pipefd) == -1) { // Crea un pipe.
        perror("No se pudo crear el pipe");
        return;
    }

    pid_t pid = fork(); // Crea un nuevo proceso.
    if (pid == -1) { // Si falla el fork.
        perror("No pude ejecutar la función fork()");
        return;
    }

    if (pid == 0) { // Proceso hijo.
        close(pipefd[0]); // Cierra el extremo de lectura del pipe.
        dup2(pipefd[1], STDOUT_FILENO); // Redirige la salida estándar al pipe.
        dup2(pipefd[1], STDERR_FILENO); // Redirige la salida de error al pipe.
        close(pipefd[1]); // Cierra el extremo de escritura después de redirigir.

        execlp("sh", "sh", "-c", command, NULL); // Ejecuta el comando.
        perror("Error al ejecutar el comando"); // Solo llega aquí si execlp falla.
        exit(EXIT_FAILURE);
    } else { // Proceso padre.
        close(pipefd[1]); // Cierra el extremo de escritura del pipe.
        char respuesta[BUFFER_SIZE];
        ssize_t numbytes;

        // Lee la salida del comando desde el pipe y la envía al cliente.
        while ((numbytes = read(pipefd[0], respuesta, sizeof(respuesta) - 1)) > 0) {
            respuesta[numbytes] = '\0'; // Asegura que la cadena esté terminada.
            if (send(socket_fd, respuesta, numbytes, 0) == -1) { // Envía datos.
                perror("Error al enviar la respuesta");
                break;
            }
        }

        close(pipefd[0]); // Cierra el extremo de lectura.
        waitpid(pid, NULL, 0); // Espera a que el proceso hijo termine.
    }
}

/// Maneja la conexión con el cliente.
void handleClient(int cliente_fd) {
    char buffer[BUFFER_SIZE]; // Buffer para recibir datos.
    while (1) {
        memset(buffer, 0, sizeof(buffer)); // Limpia el buffer.
        ssize_t valread = recv(cliente_fd, buffer, sizeof(buffer) - 1, 0); // Lee datos.
        if (valread <= 0) { // Si no se reciben datos, termina.
            perror("Error al recibir datos del cliente");
            break;
        }

        buffer[valread] = '\0'; // Asegura que la cadena esté terminada.
        char user[50], command[BUFFER_SIZE];
        if (sscanf(buffer, "%49[^:]: %[^\n]", user, command) != 2) { // Divide en usuario y comando.
            printf("Cadena inválida recibida: %s", buffer);
            continue;
        }

        processCommand(cliente_fd, user, command); // Procesa el comando.
    }
    close(cliente_fd); // Cierra la conexión con el cliente.
}

int main(int argc, char* argv[]) {
    int port = 8080; // Puerto por defecto.

    // Procesa los argumentos del programa.
    if (argc >= 2) {
        if (strcmp(argv[1], "-h") == 0) { // Si se pasa "-h", muestra la ayuda.
            showHelp(argv[0]);
        } else if (verifyNum(argv[1])) { // Si es un número, lo usa como puerto.
            port = atoi(argv[1]);
        } else { // Si no es válido ese numero, muestra un error.
            fprintf(stderr, "El puerto '%s' no es válido.\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    int servidor_fd = setupServer(port); // Configura el servidor.
    struct sockaddr client_address; // Información del cliente.
    socklen_t addrlen = sizeof(client_address);

    while (1) { // Bucle principal para aceptar conexiones.
        int cliente_fd = accept(servidor_fd, (struct sockaddr*)&client_address, &addrlen); // Acepta una conexión.
        if (cliente_fd < 0) {
            perror("Error al aceptar la conexión");
            continue;
        }

        if (!fork()) { // Crea un proceso hijo para manejar al cliente.
            close(servidor_fd); // El hijo no necesita el socket del servidor.
            handleClient(cliente_fd); // Maneja la conexión del cliente.
            exit(EXIT_SUCCESS); // Termina el proceso hijo.
        }
        close(cliente_fd); // El proceso padre cierra el socket del cliente.
    }

    close(servidor_fd); // Cierra el socket del servidor al salir. 
    return 0;
}

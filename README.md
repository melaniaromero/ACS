## Proyecto Final Arquitectura Cliente-Servidor 


## Ejecución del proyecto
Paso 1
```bash
gcc -o servidor servidor.c
gcc -o cliente cliente.c
```

Paso 2

```bash
./servidor 8080
```

Paso 3

```bash
./client <USUARIO>/<IP>:<PUERTO>
./cliente localhost/127.0.0.1:8080

```

 `<USUARIO>` es el nombre de usuario del servidor
 `<IP>` es la dirección IP del servidor
 `<PUERTO>` es el puerto en el que el servidor está escuchando.

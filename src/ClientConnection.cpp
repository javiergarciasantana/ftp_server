//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//              This class processes an FTP transaction.
// 
//****************************************************************************



#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

#include "common.h"

#include "ClientConnection.h"




ClientConnection::ClientConnection(int s) {
    int sock = (int)(s);
  
    char buffer[MAX_BUFF];

    control_socket = s;
    // Check the Linux man pages to know what fdopen does.
    fd = fdopen(s, "a+");
    if (fd == NULL){
	std::cout << "Connection closed" << std::endl;

	fclose(fd);
	close(control_socket);
	ok = false;
	return ;
    }
    
    ok = true;
    data_socket = -1;
    parar = false;
   
  
  
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
  
}


int connect_TCP( uint32_t address,  uint16_t  port) {//implemented by student
  // Create socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Error creating socket");
    return -1;
  }

  // Define server address
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(address);
  serv_addr.sin_port = htons(port);

  // Connect to server
  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("Error connecting to server");
    close(sockfd);
    return -1;
  }

  return sockfd;
}






void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;
  
}





    
#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
  bool modo_pasivo;
    if (!ok) {
	 return;
    }
    
    fprintf(fd, "220 Service ready\n");
  
    while(!parar) {

      fscanf(fd, "%s", command);
      if (COMMAND("USER")) {
	    fscanf(fd, "%s", arg);
	    fprintf(fd, "331 User name ok, need password\n");
      }
      else if (COMMAND("PWD")) {
	   
      }
      else if (COMMAND("PASS")) {
        fscanf(fd, "%s", arg);
        if(strcmp(arg,"1234") == 0){
            fprintf(fd, "230 User logged in\n");
        }
        else {
            fprintf(fd, "530 Not logged in.\n");
            parar = true;
        }
	   
      }
      else if (COMMAND("PORT")) {
        int a[6];
        fscanf(fd, "%d,%d,%d,%d,%d,%d", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5]);
        char ip_address[16];
        sprintf(ip_address, "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
        int port = a[4] * 256 + a[5]; //bits shift

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_aton(ip_address, &addr.sin_addr);
        data_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(data_socket, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
            fprintf(fd, "425 Can't open data connection.\n");
            close(data_socket);
            data_socket = -1;
        } else {
            fprintf(fd, "200 Port command successful.\n");
        }
      }
      else if (COMMAND("PASV")) {
        std::cout << "hola" << std::endl;
        // generate a random port number for data transfer
      
        // set up a passive data connection listener on the server
        struct sockaddr_in data_server_addr;
        memset(&data_server_addr, 0, sizeof(data_server_addr));
        data_server_addr.sin_family = AF_INET;
        data_server_addr.sin_addr.s_addr = INADDR_ANY;
        data_server_addr.sin_port = 0;

        int data_server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (data_server_socket < 0) {
            fprintf(fd, "425 Can't open data connection.\n");
            return;
        }

        if (bind(data_server_socket, (struct sockaddr *)&data_server_addr, sizeof(data_server_addr)) < 0) {
            fprintf(fd, "425 Can't open data connection.\n");
            return;
        }

        if (listen(data_server_socket, 1) < 0) {
            fprintf(fd, "425 Can't open data connection.\n");
            return;
        }

        // get the server's IP address
        struct sockaddr_in server_addr;
        socklen_t server_addr_len = sizeof(server_addr);
        getsockname(data_server_socket, (struct sockaddr *)&server_addr, &server_addr_len);
        char *server_ip = inet_ntoa(server_addr.sin_addr);
        uint16_t port = server_addr.sin_port;
        int p1 = port >> 8;
        int p2 = port & 0xff;

        // send the response to the client with the address and port to connect to
        fprintf(fd, "227 Entering Passive Mode (127,0,0,1,%d,%d).\n", p2, p1);
        fflush(fd);

        // wait for the client to connect
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        data_socket = accept(data_server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (data_socket < 0) {
            fprintf(fd, "425 Can't open data connection.\n");
            return;
        }

        // close the listener socket
        //close(data_server_socket);
      }
      else if (COMMAND("STOR") ) {
        fscanf(fd, "%s", arg); // Leer el nombre del archivo del cliente
        FILE *file = fopen(arg, "wb"); // Abrir el archivo en modo de escritura binaria
        if (file == NULL) {
          fprintf(fd, "550 Failed to open file.\n"); // Si no se puede abrir el archivo, enviar un mensaje de error al cliente
          continue;
        }
        fprintf(fd, "150 File status okay; about to open data connection.\n"); // Enviar mensaje de éxito al cliente
        fflush(fd);
        int data_fd = data_socket; // Esperar a que el cliente se conecte al socket de datos
        char buffer[1024];
        int n;
        while ((n = recv(data_fd, buffer, sizeof(buffer), 0)) > 0) {
          fwrite(buffer, sizeof(char), n, file); // Leer los datos del archivo del socket de datos y escribirlos en el archivo en el servidor
        }
        fclose(file); // Cerrar el archivo
        fprintf(fd, "226 Transfer complete.\n"); // Enviar mensaje de éxito al cliente
        close(data_fd); // Cerrar el socket de datos
      }
      else if (COMMAND("RETR")) {
        fscanf(fd, "%s", arg); // Read the name of the file from the client
        FILE *file = fopen(arg, "rb"); // Open the file in binary read mode
        if (file == NULL) {
            fprintf(fd, "550 Failed to open file.\n"); // If the file cannot be opened, send an error message to the client
            continue;
        }
        fprintf(fd, "150 File status okay; about to open data connection.\n"); // Send success message to the client
        fflush(fd);
        int data_fd = data_socket; // Wait for the client to connect to the data socket
        char buffer[1024];
        int n;
        while ((n = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
            send(data_fd, buffer, n, 0); // Read the data from the file and write it to the data socket to be sent to the client
        }
        fclose(file); // Close the file
        fprintf(fd, "226 Transfer complete.\n"); // Send success message to the client
        close(data_fd); // Close the data socket

      }
      else if (COMMAND("LIST")) {
        fprintf(fd, "150 Opening data connection for directory list.\n");
        fflush(fd);
        int data_fd = data_socket;
        char buffer[1024];
        int n;

        DIR *dir = opendir(".");
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            snprintf(buffer, sizeof(buffer), "%s\n", entry->d_name);
            send(data_fd, buffer, strlen(buffer), 0);
        }
        closedir(dir);

        fprintf(fd, "226 Directory send OK.\n");
        close(data_fd);
      }
      else if (COMMAND("SYST")) {
           fprintf(fd, "215 UNIX Type: L8.\n");   
      }

      else if (COMMAND("TYPE")) {
        fscanf(fd, "%s", arg);
        fprintf(fd, "200 OK\n");   
      }
     
      else if (COMMAND("QUIT")) {
        fprintf(fd, "221 Service closing control connection. Logged out if appropriate.\n");
        close(data_socket);	
        parar=true;
        break;
      }
  
      else  {
	    fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	    printf("Comando : %s %s\n", command, arg);
	    printf("Error interno del servidor\n");
	
      }
      
    }
    
    fclose(fd);

    
    return;
  
};

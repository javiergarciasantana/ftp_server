//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//                        Main class of the FTP server
// 
//****************************************************************************

#include <iostream>
#include <cerrno>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

 #include <unistd.h>


#include <pthread.h>

#include <list>

#include "common.h"
#include "FTPServer.h"
#include "ClientConnection.h"


int define_socket_TCP(int port) { //implementado por alumnos
  // Creating the socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
      std::cerr << "Error al crear el socket" << std::endl;
      return -1;
  }

  // Configuring the server address
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  // Linking the socket to the server address
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      std::cerr << "Error al vincular el socket a la dirección del servidor" << std::endl;
      close(sockfd);
      return -1;
  }

  // Looking for or listening to incoming connections
  if (listen(sockfd, 5) < 0) {
    //error case
      std::cerr << "Error al escuchar las conexiones entrantes" << std::endl;
      close(sockfd);
      return -1;
  }
    //succes case
  std::cout << "Socket TCP en el puerto " << port << " creado correctamente" << std::endl;

  return sockfd; //returns socket descriptor(socket identifier in the operating system)
}

// This function is executed when the thread is executed.
void* run_client_connection(void *c) {
    ClientConnection *connection = (ClientConnection *)c;
    connection->WaitForRequests();
  
    return NULL;
}

FTPServer::FTPServer(int port) {
    this->port = port;
  
}

// Parada del servidor.
void FTPServer::stop() {
    close(msock);
    shutdown(msock, SHUT_RDWR);

}

// Starting of the server
void FTPServer::run() {

    struct sockaddr_in fsin;
    int ssock;
    socklen_t alen = sizeof(fsin);
    msock = define_socket_TCP(port);  // This function must be implemented by you.
    while (1) {
	    pthread_t thread;
        ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
        if(ssock < 0) {
            errexit("Fallo en el accept: %s\n", strerror(errno));
        }
    
        ClientConnection *connection = new ClientConnection(ssock);
        
        // Here a thread is created in order to process multiple
        // requests simultaneously
        pthread_create(&thread, NULL, run_client_connection, (void*)connection);
       
    }
}

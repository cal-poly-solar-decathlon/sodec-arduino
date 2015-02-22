/******************************************************************************
 * tcp_server.c
 *
 * CPE 464 - Program 1
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "networks.h"
#include "ClientTable.h"


int main(int argc, char *argv[]) {
   int server_socket= 0;           //socket descriptor for the server socket
   fd_set socketSet;               //set of sockets to watch
   uint32_t seqNum = 0;            //sequence number of packet sent from server
   int maxSocket = 0;
   ClientTable table;
   
   table.count = 0;
   table.first = NULL;
   
   //create the server socket
   maxSocket = server_socket = tcp_recv_setup();
   
   //look for a client to serve
   if (listen(server_socket, 10) < 0) {
      perror("listen call");
      exit(-1);
   }
      
   while (1) {
      tcp_server_Select(server_socket, &socketSet, &maxSocket, &seqNum, &table);
   }
   
   close(server_socket);
   
   return 0;
}

int tcp_recv_setup() {
   int server_socket= 0;
   struct sockaddr_in local;      /* socket address for local side  */
   socklen_t len= sizeof(local);  /* length of local address        */
   char hostname[1024];
   
   hostname[1023] = '\0';
   gethostname(hostname, 1023);
   
   /* create the socket  */
   server_socket= socket(AF_INET, SOCK_STREAM, 0);
   if(server_socket < 0) {
      perror("socket call");
      exit(1);
   }
   
   local.sin_family= AF_INET;         //internet family
   local.sin_addr.s_addr= INADDR_ANY; //wild card machine address
   local.sin_port= htons(63314);                 //let system choose the port
   
   /* bind the name (address) to a port */
   if (bind(server_socket, (struct sockaddr *) &local, sizeof(local)) < 0) {
      perror("bind call");
      local.sin_port= htons(0);                 //let system choose the port
      if (bind(server_socket, (struct sockaddr *) &local, sizeof(local)) < 0) {
         perror("bind call");
         exit(-1);
      }
   }
   
   //get the port name and print it out
   if (getsockname(server_socket, (struct sockaddr*)&local, &len) < 0) {
      perror("getsockname call");
      exit(-1);
   }
   
   printf("Server is using port %d \n", ntohs(local.sin_port));
//   printf("Host: %s\n", hostname);
//   printf("IP: %s\n", inet_ntoa(local.sin_addr));
   
   return server_socket;
}

void tcp_server_Select(int server_socket, fd_set *socketSet, int *maxSocket,
               uint32_t *seqNum, ClientTable *table) {
   struct timeval timeout;
   int currentSocket = 0;
   char handle[MAX_HANDLE_SIZE];
   int addSocket = 0;
   
   timeout.tv_sec = 1;
   timeout.tv_usec = HALF_SEC;
   FD_ZERO(socketSet);
   FD_SET(server_socket, socketSet);
   for (currentSocket = 1; currentSocket <= table->count; ++currentSocket) {
      ClientTable_getHandleSocket(table, currentSocket, handle, &addSocket);
      FD_SET(addSocket, socketSet);
   }
   
   if ((select(*maxSocket + 1, socketSet, (fd_set *)0, (fd_set *)0, &timeout)
       ) < 0) {
      perror("select call");
      exit(-1);
   }
   
   for (currentSocket = 0; currentSocket <= *maxSocket; ++currentSocket) { 
      if (FD_ISSET(currentSocket, socketSet)) {// || !recv(currentSocket, handle,
//          MAX_HANDLE_SIZE, MSG_PEEK | MSG_DONTWAIT)) {
         tcp_ProcessSocket(currentSocket, server_socket, maxSocket, socketSet,
                            table, seqNum);
      }
   }
}

void tcp_ProcessSocket(int socket, int serverSocket, int *maxSocket, fd_set
                        *socketSet, ClientTable *table, uint32_t *seqNum) {
   int packetSize = 0,
       clientSocket = 0;
   char packet[MAX_MSG_SIZE];
   ClientEntry *newClient;
   
   if (socket == serverSocket) {          //new client connection
      if ((clientSocket = accept(socket, (struct sockaddr*)0,
                                  (socklen_t *)0)) < 0) {
         perror("accept call");
         exit(-1);
      }
      else {
         if (clientSocket > *maxSocket)
            *maxSocket = clientSocket;
         FD_SET(clientSocket, socketSet);
      }
      
      newClient = malloc(sizeof(ClientEntry));
      newClient->handle[0] = '\0';              //null handle name
      newClient->next = NULL;
      newClient->socket = clientSocket;
      ClientTable_Insert(table, newClient);
      
      printf("Client %d connected.\n", clientSocket);
   }
   
   else if ((packetSize = (int)recv(socket, packet, MAX_MSG_SIZE, 0)) < 0) {
      perror("Notice");
//      exit(-1);
   }
   else if (!packetSize) {                  //client died
      ClientTable_Remove(table, socket);
      close(socket);
      printf("Client %d disconnected.\n", socket);
   }
   else {
      packet[packetSize] = '\0';
      printf("%s", packet);
      fflush(stdout);
   }
}

void tcp_RespondClient(int socket, uint8_t flag, uint32_t *seqNum,
                        uint32_t handleNum, char *badHdl, ClientTable *table) {
   char packet[MAX_MSG_SIZE];
   uint32_t packetSize = MAX_MSG_SIZE;
   
   memcpy(packet, &flag, 1);
   
   if (flag == 7) {           //Destination handle not found
      ;
   }
   else if (flag == 11) {     //Return number of client handles
      ;
   }
   else if (flag == 13) {     //Send client handle by number
      ;
   }

   if (send(socket, packet, packetSize, 0) < 0) {
      perror("send call");
      exit(-1);
   }
}


/******************************************************************************
 * tcp_client.c
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
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
#include <netdb.h>

#include "networks.h"

int main(int argc, char * argv[])
{
   int serverSocket= 0;   //socket descriptor for the server socket
   fd_set socketSet;
   char exitFlag = 0,
        promptFlag = 1;
   uint32_t seqNum = 1;    //sequence number of packet sent from client

   
   /* check command line arguments  */
   if(argc!= 4)
   {
      printf("usage: %s host-name port-number \n", argv[0]);
      exit(-1);
   }
   
   if (strlen(argv[1]) >= MAX_HANDLE_SIZE) {
      printf("Length of handle (%d) exceeds maximum (%d).\n", strlen(argv[1]),
             MAX_HANDLE_SIZE);
   }
   
   /* set up the socket for TCP transmission  */
   serverSocket= tcp_send_setup(argv[1], argv[2], argv[3], &exitFlag);
   
   while (!exitFlag) {
      if (promptFlag) {
         printf("> ");
         fflush(stdout);
         promptFlag = 0;
      }
      promptFlag = tcp_client_Select(serverSocket, &socketSet, &seqNum,
                                     &exitFlag, argv[1]);
   }
   
   close(serverSocket);
   
   return 0;
}

void tcp_client_InitialPackets(int socket_num, char *handle, char *exitFlag) {
   char header[INITIAL_PACKET_SIZE];
   uint8_t byte = 1;
   uint32_t initialPacketNum = htonl(1);  //seq number = 1
   
   memcpy(header, &initialPacketNum, 4);
   memcpy(header + SEQNUM_SIZE, &byte, 1);
   byte = strlen(handle);
   memcpy(header + NORM_HDR_SIZE, &byte, 1);
   memcpy(header + NORM_HDR_SIZE + 1, handle, byte);
   byte = 2;                              //good handle flag
   
   if (send(socket_num, header, NORM_HDR_SIZE + 1 + strlen(handle), 0) < 0) {
      perror("send call");
      exit(-1);
   }
   if (recv(socket_num, header, NORM_HDR_SIZE, 0) < 0) {
      perror("recv call");
      exit(-1);
   }
   else if (memcmp(header + SEQNUM_SIZE, &byte, 1)) {
      fprintf(stderr, "Error, handle: %s is already taken.\n", handle);
      *exitFlag = 1;
   }
}

int tcp_send_setup(char *handle, char *host_name, char *port, char *exitFlag) {
   int socket_num;
   struct sockaddr_in remote;       // socket address for remote side
   struct hostent *hp;              // address of remote host
   
   
   // create the socket
   if ((socket_num = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
      perror("socket call");
      exit(-1);
	}
   
   // designate the addressing family
   remote.sin_family= AF_INET;
   
   // get the address of the remote host and store
   if ((hp = gethostbyname(host_name)) == NULL)
	{
      printf("Error getting hostname: %s\n", host_name);
      exit(-1);
	}
   
   memcpy((char*)&remote.sin_addr, (char*)hp->h_addr, hp->h_length);
   
   // get the port used on the remote side and store
   remote.sin_port= htons(atoi(port));
   
   if(connect(socket_num, (struct sockaddr*)&remote, sizeof(struct sockaddr_in))
      < 0) {
      perror("connect call");
      exit(-1);
   }
   
   tcp_client_InitialPackets(socket_num, handle, exitFlag);

   return socket_num;
}

char tcp_client_Select(int serverSocket, fd_set *socketSet, uint32_t *seqNum,
                       char *exitFlag, char *thisHandle) {
   char selectFlag = 0;
   struct timeval timeout;
   
   timeout.tv_sec = 1;
   timeout.tv_usec = HALF_SEC;
   FD_ZERO(socketSet);
   FD_SET(serverSocket, socketSet);
   FD_SET(0, socketSet);
   
   if ((select(serverSocket + 1, socketSet, (fd_set *)0, (fd_set *)0, &timeout)
        ) < 0) {
      perror("select call");
      exit(-1);
   }
   
   if (FD_ISSET(serverSocket, socketSet)) {
      selectFlag = tcp_client_ProcessSocket(serverSocket, seqNum, exitFlag);
   }
   else if (FD_ISSET(0, socketSet)) {
      selectFlag = tcp_ProcessInput(seqNum, thisHandle, serverSocket);
   }
   
   return selectFlag;
}

char tcp_client_ProcessSocket(int socket, uint32_t *seqNum, char *exitFlag) {
   char promptAgain = 0;
   char packet[MESSAGE_PACKET_SIZE];
   char *pktPtr = packet + SEQNUM_SIZE;
   int packetSize;
   uint8_t flag = 0;
   uint8_t handleSize = 0;
   char handle[MAX_HANDLE_SIZE];

   if ((packetSize = (int)recv(socket, packet, MESSAGE_PACKET_SIZE, 0)) < 0) {
      perror("recv call");
      exit(-1);
   }
   else if (packetSize) {                             //not a closed socket
      memcpy(&flag, pktPtr++, 1);                     //get flag
      if (flag == 9)                                  //Client allowed to exit
         *exitFlag = 1;
      else if (flag == 11 || flag == 13) {
         promptAgain = tcp_client_ListHandles(socket, seqNum, flag, packet);
      }
      else if (flag == 7) {                           //Error: No such handle
         tcp_GetHandle(handle, pktPtr);               //get dest handle
         printf("Client with handle %s does not exist.\n", handle);
         promptAgain = 1;
      }
      else if (flag == 5 || flag == 6) {              //being sent a message
         if (flag == 6) {
            handleSize = tcp_GetHandle(handle, pktPtr); //get dest handle
            pktPtr += 1 + handleSize;                 //skip destination handle 
         }
         handleSize = tcp_GetHandle(handle, pktPtr);  //get source handle
         printf("\n%s: %s", handle, pktPtr + 1 + handleSize);
         promptAgain = 1;
      }
   }
   else {
      fprintf(stderr, "Server socket closed.\n");
      *exitFlag = 1;
   }
   
   return promptAgain;
}

int tcp_GetHandle(char *handle, char *packet) {
   int handleSize = 0;
   
   memcpy(&handleSize, packet, 1);              //get handle size
   memcpy(handle, packet + 1, handleSize);      //get handle
   handle[handleSize] = '\0';                   //append null

   
   return handleSize;
}

char tcp_ProcessInput(uint32_t *seqNum, char *thisHandle, int serverSocket) {
   char command[3], destHandle[MSG_MAX], message[MSG_MAX + 1], promptAgain = 1;
   int sendSize = 0;
   uint8_t flag = 0;
   
   scanf("%2s", command);                    //get command
   
   if (command[0] == '%') {
      if (toupper(command[1]) == 'M')
         scanf("%100s", destHandle);         //get dest client handle
      if (toupper(command[1]) == 'M' || toupper(command[1]) == 'B') {
         //read message
         while ((message[sendSize] = getchar()) != '\n' && sendSize < MSG_MAX)
            ++sendSize;
         memmove(message, message + 1, sendSize);
         if (message[0] != '\n')
            message[sendSize] = '\0';
         else
            message[sendSize + 1] = '\0';
         
         if (toupper(command[1]) == 'M') {   //client-to-client message
            flag = 6;
         }
         else {                              //broadcast message
            flag = 5;
         }
         tcp_SendMessage(serverSocket, seqNum, flag, destHandle,
                         strlen(destHandle), thisHandle, strlen(thisHandle),
                         message);
      }            
      else if (toupper(command[1]) == 'L') {
         tcp_client_ListHandles(serverSocket, seqNum, 10, NULL);
         promptAgain = 0;
      }            
      else if (toupper(command[1]) == 'E') {
         tcp_client_Exit(serverSocket, seqNum);
         promptAgain = 0;
      }
      else
         printf("Commands: %%M, %%B, %%L, %%E\n");
   }
   else
      printf("Commands: %%M, %%B, %%L, %%E\n");
   
   return promptAgain;
}

void tcp_SendMessage(int serverSocket, uint32_t *seqNum, int flag, char
                     *destHdl, long h_dest_size, char *thisHdl,
                     long h_this_size, char *message) {
   char packet[MESSAGE_PACKET_SIZE];
   char *pktPtr = packet + NORM_HDR_SIZE;
   uint32_t sequence = htonl(++*seqNum);
   
   memcpy(packet, &sequence, SEQNUM_SIZE);
   memcpy(packet + SEQNUM_SIZE, &flag, 1);
   if (flag == 6) {
      memcpy(packet + NORM_HDR_SIZE, &h_dest_size, 1);
      memcpy(packet + NORM_HDR_SIZE + 1, destHdl, h_dest_size);
      pktPtr += h_dest_size + 1;
   }
   memcpy(pktPtr, &h_this_size, 1);
   memcpy(pktPtr + 1, thisHdl, h_this_size);
   memcpy(pktPtr + 1 + h_this_size, message, strlen(message) + 1);
      
   if (send(serverSocket, packet, NORM_HDR_SIZE + 3 + h_this_size + h_dest_size
            + strlen(message), 0) < 0) {
      perror("send call");
      exit(-1);
   }
}

char tcp_client_ListHandles(int serverSocket, uint32_t *seqNum, uint8_t stage,
                            char *packet) {
   char sendPacket[INITIAL_PACKET_SIZE], handle[MAX_HANDLE_SIZE], done = 0;
   uint32_t sequence = 0, nHandleNum = 0;
   static uint32_t clients = 0;
   static uint32_t handleNum = 0;
   int sendSize = NORM_HDR_SIZE;
   
   if (stage == 10 || stage == 12) {
      sequence = htonl(++*seqNum);
      memcpy(sendPacket, &sequence, SEQNUM_SIZE);
      memcpy(sendPacket + SEQNUM_SIZE, &stage, 1);
      if (stage == 12) {
         if (++handleNum <= clients) {
            nHandleNum = htonl(handleNum);
            memcpy(sendPacket + NORM_HDR_SIZE, &nHandleNum, SEQNUM_SIZE);
         }
         else {
            handleNum = 0;
            done = 1;
         }
         sendSize += SEQNUM_SIZE;
      }
      if (!done)
         if (send(serverSocket, sendPacket, sendSize, 0) < 0) {
            perror("send call");
            exit(-1);
         }
   }
   else if (stage == 11) {
      memcpy(&clients, packet + NORM_HDR_SIZE, SEQNUM_SIZE);
      clients = ntohl(clients);
      tcp_client_ListHandles(serverSocket, seqNum, 12, packet);
   }
   else if (stage == 13) {
      tcp_GetHandle(handle, packet + NORM_HDR_SIZE);
      printf("%s\n", handle);
      done = tcp_client_ListHandles(serverSocket, seqNum, 12, packet);
   }
   
   return done;
}

void tcp_client_Exit(int serverSocket, uint32_t *seqNum) {
   char packet[NORM_HDR_SIZE];
   uint32_t sequence = htonl(*++seqNum);
   uint8_t flag = 8;
   
   memcpy(packet, &sequence, SEQNUM_SIZE);
   memcpy(packet + SEQNUM_SIZE, &flag, 1);
   
   if (send(serverSocket, packet, NORM_HDR_SIZE, 0) < 0) {
      perror("send call");
      exit(-1);
   }
}



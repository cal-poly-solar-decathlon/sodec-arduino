#ifndef ChatTCP_Networks_h
#define ChatTCP_Networks_h

#include <stdlib.h>
#include <string.h>
#include "ClientTable.h"

#define MSG_MAX 1000
#define HALF_SEC 500000
//#define NORM_HDR_SIZE 5
//#define SEQNUM_SIZE 4
//#define INITIAL_PACKET_SIZE 106
#define MAX_MSG_SIZE 1001
//1000B message, 2 100B handles, 2 1B handle length. 5B normal header

//--------------- For the server side ----------------------
/*
 Waits for initial packet from a new client establishing commuication.
 Sends back an ACK so the client knows communication is good
 */
//void tcp_server_InitialPackets(int socket_num, uint32_t *seqNum,
//                               ClientTable *table, char *packet) ;

/*
 This function sets the server socket.  It lets the system
 determine the port number.  The function returns the server
 socket number and prints the port number to the screen. It
 then calls tcp_server_initial_packets()
 */
int tcp_recv_setup();

/*
 This function waits for a client to ask for services.  It returns
 the socket number to service the client on.
 */
void tcp_server_Select(int server_socket, fd_set *socketSet, int *maxSocket,
                      uint32_t *seqNum, ClientTable *table);

/*
 
 */
void tcp_ProcessSocket(int socket, int serverSocket, int *maxSocket, fd_set
                        *socketSet, ClientTable *table, uint32_t *seqNum);

/*
 
 */
void tcp_RespondClient(int socket, uint8_t flag, uint32_t *seqNum,
                        uint32_t handleNum, char *badHdl, ClientTable *table);

//--------------- For the client side ----------------------

/*
 Sends initial packet to server establishing commuication.
 Waits for an ACK back from the server.
 */
void tcp_client_InitialPackets(int socket_num, char *handle, char *exitFlag);

/*
 Sets up socket to communicate with server. After a successful connect(),
 this function calls tcp_client_initial_packets()
 */
int tcp_send_setup(char *handle, char *host_name, char *port, char *exitFlag);

/*
 
 */
char tcp_client_Select(int serverSocket, fd_set *socketSet, uint32_t *seqNum,
                       char *exitFlag, char *thisHandle);

/*
 
 */
char tcp_client_ProcessSocket(int socket, uint32_t *seqNum, char *exitFlag);

/*
 
 */
int tcp_GetHandle(char *handle, char *packet);


/*
 
 */
char tcp_ProcessInput(uint32_t *seqNum, char *thisHandle, int serverSocket);

/*
 
 */
void tcp_SendMessage(int serverSocket, uint32_t *seqNum, int flag, char
                     *destHdl, long h_dest_size, char *thisHdl,
                     long h_this_size, char *message);

/*
 
 */
char tcp_client_ListHandles(int serverSocket, uint32_t *seqNum, uint8_t stage,
                            char *packet);

/*
 
 */
void tcp_client_Exit(int serverSocket, uint32_t *seqNum);

#endif
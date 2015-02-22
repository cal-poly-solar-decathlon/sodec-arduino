//
//  ClientTable.h
//  ChatTCP
//
//  Created by Timothy Ambrose on 1/27/15.
//  Copyright (c) 2015 Tim Ambrose. All rights reserved.
//

#ifndef ChatTCP_ClientTable_h
#define ChatTCP_ClientTable_h

#include <stdint.h>

#define MAX_HANDLE_SIZE 101
#pragma pack(1)

typedef struct ClientEntry {
   int socket;
   char handle[MAX_HANDLE_SIZE];
   struct ClientEntry *next;
} ClientEntry;

typedef struct ClientTable {
   ClientEntry *first;
   uint32_t count;
} ClientTable;

/* Remove
 
 */
void ClientTable_Remove(ClientTable *table, int socket);

/* Insert
 
 */
int ClientTable_Insert(ClientTable *table, ClientEntry *newClient);

/* getHandle
 
 */
int ClientTable_getHandleSocket(ClientTable *table, unsigned int clientNum, 
                                char *handle, int *socket);

/* getSocket
 
 */
int ClientTable_getSocket(ClientTable *table, char *handle);

#endif

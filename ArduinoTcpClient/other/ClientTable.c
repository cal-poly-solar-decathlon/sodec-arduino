//
//  ClientTable.c
//  ChatTCP
//
//  Created by Timothy Ambrose on 1/27/15.
//  Copyright (c) 2015 Tim Ambrose. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ClientTable.h"

void ClientTable_Remove(ClientTable *table, int socket) {
   ClientEntry *current = table->first,
               *remove;
//   printf("Table: %d->",table->count);
   fflush(stdout);
   if (table->count) {
      if (table->first->socket == socket) {
         remove = table->first;
         table->first = table->first->next;
         free(remove);
         --table->count;
      }
      else {
         for (; current->next && current->next->socket != socket;
              current = current->next);
         if (!current->next)
            fprintf(stderr, "Socket %d does not exit in table.\n", socket);
         else {
            remove = current->next;
            current->next = current->next->next;
            free(remove);
            --table->count;
         }
      }
   }
//   printf("%d\n",table->count);
}

int ClientTable_Insert(ClientTable *table, ClientEntry *newClient) {
   char status = 0;           //good handle
   ClientEntry *current;
      
//   printf("Table: %d->",table->count);
   if (table->first) {
      for (current = table->first; current->next && strcmp(current->handle,
           newClient->handle); current = current->next);
      if (!strcmp(current->handle, newClient->handle))
         status = 1;          //taken handle
      else
         current->next = newClient;
   }
   else
      table->first = newClient;
   
   if (!status)
      ++table->count;
   
//   printf("%d\n",table->count);
   return status;
}

int ClientTable_getHandleSocket(ClientTable *table, unsigned int clientNum,
                          char *handle, int *socket) {
   char handleSize = 0;
   ClientEntry *current = table->first;
   
   if (table->count >= clientNum && clientNum) {
      while (--clientNum)
         current = current->next;
      strcpy(handle, current->handle);
      handleSize = strlen(handle);
      *socket = current->socket;
   }
   
   return handleSize;
}

int ClientTable_getSocket(ClientTable *table, char *handle) {
   int socket = -1;
   ClientEntry *current;   
   
   for (current = table->first; current && strcmp(current->handle, handle);
        current = current->next);
   if (current)
      socket = current->socket;
   
   return socket;
}

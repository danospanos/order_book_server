/**
 * @author     Daniel Stasek
 * @date       June 24, 2016
 * @version    1.0
 */
#ifndef CLIENTDTA_H
#define CLIENTDTA_H

#include <stdlib.h>
#include <stdio.h>

#ifndef my_assert
#include <assert.h>
#define my_assert(x) assert(x)
#endif
//structs for dyanmic list to have overall look at all clients
typedef struct client{
  struct client *next;
  struct client *prev;
  int ID;
  int client_sockfd;
}client;

typedef struct clients_list{
  client *first;
  client *last;
}clients_list;
//so we can easily pass pointers to list with our clients in pthread_create()
typedef struct pointers{
  client *each_client;
  clients_list *list_of_clnts;
}pointers;
/**
 *@brief Memory allocation of struct client
 *@return pointer at allocated struct
 */
struct client *malloc_struct_client();
/**
 * @brief Set initial parameters to clients_list struct
 */
void create_queue_for_clients(clients_list *dta_pos);
/**
 * @brief Add queue at the end of it
 */
void push_queue_clients(clients_list *dta_pos, client *each_client);
/**
 * @brief delete whole queue and free() it from heap, list is set to default
 * @return number of deleted structs from list is returned
 */
unsigned int del_queue_clients(clients_list *dta_pos);
/**
 * @brief Deletes given struct (as 2nd argument) and pass the pointer as they should be in the dynamic list
 */
void del_given_struct_from_clients(clients_list *dta_pos, client *each_client);
/**
 * @brief To make main.c more readable, two fncs create_queue_for_clients combined here.
 */
 void create_queues_for_clients(clients_list *dta_pos_1, clients_list *dta_pos_2);
 /**
  * @brief Memory allocation of struct pointers
  * @return pointer at allocated struct
  */
 struct pointers *malloc_struct_pointers();

#endif //CLIENTDTA_H

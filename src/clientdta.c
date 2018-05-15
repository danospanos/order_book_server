/**
 * @author     Daniel Stasek
 * @date       June 24, 2016
 * @version    1.0
 */
#include "clientdta.h"

struct client *malloc_struct_client(){
  client *tmp = malloc(sizeof(struct client));
  if(tmp == NULL){
    fprintf(stderr, "Couldn't allocate more memory for struct public_client!\n");
    return NULL;
  }
  return tmp;
}

void create_queue_for_clients(clients_list *dta_pos){
  my_assert(dta_pos);
  dta_pos->first = NULL;
  dta_pos->last = NULL;
}

void push_queue_clients(clients_list *dta_pos, client *each_client){
  my_assert(dta_pos);
  if(each_client == NULL){
    fprintf(stderr, "Value of *each_client passed wrong.\nCheck if mallocated corectlly.\nBehavior undefined from now!\n");;
    return;
  }
  each_client->next = NULL; //setting next
  if(dta_pos->first == NULL){ //first push
    dta_pos->first = each_client;
    each_client->prev = NULL;
  }else{ //queue size > 1
    dta_pos->last->next = each_client;
    each_client->prev = dta_pos->last;
  }
  dta_pos->last = each_client; //setting last
}

unsigned int del_queue_clients(clients_list *dta_pos){
  my_assert(dta_pos);
  if(dta_pos->first == NULL){ //queue is empty, nothing to be deleted
      return 0;
  }
  unsigned int nof_del_structs = 1;
  client *tmp1;
  client *tmp2;

  tmp1 = dta_pos->first;
  while(1){
    tmp2 = tmp1;
    if(tmp1->next == NULL){ //just one struct in dyn list
      free(tmp1);
      break;
    }
    nof_del_structs++;
    tmp1 = tmp1->next;
    free(tmp2);
  }
  dta_pos->first = NULL;
  dta_pos->last = NULL;
  return nof_del_structs;
}

void del_given_struct_from_clients(clients_list *dta_pos, client *each_client){
  my_assert(dta_pos);
  if(dta_pos->first == NULL){
    fprintf(stderr, "The queue public_clients_list is empty. You can't take anything from out there!\n");
    return;
  }
  client *tmp = dta_pos->first;
  if(dta_pos->first->next == NULL){ //There is only one struct in the queue to be deleted
    dta_pos->first = NULL;
    dta_pos->last = NULL;
    free(tmp);
  }else{ //There is more than one struct in the queue
    if(dta_pos->first == each_client){ //It is the 1st one we desire to delete
      if(tmp->next == NULL){ //Only one struct in list
        dta_pos->first = NULL;
        dta_pos->last = NULL;
      }else{ //More than one struct
        dta_pos->first = tmp->next;
        dta_pos->first->prev = NULL;
      }
      free(tmp);
    }else if(dta_pos->last == each_client){ //It is the last one we desire to delete
      tmp = dta_pos->last;
      dta_pos->last->prev->next = NULL;
      dta_pos->last = dta_pos->last->prev;
      free(tmp);
    }else{ //Struct being deleted from queue is in betwen 1st and last
      tmp = dta_pos->last;
      while(1){
        if(tmp == each_client){
          break;
        }
        tmp = tmp->prev;
        if(tmp == NULL){
          fprintf(stderr, "Inputed struct to be deleted not found in list!\n");
          return;
        }
      }
      each_client->prev->next = each_client->next;
      each_client->next->prev = each_client->prev;
      free(tmp);
    }
  }
  return;
}

 void create_queues_for_clients(clients_list *dta_pos_1, clients_list *dta_pos_2){
   create_queue_for_clients(dta_pos_1);
   create_queue_for_clients(dta_pos_2);
 }

struct pointers *malloc_struct_pointers(){
   pointers *tmp = malloc(sizeof(struct pointers));
   if(tmp == NULL){
     fprintf(stderr, "Couldn't allocate more memory for struct public_client!\n");
     return NULL;
   }
   return tmp;
 }

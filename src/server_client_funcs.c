/**
 * @author     Daniel Stasek
 * @date       June 24, 2016
 * @version    1.0
 */
#include "server_client_funcs.h"

clients_list *pblc_clnts_global_var;

int check_for_input_arguments(int argc, char *argv[], int *ports){
  int private_port, public_port;

  if(argc != 3){
    fprintf(stderr, "Invalid input arguments.\nThere are 2 input arguments required!\nRun as ./server <port_for_private_comm> <port_for_public_comm>\n");
    exit(-1);
  }
  if(!(isdigit(*argv[1])) || !(isdigit(*argv[2]))){
    fprintf(stderr, "One of inputed arguments or both are non-digit.\nPlease enter arguments as follows: ./server <port_for_private_comm> <port_for_public_comm>\n");
    exit(-1);
  }
  private_port = atoi(argv[1]);
  public_port = atoi(argv[2]);

  ports[0] = private_port;
  ports[1] = public_port;

  return 0;
}

int socketfiledesc_ready2connect(int port){
  int sockfd;
  struct sockaddr_in addrinfo;

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){ //alloc socket && socket file descriptor
    fprintf(stderr, "socket() failed.\n");
    exit(-1);
  }
  addrinfo.sin_family = AF_INET; //IPv4
  addrinfo.sin_port = htons(port); //local port
  addrinfo.sin_addr.s_addr = htonl(INADDR_ANY); //any incoming interface

  if(bind(sockfd, (struct sockaddr *)&addrinfo, sizeof(addrinfo)) < 0){ //bind port
    fprintf(stderr, "bind() failed at port %d.\n", port);
    exit(-1);
  }
  if(listen(sockfd, MAXPENDING) < 0){
    fprintf(stderr, "listen() failed.\n");
    exit(-1);
  }

  return sockfd;
}

void run_prep_before_select(int argc, char *argv[], int *sockfd_priv_pub, fd_set *readset, int *max_sockfd_val){
  int ports[2];

  check_for_input_arguments(argc, argv, ports); //ports filled
  sockfd_priv_pub[0] = socketfiledesc_ready2connect(ports[0]); //private port
  sockfd_priv_pub[1] = socketfiledesc_ready2connect(ports[1]); //public port

  FD_ZERO(readset); //set for select() manipulation
  FD_SET(sockfd_priv_pub[0], readset);
  FD_SET(sockfd_priv_pub[1], readset);
  *max_sockfd_val = (sockfd_priv_pub[0] > sockfd_priv_pub[1])?sockfd_priv_pub[0]:sockfd_priv_pub[1]; //add the max value so select() knows where to end looking for sockfds
}

void filedesc_set_bin(int *sockfd_priv_pub, fd_set *readset){
  FD_ZERO(readset);
  FD_SET(sockfd_priv_pub[0], readset);
  FD_SET(sockfd_priv_pub[1], readset);

}

void accept_pblc_client_run_thrd(int public_sockfd, clients_list *pblc_clnts){
  int client_sockfd, client_len;
  struct sockaddr_in incoming_client;
  pthread_t thread_dta;
  client *pblc_client;
  static int client_ID = 1; //beware of static used to not mess main()
  pointers *pDta = malloc_struct_pointers(); //it has to be done this way, cuz the pointer of locale defined variable could be lost

  pblc_client = malloc_struct_client();
  push_queue_clients(pblc_clnts, pblc_client); //client in public client list

  client_len = sizeof(struct sockaddr_in);
  client_sockfd = accept(public_sockfd, (struct sockaddr *)&incoming_client, (socklen_t *)&client_len); //connection accepted

  pblc_client->ID = client_ID;
  pblc_client->client_sockfd = client_sockfd; //struct filled
  client_ID++; //increased to next fnc run

  pDta->each_client = pblc_client;
  pDta->list_of_clnts = pblc_clnts;

  if(pthread_create(&thread_dta, NULL, check_if_pblc_connected, (void *)pDta) < 0) //thread started, check passed function what is done
    fprintf(stderr, "Could not create a thread for public client number %d.\n", (client_ID - 1));
}

void *check_if_pblc_connected(void *pDta){
  pointers *pData = (struct pointers *)pDta;
  client *pblc_client = pData->each_client;
  clients_list *pblc_clnts = pData->list_of_clnts;
  int read_size;

  read_size = recv(pblc_client->client_sockfd, NULL, 0, 0);

  if(read_size == 0){
    printf("Public client number %d disconnected.\n", pblc_client->ID);
    del_given_struct_from_clients(pblc_clnts, pblc_client);
  }
  if(read_size == -1){
    fprintf(stderr, "recv() of client number %d failed.\n", pblc_client->ID);
  }
  free(pData); //what was mallocated has to be freed :o]
  return NULL;
}

void accept_prvt_client_run_thrd(int private_sockfd, clients_list *prvt_clnts){
  int client_sockfd, client_len;
  struct sockaddr_in incoming_client;
  pthread_t thread_dta;
  client *prvt_client;
  static int prvt_client_ID = 1; //beware of static used to not mess main()
  pointers *pDta = malloc_struct_pointers(); //it has to be done this way, cuz the pointer of locale defined variable could be lost

  prvt_client = malloc_struct_client();
  push_queue_clients(prvt_clnts, prvt_client); //client in private client list

  client_len = sizeof(struct sockaddr_in);
  client_sockfd = accept(private_sockfd, (struct sockaddr *)&incoming_client, (socklen_t *)&client_len); //connection accepted

  prvt_client->ID = prvt_client_ID;
  prvt_client->client_sockfd = client_sockfd; //struct filled
  prvt_client_ID++; //increased to next fnc run

  pDta->each_client = prvt_client;
  pDta->list_of_clnts = prvt_clnts;

  if((pthread_create(&thread_dta, NULL, recv_from_prvt_client, (void *)pDta)) < 0)
    fprintf(stderr, "Could not create a thread for private client number %d.\n", (prvt_client_ID - 1));
}

void *recv_from_prvt_client(void *pDta){
  pointers *pData = (struct pointers *)pDta;
  client *prvt_client = pData->each_client;
  clients_list *prvt_clnts = pData->list_of_clnts;
  int read_size;
  char message_from_clnt[1024];

  free(pData); //what was mallocated has to be freed :o]

  while((read_size = recv(prvt_client->client_sockfd, message_from_clnt, 1024, 0)) > 0){
    printf("Recieved from client number %d: %s\n",prvt_client->ID, message_from_clnt);
    continue_with_mssg_processing(message_from_clnt, prvt_client);
    memset(message_from_clnt, 0, 1024); //to zero the string, so no mess remains there
  }

  if(read_size == 0){
    printf("Private client number %d disconnected.\n", prvt_client->ID);
    prvt_client_disc_send_change_in_orderbook_2_pblc(prvt_client->ID);
    del_all_of_client_orders(prvt_client->ID);
    del_given_struct_from_clients(prvt_clnts, prvt_client);
  }
  if(read_size == -1){
    fprintf(stderr, "recv() of client number %d failed.\n", prvt_client->ID);
  }
  return NULL;
}

void continue_with_mssg_processing(char *message_from_clnt, client *prvt_client){
  char *message = NULL, *orderId = NULL, *price = NULL, *quantity = NULL, *side = NULL; //strndup() will malloc enough space for it, don't forget to free it
  pthread_t thread_dta;
  order_data *ordr_dta; //data to thread paased trough this struct

  /*only the values that are passed in message_from_clnt are malloc and filled*/
  parse_json_add_to_strings(message_from_clnt, &message, &orderId, &price, &quantity, &side); //strings filled by desired values

  if(strcmp(message, "createOrder") == 0){ //are eq
    add_order_to_db(orderId, price, quantity, side, prvt_client);
    ordr_dta = malloc_struct_order_data(side, price); //struct filled and malloc
    send_exec_report(orderId, "NEW", prvt_client); //2 prvt_client

    /*creating thread, exec in check_if_anything_to_be_traded()*/
    if((pthread_create(&thread_dta, NULL, check_if_anything_to_be_traded, (void *)ordr_dta)) < 0)
      fprintf(stderr, "Could not create a thread for private client number %d.\nError occured in continue_with_mssg_processing()\n", prvt_client->ID);

  }else if(strcmp(message, "cancelOrder") == 0){ ///are eq
    canceled_order_notify_pblc(prvt_client->ID, orderId);
    delete_order_from_db(orderId);
    send_exec_report(orderId, "CANCELED", prvt_client); //2 prvt_client

  }else{
    fprintf(stderr, "Unknown message!\n It is meant to be 'createOrder' or 'cancelOrder' only\n");
  }
  free_all_data_from_heap(message, orderId, price, quantity, side);
}

void send_exec_report(char *orderId, char *report, client *prvt_client){
  char final_message[1024];

  sprintf(final_message, "{\"message\": \"executionReport\", \"orderId\": %s, \"report\": \"%s\"}\n", orderId, report);

  if(send(prvt_client->client_sockfd, final_message, strlen(final_message),0 ) < 0)
    fprintf(stderr, "Sending execution report to private client number %d failed.\n", prvt_client->ID);

}

void *check_if_anything_to_be_traded(void *input_struct){
  char *bid_row[8], *ask_row[8], *trdn_initiator[8];
  double best_bid, best_ask, bid_time, ask_time, executed_price, *traded_prices;
  int bid_sockfd, ask_sockfd, bid_quantity, ask_quantity, executed_quantity, loop_cntr = 0, TRADED;
  order_data *ordr_dta = (struct order_data *)input_struct;

  while(1){ //loop until there is something to trade
    bid_row[0] = NULL; //so we do know if command found anything in db
    ask_row[0] = NULL; //so we do know if command found anything in db
    get_best_bidask_rows_from_db(bid_row, ask_row); //malloc for bid/ask_row make free()!!!

    if(bid_row[0] != NULL && ask_row[0] != NULL){ //best prices from bid/ask found
      modify_pchar_to_digit(bid_row, ask_row, &bid_quantity, &ask_quantity, &bid_time, &ask_time, &bid_sockfd, &ask_sockfd, &best_bid, &best_ask);

      if(loop_cntr == 0){
        copy_two_array_pointers(((ask_time > bid_time)?ask_row:bid_row), trdn_initiator);
      }

      if(best_bid >= best_ask){ //lets trade !!
        traded_prices = realloc(traded_prices, sizeof(double)*(loop_cntr+1)); //dont forget to free
        TRADED++;
        executed_price = rtrn_executable_price(best_bid, best_ask, ask_time, bid_time); //the published one

        if(bid_quantity == ask_quantity){ //both trades will be fully traded
          executed_quantity = bid_quantity;
          delete_order_from_db(bid_row[4]);
          delete_order_from_db(ask_row[4]);
        }else{ //demand and supply volumes differ
          executed_quantity = (bid_quantity > ask_quantity)?ask_quantity:bid_quantity; //the smaller one

          if(bid_quantity > ask_quantity){
            delete_order_partly(bid_row[4], bid_quantity, ask_quantity); //cuz there is more than to be executed
            delete_order_from_db(ask_row[4]);
          }else{
            delete_order_partly(ask_row[4], bid_quantity, ask_quantity);
            delete_order_from_db(bid_row[4]);
          }
        }
        traded_prices[loop_cntr] = (((strcmp(trdn_initiator[1], "BUY")) == 0)?best_ask:best_bid);
        send_trade_rep_2_pblc((bid_quantity > ask_quantity)?ask_row[3]:bid_row[3],((ask_time > bid_time)?ask_row[2]:bid_row[2]));
        send_exec_report_fill(bid_row[4], best_bid, executed_quantity, bid_sockfd); //its own price is given
        send_exec_report_fill(ask_row[4], best_ask, executed_quantity, ask_sockfd); //its own price is given

      }else{ //no more matches possible for now
        decide_whether_free_row(bid_row, ask_row);
        break;
      }
    }
    decide_whether_free_row(bid_row, ask_row);
    if(bid_row[0] == NULL || ask_row[0] == NULL){ //there is no trades at one side
      break; //leave - no more trades to be made
    }
    loop_cntr++;
  }
  if(TRADED >= 1){ //there was at least 1 trade, so we will report public clients about changes in orderbook
    check_if_anything_left_at_trade_initiator(trdn_initiator[4], trdn_initiator[1], trdn_initiator[2]);
    send_changes_in_orderbook_to_pblc(TRADED, traded_prices, ((strcmp(trdn_initiator[1], "SELL") == 0)?"ASK":"BID"));
  }else{  //not traded, just sent report orderbook
    send_pblc_new_orderbook_order(ordr_dta->side, ordr_dta->price);
  }
  free(ordr_dta->side);
  free(ordr_dta->price);
  free(ordr_dta);
  for(loop_cntr = 0; loop_cntr < 8; loop_cntr++) free(trdn_initiator[loop_cntr]);
  return NULL;
}

void free_taken_row(char **to_be_freed){
  int i;
  for(i = 0; i < 8; i++){
    free(to_be_freed[i]);
  }
}

 void decide_whether_free_row(char **bid_row, char **ask_row){
   if(bid_row[0] != NULL) //it was filled by mallocs, needs to be freed
     free_taken_row(bid_row);
   if(ask_row[0] != NULL) //it was filled by mallocs, needs to be freed
     free_taken_row(ask_row);
 }

void modify_pchar_to_digit( char **bid_row, char **ask_row,
                            int *bid_quantity, int *ask_quantity,
                            double *bid_time, double *ask_time,
                            int *bid_sockfd, int *ask_sockfd,
                            double *best_bid, double *best_ask){
  *best_bid = atof(bid_row[2]);
  *best_ask = atof(ask_row[2]);

  *bid_sockfd = atoi(bid_row[6]);
  *ask_sockfd = atoi(ask_row[6]);

  *bid_time = atof(bid_row[7]);
  *ask_time = atof(ask_row[7]);

  *bid_quantity = atoi(bid_row[3]);
  *ask_quantity = atoi(ask_row[3]);
}

double rtrn_executable_price(double best_bid, double best_ask, double ask_time, double bid_time){
    return (ask_time > bid_time)?best_bid:best_ask;
}

void send_exec_report_fill(char *orderId, double exec_price, int quantity, int sockfd){
  char final_message[1024], str_price[32], str_quant[32];
  sprintf(str_price, "%f", exec_price);
  sprintf(str_quant, "%d", quantity);

  sprintf(final_message, "{\"message\": \"executionReport\", \"orderId\": %s, \"report\": \"FILL\", \"price\": %s, \"quantity\": %s}\n", orderId, str_price, str_quant);

  if(send(sockfd, final_message, strlen(final_message), 0) < 0)
    fprintf(stderr, "Sending report about executed trade to private client number failed.\n");
}

void send_trade_rep_2_pblc(char *quantity, char *price){
  client *tmp = pblc_clnts_global_var->first;
  char message_to_send[512], tm[64];
  struct timeval tv;
  /*Get epochUnix time with milisec precision, time is GMT*/
  gettimeofday(&tv, NULL);
  sprintf(tm, "%ld.%ld", tv.tv_sec, tv.tv_usec);

  sprintf(message_to_send, "{\"type\": \"trade\", \"time\": %s, \"price\": %s, \"quantity\": %s}\n", tm, price, quantity);
  while(tmp != NULL){
    if(send(tmp->client_sockfd, message_to_send, strlen(message_to_send), 0) < 0)
      fprintf(stderr, "Sending orderbok report to public client number %d failed.\n", tmp->ID);
    tmp = tmp->next;
  }
}

void copy_two_array_pointers(char **passed_row, char **trnd_initiator){
  int i;

  for(i = 0; i < 8; i++){
    trnd_initiator[i] =  malloc(sizeof(char)*(strlen(passed_row[i])+1));
    strcpy(trnd_initiator[i], passed_row[i]);
  }
}

struct order_data *malloc_struct_order_data(char *side, char *price){
   order_data *tmp = malloc(sizeof(struct order_data));
   if(tmp == NULL){
     fprintf(stderr, "Couldn't allocate more memory for struct public_client!\n");
     return NULL;
   }
   char *pside = malloc(sizeof(char)*(strlen("bid")+1));
   if(strcmp(side, "BUY") == 0){
     strcpy(pside, "bid");
   }else{
     strcpy(pside, "ask");
   }

   char *pprice = malloc(sizeof(char)*(strlen(price)+1));
   strcpy(pprice, price);

   tmp->side = pside;
   tmp->price = pprice;

   return tmp;
 }

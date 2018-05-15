/**
 * @author     Daniel Stasek
 * @date       June 24, 2016
 * @version    1.0
 */
#include "sqlite_calls.h"
#include "clientdta.h"

sqlite3 *db;

sqlite3  * del_old_create_new_database(char *db_name){
   sqlite3 *db2;
   int wrrng1, wrrng2;
   char *err_msg;
   char *sql_command =
        "DROP TABLE IF EXISTS BID;"
				"DROP TABLE IF EXISTS ASK;"
				"CREATE TABLE BID(id INTEGER PRIMARY KEY, side TEXT, price REAL, quantity INTEGER, orderId INTEGER, clientId INTEGER, clientsckfd INTEGER, time DOUBLE);"
				"CREATE TABLE ASK(id INTEGER PRIMARY KEY, side TEXT, price REAL, quantity INTEGER, orderId INTEGER, clientId INTEGER, clientsckfd INTEGER, time DOUBLE);";

  wrrng1 = sqlite3_open(db_name, &db2); //opened even if did not existed
  wrrng2 = sqlite3_open_v2(db_name, &db2, SQLITE_OPEN_READWRITE, NULL); //readwrite mode, more optimalised
  if(wrrng1 != SQLITE_OK || wrrng2 != SQLITE_OK){
    fprintf(stderr, " SQLite: cannot open: %s\n", sqlite3_errmsg(db2));
		exit(-1);
	}
  /*exec sql_command*/
  wrrng1 = sqlite3_exec(db2, sql_command, 0, 0, &err_msg);
  if(wrrng1 != SQLITE_OK){
		fprintf(stderr, "SQLite error: %s\n", err_msg);
		sqlite3_close(db2);
		exit(-1); //if it can not be set up, server terminates with -1
	}

  return db2;
}

void add_order_to_db(char *orderId, char *price, char *quantity, char *side, client *prvt_client){
  char *err_msg, final_command[1024], str_clntID[64], str_clntsckfd[64], tm[64];
  int wrrng;
  double time;
  struct timeval tv;

  /*Get epochUnix time with milisec precision, time is GMT*/
  gettimeofday(&tv, NULL);
  sprintf(tm, "%ld.%ld", tv.tv_sec, tv.tv_usec); //converts into string tm

  /*Cleint ID to str*/
  sprintf(str_clntID, "%d", prvt_client->ID);
  sprintf(str_clntsckfd, "%d", prvt_client->client_sockfd);

  /*Input ops*/
  sprintf(final_command, "INSERT INTO %s(side, price, quantity, orderId, clientId, clientsckfd, time) VALUES('%s', %s, %s, %s, %s, %s, %s);", \
    ((strcmp(side, "BUY") == 0)?"BID":"ASK"), side, price, quantity, orderId, str_clntID, str_clntsckfd, tm);

  /*exec final_command*/
  wrrng = sqlite3_exec(db, final_command, 0, 0, &err_msg);
  if(wrrng != SQLITE_OK){
    fprintf(stderr, "SQLite error in add_order_to_db: %s\n", err_msg);
		sqlite3_free(err_msg);
	}
}

void delete_order_from_db(char *orderId){
  char *err_msg, final_command[1024];
  int wrrng1, wrrng2;

  /*BID side and exec*/
  sprintf(final_command, "DELETE FROM BID WHERE orderId = %s;", orderId);
  wrrng1 = sqlite3_exec(db, final_command, 0, 0, &err_msg);

  /*ASK side and exec*/
  sprintf(final_command, "DELETE FROM ASK WHERE orderId = %s;", orderId);
  wrrng2 = sqlite3_exec(db, final_command, 0, 0, &err_msg);

  /*Check for possible errs*/
  if(wrrng1 != SQLITE_OK || wrrng2 != SQLITE_OK){
    fprintf(stderr, "SQLite error in delete_order_from_db: %s\n", err_msg);
    sqlite3_free(err_msg);
  }
}

void del_all_of_client_orders(int client_ID){
  char *err_msg, final_command[1024], str_clntID[32];
  int wrrng1, wrrng2;

  sprintf(str_clntID, "%d", client_ID); //tranferd to string

  /*BID side and exec*/
  sprintf(final_command, "DELETE FROM BID WHERE clientId = %s;", str_clntID);
  wrrng1 = sqlite3_exec(db, final_command, 0, 0, &err_msg);

  /*ASK side and exec*/
  sprintf(final_command, "DELETE FROM ASK WHERE clientId = %s;", str_clntID);
  wrrng2 = sqlite3_exec(db, final_command, 0, 0, &err_msg);

  /*Check for possible errs*/
  if(wrrng1 != SQLITE_OK || wrrng2 != SQLITE_OK){
    fprintf(stderr, "SQLite error in del_all_of_client_orders: %s\n", err_msg);
    sqlite3_free(err_msg);
  }
}

void get_best_bidask_rows_from_db(char **bid_row, char **ask_row){
  char *err_msg, final_command[1024];
  int wrrng1, wrrng2;

  /*Get row on BID side and fill passed argument*/
  strcpy(final_command, "SELECT * FROM BID WHERE price = (SELECT price FROM BID ORDER BY price DESC LIMIT 1) ORDER BY time ASC LIMIT 1;");
  wrrng1 = sqlite3_exec(db, final_command, callback_row_from_db, (void **)bid_row, &err_msg);

  /*Get row on ASK side and fill passed argument*/
  strcpy(final_command, "SELECT * FROM ASK WHERE price = (SELECT price FROM ASK ORDER BY price ASC LIMIT 1) ORDER BY time ASC LIMIT 1;");
  wrrng2 = sqlite3_exec(db, final_command, callback_row_from_db, (void **)ask_row, &err_msg);

  /*Check for possible errs*/
  if(wrrng1 != SQLITE_OK || wrrng2 != SQLITE_OK){
    fprintf(stderr, "SQLite error in get_best_bidask_rows_from_db: %s\n", err_msg);
    sqlite3_free(err_msg);
  }
}

int callback_row_from_db(void *input_dta, int nof_called_args, char **called_args, char **CollmnName){
  char **called_row_2 = (char **)input_dta;
  int i;

  if(nof_called_args == 8){ //number of collumns in db
    for(i = 0; i < 8; i++){
      called_row_2[i] = malloc(sizeof(called_args[i]));
      strcpy(called_row_2[i], called_args[i]);
    }
  }
  return 0;
}

void delete_order_partly(char *orderId, int bid_quantity, int ask_quantity){
  char final_message[1024], str_quant[32], *err_msg;
  int rest_of_quantity, wrrng1;

  rest_of_quantity = (bid_quantity > ask_quantity)?(bid_quantity - ask_quantity):(ask_quantity - bid_quantity);
  sprintf(str_quant, "%d", rest_of_quantity);

  if(bid_quantity > ask_quantity){
    strcpy(final_message, "UPDATE BID SET quantity =");
  }else{
    strcpy(final_message, "UPDATE ASK SET quantity =");
  }
  strcat(final_message, str_quant);
  strcat(final_message, " WHERE orderId = ");
  strcat(final_message, orderId);
  strcat(final_message, ";");

  wrrng1 = sqlite3_exec(db, final_message, 0, 0, &err_msg);
  if(wrrng1 != SQLITE_OK){
    fprintf(stderr, "SQLite error in delete_order_partly: %s\n", err_msg);
    sqlite3_free(err_msg);
  }
}

void prvt_client_disc_send_change_in_orderbook_2_pblc(int prvt_client_ID){
  char final_message[1024], str_ID[32], *err_msg, *bid = "BID", *ask = "ASK";
  char *p_side_clntid[2]; //1st exec_side, 2nd client_ID -> so we can pass those to callback
  int  wrrng, i;

  sprintf(str_ID, "%d", prvt_client_ID);
  p_side_clntid[1] = str_ID;

  for(i = 0; i < 2; i++){
    p_side_clntid[0] = (i == 0)?bid:ask;
    sprintf(final_message, "SELECT price FROM %s WHERE clientId = %s;", p_side_clntid[0], str_ID);

    wrrng = sqlite3_exec(db, final_message, callback_client_prices_v2, (void **)p_side_clntid, &err_msg);
    if(wrrng != SQLITE_OK){
      fprintf(stderr, "SQLite error in prvt_client_disc_send_change_in_orderbook_2_pblc: %s\n", err_msg);
      sqlite3_free(err_msg);
    }
  }
}

int callback_client_prices_v2(void *input_dta, int nof_called_args, char **called_args, char **CollmnName){
  int i, wrrng;
  char *err_msg, final_message[512], passed_message[512], *p_final_passed_mssg[2]; //freee

  char **p_side_clntid = (char **)input_dta; //conversion

  for(i = 0; i < nof_called_args; i++){
    sprintf(final_message, "SELECT SUM(quantity) FROM %s WHERE clientId != %s AND price = %s;", p_side_clntid[0], p_side_clntid[1], called_args[i]);

    /*pass string with data*/
    sprintf(passed_message, "{\"type\": \"orderbook\", \"side\": \"%s\", \"price\": %s, \"quantity\": ", ((strcmp(p_side_clntid[0], "BID") == 0)?"bid":"ask"), called_args[i]);
    p_final_passed_mssg[0] = final_message;
    p_final_passed_mssg[1] = passed_message;

    /*CREATING THREAD because I need to enter into new process from reason that previous callback already took char **called_args space in this process*/
    wrrng = sqlite3_exec(db, p_final_passed_mssg[0], callback_send_messages_v2, (void *)p_final_passed_mssg[1], &err_msg);
    if(wrrng != SQLITE_OK){
      fprintf(stderr, "SQLite error in callback_client_prices_v2: %s\n", err_msg);
      sqlite3_free(err_msg);
    }
  }
  if(strcmp(p_final_passed_mssg[1], "err") != 0){ //there was nothing more at given price to count and send
    char message_to_send[512];
    client *tmp = pblc_clnts_global_var->first;

    sprintf(message_to_send, "%s0}\n", p_final_passed_mssg[1]);
    while(tmp != NULL){
      if(send(tmp->client_sockfd, message_to_send, strlen(message_to_send), 0) < 0)
        fprintf(stderr, "Sending orderbok report to public client number %d failed.\n", tmp->ID);
      tmp = tmp->next;
    }
  }
  return 0;
}

int callback_send_messages_v2(void *input_dta2, int nof_called_args2, char **called_args2, char **CollmnName){
  if(nof_called_args2 > 0){
    char *passed_message = (char *)input_dta2; //conversion
    char final_message[512];
    client *tmp = pblc_clnts_global_var->first;

    if(called_args2[0] == NULL || (strcmp(called_args2[0], "0") == 0))//cuz of undefined behavior of callback in callback
      return 0;

    sprintf(final_message, "%s%s}\n", passed_message, called_args2[0]);
    strcpy(passed_message, "err");

    while(tmp != NULL){
      if(send(tmp->client_sockfd, final_message, strlen(final_message), 0) < 0)
        fprintf(stderr, "Sending orderbok report to public client number %d failed.\n", tmp->ID);
      tmp = tmp->next;
    }
  }
  return 0;
}

void canceled_order_notify_pblc(int client_ID, char *orderId){
  char *err_msg, final_message[512], str_is_ask[8], str_price[32], filled_quantity[64], side[8], str_ID[32];
  int wrrng1, wrrng2, wrrng3;
  client *tmp = pblc_clnts_global_var->first;

  strcpy(filled_quantity, "0");
  sprintf(str_ID, "%d", client_ID);

  /*We need to get side before counting quantity*/
  sprintf(final_message ,"SELECT count(orderId = %s) FROM ASK;", orderId);
  wrrng1 = sqlite3_exec(db, final_message, callback_get_side, (void *)str_is_ask, &err_msg);
  if(strcmp(str_is_ask, "1") == 0){ //is ask
    strcpy(side, "ASK");
  }else{
    strcpy(side, "BID");
  }

  /*We need to get price before counting quantity*/
  sprintf(final_message, "SELECT price FROM %s WHERE orderId = %s;", side, orderId);
  wrrng1 = sqlite3_exec(db, final_message, callback_get_price, (void *)str_price, &err_msg);

  sprintf(final_message, "SELECT SUM(quantity) FROM %s WHERE price = %s AND orderId != %s;", side, str_price, orderId);
  wrrng3 = sqlite3_exec(db, final_message, callback_rtrn_quantity, (void *)filled_quantity, &err_msg);

  /*Errs checking*/
  if(wrrng1 != SQLITE_OK || wrrng2 != SQLITE_OK || wrrng3 != SQLITE_OK){
    fprintf(stderr, "SQLite error in canceled_order_notify_pblc: %s\n", err_msg);
    sqlite3_free(err_msg);
  }

  /*Message to public clients*/
  if(strcmp(filled_quantity, "0") == 0){ //callback was not called, because of not val to be returned
    sprintf(final_message,"{\"type\": \"orderbook\", \"side\": \"%s\", \"price\": %s, \"quantity\": 0}\n", ((strcmp(side, "BID") == 0)?"bid":"ask"), str_price);
  }else{ //callback was called and we have our quantity val
    sprintf(final_message,"{\"type\": \"orderbook\", \"side\": \"%s\", \"price\": %s, \"quantity\": %s}\n", ((strcmp(side, "BID") == 0)?"bid":"ask"), str_price, filled_quantity);
  }

  /*sending to public clientc*/
  while(tmp != NULL){
    if(send(tmp->client_sockfd, final_message, strlen(final_message), 0) < 0)
      fprintf(stderr, "Sending orderbok report to public client number %d failed.\n", tmp->ID);
    tmp = tmp->next;
  }
}

int callback_rtrn_quantity(void *input_dta, int nof_called_args, char **called_args, char **CollmnName){
  char *filled_quantity = (char *)input_dta;

  if(called_args[0] == NULL) //cuz of sum() sqlite3 function
    return 0;

  strcpy(filled_quantity, called_args[0]);
  return 0;
}

int callback_get_side(void *input_dta, int nof_called_args, char **called_args, char **CollmnName){
  char *is_ask = (char *)input_dta;

  strcpy(is_ask, called_args[0]);
  return 0;
}

int callback_get_price(void *input_dta, int nof_called_args, char **called_args, char **CollmnName){
  char *price = (char *)input_dta;

  strcpy(price, called_args[0]);
  return 0;
}

void check_if_anything_left_at_trade_initiator(char *orderId, char *side, char* price){
  char *err_msg, final_command[512], quantity[32];
  int wrrng;
  client *tmp = pblc_clnts_global_var->first;
  strcpy(quantity, "0");

  sprintf(final_command, "SELECT quantity FROM %s WHERE orderId = %s;", (((strcmp(side, "SELL")) == 0)?"ASK":"BID"), orderId);

  wrrng = sqlite3_exec(db, final_command, callback_get_quantity, (void *)quantity, &err_msg);
  if(wrrng != SQLITE_OK){
    fprintf(stderr, "SQLite error in check_if_anything_left_at_trade_initiator: %s\n", err_msg);
    sqlite3_free(err_msg);
  }
  if(strcmp(quantity, "0") == 0) //this order didn't changed the structure of orderbook for public client
    return;

  sprintf(final_command, "{\"type\": \"orderbook\", \"side\": \"%s\", \"price\": %s, \"quantity\": %s}\n", (((strcmp(side, "SELL")) == 0)?"ask":"bid"), price, quantity);
  while(tmp != NULL){
    if(send(tmp->client_sockfd, final_command, strlen(final_command), 0) < 0)
      fprintf(stderr, "Sending orderbok report to public client number %d failed.\n", tmp->ID);
    tmp = tmp->next;
  }
}

int callback_get_quantity(void *input_dta, int nof_called_args, char **called_args, char **CollmnName){
  char *quantity = (char *)input_dta;

  strcpy(quantity, called_args[0]);

  return 0;
}

void send_changes_in_orderbook_to_pblc(int nof_cleared_orders, double *traded_prices, char *side){
  double already_passed_price = 0;
  int i, wrrng;
  char *err_msg, final_command[512], str_price[32], str_quant[32];
  strcpy(str_quant, "0");
  client *tmp = pblc_clnts_global_var->first;

  for(i = 0; i < nof_cleared_orders; i++){
    if(already_passed_price == traded_prices[i]){
      continue;
    }
    sprintf(str_price, "%f", traded_prices[i]);
    sprintf(final_command, "SELECT SUM(quantity) FROM %s WHERE price = %s;", (((strcmp(side, "ASK")) == 0)?"BID":"ASK"), str_price);

    wrrng = sqlite3_exec(db, final_command, callback_get_quantity_from_sum, (void *)str_quant, &err_msg);
    if(wrrng != SQLITE_OK){
      fprintf(stderr, "SQLite error in send_changes_in_orderbook_to_pblc: %s\n", err_msg);
      sqlite3_free(err_msg);
    }

    sprintf(final_command, "{\"type\": \"orderbook\", \"side\": \"%s\", \"price\": %s, \"quantity\": %s}\n", (((strcmp(side, "ASK")) == 0)?"bid":"ask"), str_price, str_quant);
    while(tmp != NULL){
      if(send(tmp->client_sockfd, final_command, strlen(final_command), 0) < 0)
        fprintf(stderr, "Sending orderbok report to public client number %d failed.\n", tmp->ID);
      tmp = tmp->next;
    }
    strcpy(str_quant, "0");
    already_passed_price = traded_prices[i];
    tmp = pblc_clnts_global_var->first;
  }
  free(traded_prices);
}

int callback_get_quantity_from_sum(void *input_dta, int nof_called_args, char **called_args, char **CollmnName){
  char* quantity = (char *)input_dta;

  if(called_args[0] == NULL) //sum is kinda bug -callback is executed but not desirable
    return 0;

  strcpy(quantity, called_args[0]);

  return 0;
}

void send_pblc_new_orderbook_order(char *side, char *price){
  int i, wrrng;
  char *err_msg, final_command[512], str_quant[32];
  client *tmp = pblc_clnts_global_var->first;
  strcpy(str_quant, "0");

  sprintf(final_command, "SELECT SUM(quantity) FROM %s WHERE price = %s;", (((strcmp(side, "bid")) == 0)?"BID":"ASK"), price);

  wrrng = sqlite3_exec(db, final_command, callback_get_quantity_from_sum, (void *)str_quant, &err_msg);
  if(wrrng != SQLITE_OK){
    fprintf(stderr, "SQLite error in send_pblc_new_orderbook_order: %s\n", err_msg);
    sqlite3_free(err_msg);
  }
  sprintf(final_command, "{\"type\": \"orderbook\", \"side\": \"%s\", \"price\": %s, \"quantity\": %s}\n", side, price, str_quant);
  while(tmp != NULL){
    if(send(tmp->client_sockfd, final_command, strlen(final_command), 0) < 0)
      fprintf(stderr, "Sending orderbok report to public client number %d failed.\n", tmp->ID);
    tmp = tmp->next;
  }
}

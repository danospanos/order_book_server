/**
 * @author     Daniel Stasek
 * @date       June 24, 2016
 * @version    1.0
 */
#ifndef SQLITE_CALLS_H
#define SQLITE_CALLS_H

#include <sqlite3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "clientdta.h"
#include <sys/time.h>
#include "server_client_funcs.h"


extern sqlite3 *db;
/*struct declared because of problems passing (void **)(array of pointers) to pthread_create*/
typedef struct mssg_pointers{
  char *final_message;
  char *passed_message;
}mssg_pointers;
/**
 * @brief Under passed db_name creates database with ASK and BID tables
 * @details If such a database already exists it is being dumped
 */
sqlite3 *del_old_create_new_database(char *db_name);
/**
 * @brief Adds order passed by arguments into right table - BID, ASK
 */
void add_order_to_db(char *orderId, char *price, char *quantity, char *side, client *prvt_client);
/**
 * @brief Deletes order from database based on passed orderId
 * @details Both sides of order book are checked
 */
void delete_order_from_db(char *orderId);
/**
 * @brief Deletes all orders based on passed client_ID
 * @details Used when client disconnect and all his orders needs to be swept
 */
void del_all_of_client_orders(int client_ID);
/**
 * @brief Fills passed arguments with best bid and best ask rows respectively its prices
 */
void get_best_bidask_rows_from_db(char **bid_row, char **ask_row);
/**
 * @brief Fills *input_dta appropriatelly with given row from db
 * @details malloc() used, free() needs to be used elsewhere
 */
int callback_row_from_db(void *input_dta, int nof_called_args, char **called_args, char **CollmnName);
/**
 * @brief Deletes part of order in database
 * @details Functions is run in cases when not all of order quantity is executed
 */
void delete_order_partly(char *orderId, int bid_quantity, int ask_quantity);
/**
 * @brief When private client disconnects there are some of his remaining orders on orderbook
 * @brief This functions checks them and before deleting sends them to public clients
 */
void prvt_client_disc_send_orderbook_2_pblc(int prvt_client_ID);
/**
 * @brief Callbacks are used in sqlite3_exec()
 * @details SUM() sqlite function used, it violates sqlite callback defined behavior!
 * @return 1 when everything goes as defined
 */
int callback_client_prices_v2(void *input_dta_not_used, int nof_called_args, char **called_args, char **CollmnName);
/**
 * @brief It sends messages to all public clients saved in dynamic list
 * @details If SUM() in callback_client_prices_v2() returned " ", then sending is performed in callback_client_prices_v2()
 */
int callback_send_messages_v2(void *input_dta2, int nof_called_args_2, char **called_args_2, char **CollmnName);
/**
 * @brief When private client cancel order, this function should be called to send notifications about its canceling
 */
void canceled_order_notify_pblc(int client_ID, char *orderId);
/**
 * @brief Functions does what its name says it does
 * @details Passed data needs to be checked at sqlite3_exec() as 4th argument
 */
int callback_rtrn_quantity(void *input_dta, int nof_called_args, char **called_args, char **CollmnName);
/**
 * @brief Functions does what its name says it does
 * @details Passed data needs to be checked at sqlite3_exec() as 4th argument
 */
int callback_get_side(void *input_dta, int nof_called_args, char **called_args, char **CollmnName);
/**
 * @brief Functions does what its name says it does
 * @details Passed data needs to be checked at sqlite3_exec() as 4th argument
 */
int callback_get_price(void *input_dta, int nof_called_args, char **called_args, char **CollmnName);
/**
 * @brief Trade initiator is meant to be the order which initiate the cascade of trades
 * @details And there can be something left in quatity, this function checks it
 */
void check_if_anything_left_at_trade_initiator(char *orderId, char *side, char* price);
/**
 * @brief Functions does what its name says it does
 * @details Passed data needs to be checked at sqlite3_exec() as 4th argument
 */
int callback_get_quantity(void *input_dta, int nof_called_args, char **called_args, char **CollmnName);
/**
 * @brief Check changes in order book at the other side of orderbook opposed to check_if_anything_left_at_trade_initiator
 */
void send_changes_in_orderbook_to_pblc(int cleared_orders, double *traded_prices, char *side);
/**
 * @brief Functions does what its name says it does
 * @details Passed data needs to be checked at sqlite3_exec() as 4th argument
 */
int callback_get_quantity_from_sum(void *input_dta, int nof_called_args, char **called_args, char **CollmnName);
/**
 * @brief When this order was not trade initiatior, it needs to be passed to orderbook
 */
void send_pblc_new_orderbook_order(char *side, char *price);

#endif //SQLITE_CALLS_H

/**
 * @author     Daniel Stasek
 * @date       June 24, 2016
 * @version    1.0
 */
#ifndef SERVER_CLIENT_FUNCS_H
#define SERVER_CLIENT_FUNCS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h> //htons, htonl...
#include <netinet/in.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <pthread.h>
#include "clientdta.h"
#include <string.h>
#include "jsmn.h"
#include "sqlite_calls.h"
#include "json_funcs.h"
//#include <unistd.h> //delete its for sleep

extern clients_list *pblc_clnts_global_var;
#define MAXPENDING 20

typedef struct order_data{
  char *side;
  char *price;
}order_data;
/**
 * @brief - Checks for input arguments whether they are filled right and pass them to array int*
 * @return - 0 everything allright, exit(-1) an error occured, end of server
 */
int check_for_input_arguments(int argc, char *argv[], int *ports);
/**
 * @brief Allocate socket to be used in accept()
 * @return - socket file descriptor OR exit(-1) an error occured, end of server
 */
int socketfiledesc_ready2connect(int port);
/**
 * @brief sets readset appropriatelly and runs previous fncs
 */
void run_prep_before_select(int argc, char *argv[], int *sockfd_priv_pub, fd_set *readset, int *max_sockfd_val);
/**
 * @brief readset binary set reset so can be used again in while
 */
void filedesc_set_bin(int *sockfd_priv_pub, fd_set *readset);
/**
 * @brief Accepts connection from public port, clients structs created and filled
 * @details Thread run to listen whether client still connected, if not <- del from dynamic list
 */
void accept_pblc_client_run_thrd(int public_sockfd, clients_list *pblc_clnts);
 /**
  * @brief Function which is passed to pthread_create() and being evluated in created thread
  * @details Simply checks if client is still connected, if not - he is taken out of client_list
  */
void *check_if_pblc_connected(void *pDta);
/**
 * @brief Accepts connection from private port, clients structs created and filled
 * @details Thread run to recieve incoming orders etc..
 */
 void accept_prvt_client_run_thrd(int public_sockfd, clients_list *pblc_clnts);
/**
 * @brief Thread function called in accept_prvt_client_run_thrd()
 * @brief data neded to be passed in struct because (void **)(dta) impossbile
 */
void *recv_from_prvt_client(void *client_struct);
/**
 * @brief Function will decide whether incoming message is "cancelOrder" or "createOrder"
 * @brief and will fork into next threads so there are no delayes
 */
void continue_with_mssg_processing(char *message_from_clnt, client *prvt_client);
/**
 * @brief Send exec report to private client "NEW" or "CANCELED"
 */
void send_exec_report(char *orderId, char* report, client *prvt_client);
/**
 * @brief Matching engine, firstly order are add to db in continue_with_mssg_processing()
 * @details Then matched here and send to both private and public clients
 */
void *check_if_anything_to_be_traded();
/**
 * @brief it free passed row from heap
 */
void free_taken_row(char **to_be_freed);
/**
 * @brief It could or not be filled an malloc, this functions checks if was and free was is needed to be freed
 */
 void decide_whether_free_row();
 /**
  * @brief Simply convert chars to digits
  */
void modify_pchar_to_digit( char **bid_row,  char **ask_row,
                            int *bid_quantity, int *ask_quantity,
                            double *bid_time, double *ask_time,
                            int *bid_sockfd, int *ask_sockfd,
                            double *best_bid, double *best_ask);
/**
 * @brief Returns the price which is older
 */
double rtrn_executable_price(double best_bid, double best_ask, double ask_time, double bid_time);
/**
 * @brief When match of ASK and BID is found, report about trades needs to be sent to private clients
 */
void send_exec_report_fill(char *orderId, double exec_price, int quantity, int sockfd);
/**
 * @brief Sends trade rep
 */
void send_trade_rep_2_pblc(char *quantity, char *price);
/**
 * @brief Does what is says it does but also malloc so must be free
 */
void copy_two_array_pointers(char **passed_row, char **trnd_initiator);
/**
 * @brief malloc struct and returns its pointer to it
 */
struct order_data *malloc_struct_order_data(char *side, char *price);

#endif //SERVER_CLIENT_FUNCS_H

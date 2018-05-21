/**
 * @author     Daniel Stasek
 * @date       June 24, 2016
 * @version    1.0
 */
#include <unistd.h> //close
#include <string.h>
#include <errno.h>
#include "server_client_funcs.h"

//GLOBAL VAR - db declared in sqlite_calls.h, defined in sqlite_call.c
//GLOBAL VAR - pblc_clnts_global_var declared in server_client_funcs.h, defined in server_client_funcs.c

int main(int argc, char *argv[]){
  int sockfd_prvt_pblc[2], max_sockfd_val, result;
  fd_set readset;
  clients_list prvt_clnts, pblc_clnts;

  db = del_old_create_new_database("data.db"); //db declared in sqlite_calls.h, defined in sqlite_call.c
  create_queues_for_clients(&prvt_clnts, &pblc_clnts);
  run_prep_before_select(argc, argv, sockfd_prvt_pblc, &readset, &max_sockfd_val); //sockets ready to be used, and passed to readset
  pblc_clnts_global_var = &pblc_clnts; //pblc_clnts_global_var declared in server_client_funcs.h, defined in server_client_funcs.c

  puts("Waiting for incoming connections...");
  while((result = select(max_sockfd_val + 1, &readset, NULL, NULL, NULL))){ //select if incoming conenctions is from private or public port
    if(result >= 0){ //more than 0 incoming connections to be resolved
      if(FD_ISSET(sockfd_prvt_pblc[0], &readset)){ //incoming new client on private port
        puts("Private port, new client connecting.");
        accept_prvt_client_run_thrd(sockfd_prvt_pblc[0], &prvt_clnts);
      }
      if(FD_ISSET(sockfd_prvt_pblc[1], &readset)){ //incoming new client on public port
        puts("Public port, new client connecting.");
        accept_pblc_client_run_thrd(sockfd_prvt_pblc[1], &pblc_clnts);
      }
      filedesc_set_bin(sockfd_prvt_pblc, &readset); //readset set to default - needed format for select()
    }else if(result < 0){
      fprintf(stderr, "Error with select().\n%s\n", strerror(errno));
    }
  }

  close(sockfd_prvt_pblc[0]); //close allocated sockets at given ports
  close(sockfd_prvt_pblc[1]); //close allocated sockets at given ports
  sqlite3_close(db);
  return 0;
}
 //hheloo
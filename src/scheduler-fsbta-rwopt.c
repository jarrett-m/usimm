#include "utils.h"
#include "utlist.h"
#include <stdio.h>
#include <stdlib.h>

#include "memory_controller.h"
#include "params.h"

/* Copied from scheduler-close.c
    Will be modified to work as Fixed Service: Bank Triple Alteration (FSBTA)
   scheduler.
*/

extern long long int CYCLE_VAL;

/* A data structure to see if a bank is a candidate for precharge. */
int recent_colacc[MAX_NUM_CHANNELS][MAX_NUM_RANKS][MAX_NUM_BANKS];

/* Keeping track of how many preemptive precharges are performed. */
long long int num_aggr_precharge = 0;

void init_scheduler_vars() {
  // initialize all scheduler variables here
  int i, j, k;
  for (i = 0; i < MAX_NUM_CHANNELS; i++) {
    for (j = 0; j < MAX_NUM_RANKS; j++) {
      for (k = 0; k < MAX_NUM_BANKS; k++) {
        recent_colacc[i][j][k] = 0;
      }
    }
  }

  for (i = 0; i < MAX_NUM_CHANNELS; i++) {
    channel_fs_data[i].last_req_issue_cycle = 0;
    channel_fs_data[i].domain_turn = 0;
    channel_fs_data[i].bank_turn = 0;
    channel_fs_data[i].domain_zero_starter = 0;
    channel_fs_data[i].deadtime = 0;
    for (j = 0; j < DOMAIN_COUNT; j++) {
      for (k = 0; k < 100; k++) {
        optype_t next_op = READ;
        if (k % 7 == 0) {
          next_op = WRITE;
        }
        channel_fs_data[i].buffer[j][k] = next_op;
      }
    }
    channel_fs_data[i].domain_reads = NULL;
    channel_fs_data[i].domain_writes = NULL;
    channel_fs_data[i].pointer = 0;
  }

  //set security policy and deadtime
  SECURED = 1;
  DEADTIME = 15;

  return;
}

int domain_write_size(int channel) {
  int size = 0;
  opt_queue_t *rq_ptr = NULL;
  LL_FOREACH(channel_fs_data[channel].domain_writes, rq_ptr) {size++;}
  return size;
}

int domain_read_size(int channel) {
  int size = 0;
  opt_queue_t *rq_ptr = NULL;
  LL_FOREACH(channel_fs_data[channel].domain_reads, rq_ptr) {size++;}
  return size;
}

void schedule(int channel) {
  request_t *rq_ptr = NULL;

  long long last_req_issue_cycle = channel_fs_data[channel].last_req_issue_cycle;
  int bank_turn = channel_fs_data[channel].bank_turn;
  int domain_zero_starter = channel_fs_data[channel].domain_zero_starter;
  int deadtime = channel_fs_data[channel].deadtime;

  //pointer stuff
  //if the lists are empyty restart!
  if(domain_write_size(channel) == 0 && domain_read_size(channel) == 0){
    int pointer = channel_fs_data[channel].pointer;
    channel_fs_data[channel].pointer = (pointer + 3) % 100; //YOOOOOO DO THIWS! to make sure we are not stuck in a loop (since its bta! trippppple)
    for(int domain = 0; domain < DOMAIN_COUNT; domain++){
      opt_queue_t *d_num = malloc(sizeof(opt_queue_t));
      d_num->domain_id = domain;
      d_num->next = NULL;

      if (channel_fs_data[channel].buffer[domain][pointer] == READ){
        LL_APPEND(channel_fs_data[channel].domain_reads, d_num);
      }
      else if (channel_fs_data[channel].buffer[domain][pointer] == WRITE){
        LL_APPEND(channel_fs_data[channel].domain_writes, d_num);
      }
    }
  }

  //are we in write mode?
  int write_mode = 1;
  if(domain_read_size(channel) != 0){
    write_mode = 0;
  }

  int domain_turn;
  if(write_mode){
    domain_turn = channel_fs_data[channel].domain_writes->domain_id;
  } else {
    domain_turn = channel_fs_data[channel].domain_reads->domain_id;
  }

  //if it is the deadtime cycle, send a ready act to bank_turn
  if (CYCLE_VAL - last_req_issue_cycle >= deadtime){
    //if bank refreshing 
    int needs_fake = 1;
    LL_FOREACH(domain_queues[channel][domain_turn], rq_ptr){
      if (rq_ptr->command_issuable && (rq_ptr->next_command == ACT_CMD) && 
          rq_ptr->dram_addr.bank % 3 == bank_turn && 
          rq_ptr->operation_type == write_mode){ //op must match domains "mode" determined by buffer
        issue_request_command(rq_ptr);
        needs_fake = 0;
        break;
      }
    }

    //remove READ or WRITE from mode queue, regardless of if it was issued
    if (write_mode){
      opt_queue_t *temp = NULL;
      opt_queue_t *rq_ptr = NULL;
      LL_FOREACH_SAFE(channel_fs_data[channel].domain_writes, rq_ptr, temp){
        if (rq_ptr->domain_id == domain_turn){
          LL_DELETE(channel_fs_data[channel].domain_writes, rq_ptr);
          free(rq_ptr);
          break;
        }
      }
    }else{
      opt_queue_t *temp = NULL;
      opt_queue_t *rq_ptr = NULL;
      LL_FOREACH_SAFE(channel_fs_data[channel].domain_reads, rq_ptr, temp){
        if (rq_ptr->domain_id == domain_turn){
          LL_DELETE(channel_fs_data[channel].domain_reads, rq_ptr);
          free(rq_ptr);
          break;
        }
      }
    }
    
    if (needs_fake == 1){
      //send fake req to bank_turn
    }
    last_req_issue_cycle = CYCLE_VAL;
    //this is just making sure that if we are starting from
    //domain 0, we will start from a different bank to
    //make sure all domains can touch all banks!
    if (domain_turn == DOMAIN_COUNT - 1) {
        bank_turn = (domain_zero_starter + 2) % 3;
        domain_zero_starter = (domain_zero_starter + 2) % 3;
    } else {
        bank_turn = (bank_turn + 1) % 3;
    }
    domain_turn = (domain_turn + 1) % DOMAIN_COUNT;

    //check if next request is read or write
    //deadtime is based on last and next op to have const data...
    if (write_mode){
      if(domain_write_size(channel) != 0){ //staying in write mode
        deadtime = 7;
      }
      else{ //switching to read mode
        deadtime = 1 + 14; //larger delay as we are going from read to write
      }
    }
    else{
                                        //we gonna stay in read mode cos there aint no writes, and we do reads first!
      if (domain_read_size(channel) != 0 || domain_write_size(channel) == 0){ //staying in read mode
        deadtime = 7;  //its pretty unlikely that we will skip a write mode then go into another write mode, but TBD
      }
      else{ //switching to write mode
        deadtime = 13;
      }
    }
  }
  //continue on man!
  else {
    for(int domain = 0; domain < DOMAIN_COUNT; domain++){
      request_t *rq_ptr = NULL;
      int issued = 0;
      LL_FOREACH(domain_queues[channel][domain], rq_ptr){
        if (rq_ptr->command_issuable && rq_ptr->next_command != ACT_CMD){
          if (rq_ptr->next_command == COL_READ_CMD || rq_ptr->next_command == COL_WRITE_CMD){
            recent_colacc[channel][rq_ptr->dram_addr.rank]
                        [rq_ptr->dram_addr.bank] = 1; //note to close it!
          }
          issue_request_command(rq_ptr);
          issued = 1;
          break;
        }
      }
      if (issued == 1){
        break;
      }
    }
  }

  if (!command_issued_current_cycle[channel]) {
    for (int i = 0; i < NUM_RANKS; i++) {
      for (int j = 0; j < NUM_BANKS; j++) { /* For all banks on the channel.. */
        if (recent_colacc[channel][i][j]) { /* See if this bank is a candidate. */
            if (issue_precharge_command(channel, i, j)) {
              num_aggr_precharge++;
              recent_colacc[channel][i][j] = 0;
            }
        }
      }
    }
  }
  //update the scheduler variables
  channel_fs_data[channel].last_req_issue_cycle = last_req_issue_cycle;
  channel_fs_data[channel].domain_turn = domain_turn;
  channel_fs_data[channel].bank_turn = bank_turn;
  channel_fs_data[channel].domain_zero_starter = domain_zero_starter;
  channel_fs_data[channel].deadtime = deadtime;
}

void scheduler_stats() {
  /* Nothing to print for now. */
  printf("Number of aggressive precharges: %lld\n", num_aggr_precharge);
}

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
  }

  //set security policy and deadtime
  SECURED = 1;
  DEADTIME = 15;

  return;
}

void schedule(int channel) {
  request_t *rq_ptr = NULL;

  long long last_req_issue_cycle = channel_fs_data[channel].last_req_issue_cycle;
  int domain_turn = channel_fs_data[channel].domain_turn;
  int bank_turn = channel_fs_data[channel].bank_turn;
  int domain_zero_starter = channel_fs_data[channel].domain_zero_starter;

  //if it is the deadtime cycle, send a ready act to bank_turn
  if (CYCLE_VAL - last_req_issue_cycle >= DEADTIME || CYCLE_VAL == 0){
    //if bank refreshing 
    int needs_fake = 1;
    LL_FOREACH(domain_queues[channel][domain_turn], rq_ptr){
      //if issuable, is an act, matches bank, and is deadtime
      //must send an act to bank_turn
      if (rq_ptr->command_issuable && (rq_ptr->next_command == ACT_CMD) && rq_ptr->dram_addr.bank % 3 == bank_turn){
        issue_request_command(rq_ptr);
        needs_fake = 0;
        break;
      }
    }
    if (needs_fake == 1){
      //send fake req to bank_turn
    }
    last_req_issue_cycle = CYCLE_VAL;
    if (domain_turn == DOMAIN_COUNT - 1) {
        bank_turn = (domain_zero_starter + 2) % 3;
        domain_zero_starter = (domain_zero_starter + 2) % 3;
    } else {
        bank_turn = (bank_turn + 1) % 3;
    }
    domain_turn = (domain_turn + 1) % DOMAIN_COUNT;
  }
  //continue on man!
  else {
    int prev_domain_turn = domain_turn == 0 ? DOMAIN_COUNT - 1 : domain_turn - 1;
    LL_FOREACH(domain_queues[channel][prev_domain_turn], rq_ptr){
      if (rq_ptr->command_issuable && rq_ptr->next_command != ACT_CMD){
        if (rq_ptr->next_command == COL_READ_CMD || rq_ptr->next_command == COL_WRITE_CMD){
          recent_colacc[channel][rq_ptr->dram_addr.rank]
                       [rq_ptr->dram_addr.bank] = 1;
        }
        issue_request_command(rq_ptr);
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
}

void scheduler_stats() {
  /* Nothing to print for now. */
  printf("Number of aggressive precharges: %lld\n", num_aggr_precharge);
}

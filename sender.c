#include "fr_util.h"
#include "tx_array.h"


//***********************************************
// Send_bit: Function executed to send each bit.
//***********************************************

/*
 * Sync at the beginning of a window of length config->sync_time_mask.
 * Then, for a clock length of config->tx_interval,
 * - Send a bit 1 to the receiver by repeatedly accessing an address.
 * - Send a bit 0 by doing nothing
 */


uint64_t send_count = 0;
uint64_t s_bits[200];
uint64_t s_start_t[200];
uint64_t s_end_t[200];


void send_bit(bool one, struct config *config){
//printf("in send_bit\n");
  ADDR_PTR addr = config->addr;
  uint64_t interval = config->tx_interval;

//printf("call cc_sync\n");
  // Synchronize with receiver
  CYCLES start_t = cc_sync(config->sync_time_mask, config->sync_jitter);
//printf("cc_sync returned\n");

  
  //TODO:
  // If 1: Load repeatedly (using maccess()) for the length of the config->tx_interval.
  // If 0: Do nothing for the length of the config->tx_interval.

  uint64_t t_end = start_t + interval;
  t_end = (t_end / interval ) * interval;
      //TODO while rdtsc is < end of tx_interval, 1: keep accessing bit 0: do nothing
  while (rdtscp() < t_end) {
      if (one) {
	//printf("call maccess\n");
          maccess(addr);
	//printf("maccess returned\n");
      }
      else {
      }
  }
  if (SYNC_DBG) {
      s_bits[send_count] = one;
      s_start_t[send_count] = start_t & (0xFFF0000);
      s_end_t[send_count] = rdtscp() & (0xFFF0000);
      send_count++;
      if (send_count > 200) printf("detection count over 200\n");
  }
  return;

}

//---------------------------------------------------------------------------


int main(int argc, char **argv)
{
    // Initialize config and local variables
    struct config config;
    init_config(&config, argc, argv);
    int sending = 1;

    puts("-------------------");
    puts("Flush Reload Covert Channel Sender CS7292 Asst-2 v1.0");
    puts("-------------------\n");   
    printf("sync_time_mask : %#lx cycles // Sync time window for each bit\n",config.sync_time_mask);
    printf("tx_interval    : %#lx cycles // Interval for tranmission within each sync time window\n",config.tx_interval);
   
    //Sleep a bit, to ensure receiver is ready.
    usleep(1000*100);

    //-------------------------
    // Start Sending Bits
    //-------------------------

    while (sending) {
      // DEBUG: Send infinite constant or alternating values.
      /* for (int i = 0; i < 100000000; i++) { */
      /* send_bit(true, &config); */
      // OR
      /* send_bit(i % 2 == 0, &config); */
      /* } */
      
      // Get a message to send from the user
      char text_buf[128] ;
      strncpy(text_buf, tx_string, strlen(tx_string)+1);
      // Convert that message to binary
      char *msg = string_to_binary(text_buf);
        
      // Send '10101011' bit sequence tell the receiver
      // a message is going to be sent
      for (int i = 0; i < 6; i++) {
        send_bit(i % 2 == 0, &config);
      }
      send_bit(true, &config);
      send_bit(true, &config);

      //-------------------------
      // Start Sending Payload
      //-------------------------

      // Send the message bit by bit
      size_t msg_len = strlen(msg);
      for (int ind = 0; ind < msg_len; ind++) {
	send_bit(msg[ind] - '0', &config); 
      }

      printf("%s\n",text_buf);
      sending = 0;
    }
 
    printf("Sender finished\n");
/////debug
    if (SYNC_DBG) {

        FILE* fptr = fopen("SENDER_dbg.txt", "w");
        fprintf(fptr, "SENDER done. index, start_t, end_t, start_t delta from rev, value\n");
        for (int i = 0; i < send_count - 8; i++) {
            int j = i - 1;
            if (i == 0) j = 0;
            //fprintf(fptr,"%d, %lx, %lx, %ld\n", i, s_start_t[i+8], s_end_t[i+8], s_bits[i+8]);
            fprintf(fptr, "%d, %lx, %lx, %lx, %ld\n", i, s_start_t[i + 8], s_end_t[i + 8], s_start_t[i + 8] - s_start_t[i + 7], s_bits[i + 8]);
        }
        fclose(fptr);
    }

/////debug


    return 0;
}












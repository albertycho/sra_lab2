#include "fr_util.h"


//----------------------------------------------------------------
// PRIMITIVES FOR FLUSH+RELOAD CHANNEL
//----------------------------------------------------------------

/* Flush a cache block of address "addr" */
extern inline __attribute__((always_inline))
void clflush(ADDR_PTR addr)
{
	//printf("before calling clflushasm\n");
  //TODO: Use clflush instruction.
  asm volatile ("clflush (%0)"
		: /*output*/
		: /*input*/ "r"(addr)
		: /*clobbers*/ );
	//printf("after calling clflushasm\n");
}


/* Load address "addr" */
void maccess(ADDR_PTR addr)
{
	//printf("addr: %lx\n",addr);
	//printf("before calling memaccess_asm\n");
    uint64_t dummy;
  //TODO: Use mov instruction.
  //asm volatile("movq %1, (%0)"
  asm volatile("movq (%%rdi), %%rsi"
	       //: /*output*/ "=r" (dummy)
	       :
	       : /*input*/ "D"(addr)
	       //: /*clobbers*/  );
	       : /*clobbers*/ "%rsi" );
	//printf("after calling memaccess_asm\n");
  
  return;
}


/* Loads addr and measure the access time */
CYCLES maccess_t(ADDR_PTR addr)
{
  CYCLES cycles;
  



  ///////////Working on single asm volatile version
  asm volatile (
		"rdtscp\n"
		"shl $32,%%rdx\n"
		"or %%rdx, %%rax\n"
		"movq %%rax, %%rbx\n"//starttime in rbx
		"movq (%%rdi), %%rsi\n"//maccess
		"rdtscp\n"
                "shl $32,%%rdx\n"
    	        "or %%rdx, %%rax\n"
		"subq %%rbx, %%rax\n"//rax=endtime-startime=rax-rbx
		: "=a" (cycles)
		: "D" (addr)
		: "%rsi");
	
  return cycles;
//////above works 	



  
////////////////////////Working version below

  CYCLES cycle1, cycle2;
  // TODO:
  // Use a mov instruction to load an address "addr",
  // which is sandwiched between two rdtscp instructions.
  // Calculate the latency using difference of the output of two rdtscps.

  asm volatile ("rdtscp\n"
        	"shl $32,%%rdx\n"
        	"or %%rdx, %%rax\n"		      
        	: /* outputs */ "=a" (cycle1));

  //asm volatile("movq %%rsi, (%0)"
  //asm volatile("movq (%0), %%rsi" // CONFIRMED TO BE CORRECT ORDER
  asm volatile("movq (%%rcx), %%rsi" // CONFIRMED TO BE CORRECT ORDER
               //: /*output*/ "=r" (dummy)
               :
               //: /*input*/ "r"(addr)
               : /*input*/ "c"(addr)
               //: /*clobbers*/  );
               : /*clobbers*/ "%rsi" );


  asm volatile ("rdtscp\n"
        	"shl $32,%%rdx\n"
        	"or %%rdx, %%rax\n"
        	"subq %%rbx, %%rax" // this confiremd to be the right order of src dest
        	: /* outputs */ "=a" (cycle2)
        	: "b" (cycle1));

  return cycle2;

	/////////////CODE ABOVE WORKS, but trying to put it in one asm func



	
//  asm volatile("rdtscp\n"
//            "shl $32,%%rdx\n"
//            "or %%rdx, %%rax\n"
//            "movq %%rax, %%rbx\n"
//            //do the mem access
//            "movq %%rsi, (%%rcx)\n"
//            //get finishtime 
//            "rdtscp\n"
//            "shl $32,%%rdx\n"
//            "or %%rdx, %%rax\n"
//            "subq %%rbx, %%rax\n"      
//	       : /*output*/ "=a"(cycles)
//	       : /*input*/  "c"(addr)
//	       : /*clobbers*/ "%rsi");
	//printf("after calling memaccess_t_asm\n");
 //return cycles;
//  return cycle2;
}


/* Returns Time Stamp Counter (using rdtscp function)*/
extern inline __attribute__((always_inline))
uint64_t rdtscp(void) {
  uint64_t cycles;
  asm volatile ("rdtscp\n"
		"shl $32,%%rdx\n"
		"or %%rdx, %%rax\n"		      
		: /* outputs */ "=a" (cycles));
  return cycles;
}

/* Synchronize sender and receiver for each bit 
   We will use a counter that counts up to SYNC_TIME_MASK)
 * -- Create the counter by masking lower bits of rdtscp using SYNC_TIME_MASK
 * -- Spin until the counter overflows and becomes less than SYNC_JITTER
 */
extern inline __attribute__((always_inline))
CYCLES cc_sync(uint64_t SYNC_TIME_MASK, uint64_t SYNC_JITTER) {
  CYCLES end_sync_cycle; //cycle where synchronization is done.
  //TODO:
  //Get the current counter value by masking lower bits of rdtscp using SYNC_TIME_MASK
  //Spin until current counter value overflows and is within SYNC_JITTER.
  //end_sync_cycle = rdtscp();
  while (1) {
      end_sync_cycle = rdtscp();
      if ((end_sync_cycle & SYNC_TIME_MASK) <= SYNC_JITTER) {
          return end_sync_cycle;
      }
      
  }
  return end_sync_cycle;
}


//----------------------------------------------------------------
// NO NEED TO CHANGE ANYTHING BELOW THIS LINE
//----------------------------------------------------------------

/*
 * Convert a given ASCII string to a binary string.
 * From:
 * https://stackoverflow.com/questions/41384262/convert-string-to-binary-in-c
 */
char *string_to_binary(char *s)
{
    if (s == NULL) return 0; /* no input string */
    size_t len = strlen(s) ;

    // Each char is one byte (8 bits) and + 1 at the end for null terminator
    char *binary = malloc(len * 8 + 1);
    binary[0] = '\0';
	
    for (size_t i = 0; i < len; ++i) {
        char ch = s[i];
        for (int j = 7; j >= 0; --j) {
            if (ch & (1 << j)) {
                strcat(binary, "1");
            } else {
                strcat(binary, "0");
            }
        }
    }    
    return binary;
}

/*
 * Convert 8 bit data stream into character and return
 */
char *conv_char(char *data, int size, char *msg)
{
    for (int i = 0; i < size; i++) {
        char tmp[8];
        int k = 0;

        for (int j = i * 8; j < ((i + 1) * 8); j++) {
            tmp[k++] = data[j];
        }

        char tm = strtol(tmp, 0, 2);
        msg[i] = tm;
    }

    msg[size] = '\0';
    return msg;
}

/*
 * Prints help menu
 */
void print_help() {

	printf("-f,\tFile to be shared between sender/receiver\n"
		"-o,\tSelected offset into shared file\n"
		"-s,\tTime period on which sender and receiver sync on each bit\n"
	       	"-i,\tTime interval for sending a single bit within a sync period\n");

}

/*
 * Parses the arguments and flags of the program and initializes the struct config
 * with those parameters (or the default ones if no custom flags are given).
 */
void init_config(struct config *config, int argc, char **argv)
{
  // Initialize default config parameters
  int offset = DEFAULT_FILE_OFFSET;
  config->tx_interval      = TX_INTERVAL_DEF;
  config->sync_time_mask   = SYNC_TIME_MASK_DEF;
  config->sync_jitter      = SYNC_JITTER_DEF;
    
  char *filename = DEFAULT_FILE_NAME;


  // Parse the command line flags
  //      -f is used to specify the shared file 
  //      -o is used to specify the shared file offset
  //      -s is used to specify the sync_time mask
  //      -i is used to specify the sending interval rate
  
  int option;
  while ((option = getopt(argc, argv, "i:s:o:f:")) != -1) {
    switch (option) {
    case 'i':
      config->tx_interval     = atoi(optarg);
      break;
    case 's':
      config->sync_time_mask = atoi(optarg);
      break;        
    case 'o':
      offset = atoi(optarg)*CACHE_BLOCK_SIZE;
      break;
    case 'f':
      filename = optarg;
      break;
    case 'h':
      print_help();
      exit(1);
    case '?':
      fprintf(stderr, "Unknown option character\n");
      print_help();
      exit(1);
    default:
        print_help();
        exit(1);
    }
  }

  // Map file to virtual memory and extract the address at the file offset
  if (filename != NULL) {
    int inFile = open(filename, O_RDONLY);
    if(inFile == -1) {
      printf("Failed to Open File\n");
      exit(1);
    }

    void *mapaddr = mmap(NULL,DEFAULT_FILE_SIZE,PROT_READ,MAP_SHARED,inFile,0);

    if (mapaddr == (void*) -1 ) {
      printf("Failed to Map Address\n");
      exit(1);
    }

    config->addr = (ADDR_PTR) mapaddr + offset;
  }
}


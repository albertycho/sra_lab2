# CS7292 Lab-2: Flush+Reload Covert Channel

In this assignment, you will learn to construct a cross-process Flush+Reload covert channel. You will construct such a channel on your own laptop or desktop machine.

## Requirements:
* **Linux operating system** (Ubuntu or Fedora is preferred, but any other linux based OS will also work).
  - Note this assignment has not been tested on WSL or in a Linux-VM. The attack itself may work, but you may have trouble with debugging & reproducibility, as you run into complications while pinning a process to a core, or setting the processor frequency to a constant value for stable bit-rate calculations.
* **x86 CPU** (Intel preferred, but in-principle AMD should also work).  
* **GCC compiler** (the code was tested on gcc versions 5.4 and 8.4, but newer versions should also work).
* **sudo access** needed to fix the processor frequency to a stable value (needed for stable bit-rate calculations).


## Task-1: Histogram of Cache Hit/Miss Latency. (4 points)


This task will involve calibration to identify the Latency of LLC-Misses on your system. In this task, you will create a histogram of the latency of cache hits and cache misses, within a single program. You will do this by executing 10,000 LLC-Misses and 10,000 L1-Cache hits and measuring the latency each access. Using the results, you will create and submit a histogram plot in your report. The started code for generating the histogram is provided in `histogram.c`.

You will need to modify the following functions in `fr_util.c`:
- Function to Flush an address from the cache: `clflush()`
- Function to Load an address: `maccess()`.
- Function to Load an address and measure its latency: `maccess_t()`.
  
Subsequently you will call these functions in `histogram.c` to obtain the latencies for cache hits and misses.

For both `fr_util.c` and `histogram.c`, detailed step-by-step comments (`// TODO`) are provided inline in the code. The sample histogram results are provided in `sample_task1_histogram.out`.

For `fr_util.c` functions, (flush, load, load and measure latency), you will need to use GCC extended assembly statements (*Extended Asm*). With these statements, you directly provide the GCC compiler the assembly instructions to be inserted into the binary, and thus you have more control over the timing of the instructions that execute unlike writing in C/C++. You only need to write 4-5 instructions in assembly, so do not get overwhelmed if you havent done this before. A few resources on writing Extended ASM statements in GCC are: [GCC Inline Assembly HOWTO](https://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html#s5), and [Fedora's How to Use Inline Assembly](https://dmalcolm.fedorapeople.org/gcc/2015-08-31/rst-experiment/how-to-use-inline-assembly-language-in-c-code.html). For reference on the x86 instructions you will need (e.g. clflush, rdtscp, mov and others), [felixcloutier.com](https://www.felixcloutier.com/x86/) is an excellent resource.

### Submission and Grading
You can run task-1 with `./run_task1.sh`, which will generate a `histogram.out`. In your submission tar file, you will submit the `histogram.out` file that is generated, when you run `run_task1.sh`. You will also plot the histogram and add it in your report. You will receive +4 if you are able to generate a suitable histogram.


## Task-2: Cross-Process Covert Channel using Flush+Reload. (4 points)
  
In this task, you will construct a cross-process covert channel. For this, we have provided you the starter code for the sender and receiver. The receiver is already programmed to wait for the sender to begin. Once the sender starts, it already sends a handshake signal to the receiver (using the covert channel you will construct) and then the sender transmits a message of 11 bytes. Once the receiver detects the handshake signal, it starts detecting the subsequent bits transmitted by the sender until the end of message is received. Finally, the receiver calculates the bit-rate and error-rate. You will only implement the low-level functions for synchronizing the sender and receiver each bit-period and sending and receiving each bit. Specifically, you will modify the following functions:
- Function for the sender and receiver to infer the start of each bit-period, i.e. `cc_sync()` in `fr_utils.h`.
- Function to send a bit, i.e. `send_bit()` in `sender.c`.
- Function to detect a bit, i.e. `detect_bit()` in `receiver.c`.

For all the functions in `fr_util.c`, `sender.c` and `receiver.c` detailed comments (`TODO`s) are provided inline in the code. The sample bit-rate and error-rate results are provided in `sample_task2_rec.out`. 

Note that for this task, you will have to set the  `CACHE_MISS_LATENCY` and `CPU_FREQ_MHZ` in `receiver.c`. See section on **Runtime Setup** below for how to obtain these values for your CPU.

### Construction of Flush+Reload Sender and Receiver
![Synchronization of Sender and Receiver and Transmission in Flush Reload](FlushReload.png?raw=true)

**Synchronization:** The sender and receiver both call the `cc_sync()` function to know when a new bit-period is beginning. Each bit-period is of `SYNC_TIME_MASK` cycles and `cc_sync()` will return within `SYNC_JITTER` cycles of the beginning of a subsequent bit-period. `cc_sync` is able to provide a shared notion of time for sender and receiver using `rdtscp` instruction (which samples the time-stamp counter of the processor). Then, both the sender and receiver will transmit/receive for the next `TX_INTERVAL` cycles.

**Transmission:** For sending a bit-0, the sender does not do anything for `TX_INTERVAL` cycles, and for a bit-1, the sender will keep accessing the shared address (available in the `config->addr` variable) over and over again. For the `TX_INTERVAL` cycles, the receiver keeps reloading the address while measuring its time and then flushes the address, waits for the sender to install once more, and then repeats. In a particular bit-period, the receiver counts the hits aand misses for the shared address. If hits are more in number, then the receiver detects a bit-value-1, otherwise if the misses are more in number, the receiver detects a bit-value-0. After `TX_INTERVAL` cycles complete, the sender and receiver call `cc_sync()` again to synchronize on the start of the next bit period.


### Submission and Grading
You can run task-2 with `./run_task2.sh`, which will generate a `rec.out` file (the results for 5 runs of the attack). You will submit the `rec.out`, all the source code in the submission tar file and also provide the bit-rate and error-rate results for the five runs generated in your report. Using the default channel parameters, you should be able to achieve at least 1 KB/s bit-rate with the error-rate <1\%. We will accept your results if you achieve these parameters for averaged over the best 3 out of 5 runs. 


## Task-3: Maximize Covert Channel Bit-Rate. (2 + Bonus points)

In this task, you will maximimze the cross-core covert-channel bit-rate while maintaining a low error-rate (less than 5%). You are free to modify the covert channel in Task-2 as you wish, or use a different flush-based covert channel of your choice. One easy approach to increase increase the bit-rate of the channel to atleast 20KB/s (while keeping error-rate rate at 0%), is by varying the channel timing parameters `SYNC_TIME_MASK`, `SYNC_JITTER`, `TX_INTERVAL` in `fr_util.h` from Task-2 (the default parameters have considerable safety margins that you can tweak). Beyond 20-40KB/s, you will have to be more creative and possibly hand-code functions is assembly more carefully to avoid errors.

### Submission and Grading
This task is competitive. You will receive 2 points if you achieve 20KB/s at less than 5% error rate (averaged over the best 3 out of 5 runs). The top 3 places based on bit-rates (with error-rate below 5%) will receive +3,+2,+1 bonus points respectively. You will submit a separate folder inside your submission tar file with all the files for the Task-3. In your report, you will include the bit-rate and error-rate of the best-3 runs, and the average, and also explain how you achieve this.

## Instructions To Run The Code:
### Compile
`make all`


### For Students Without Linux/ x86 Systems

For the students who do not have access to a Linux x86 system, any of the servers rover1.cc.gatech.edu to rover5.cc.gatech.edu should work. They all have a stable operating frequency (you can check with `cat /proc/cpuinfo | grep -i Mhz`), so you don’t need to use cpupower or sudo to influence DVFS. Each of these machines have 4 cores, so if you find some cores busy, you can try taskset with another core number or try another machine for better isolation, or try again (the sender & receiver take up less than 0.1s), or just start early.

I would not recommend the ECE server systems as they seem to be heavily loaded and also do not have stable frequencies. So you will not get reproducible results.

Lastly, please make sure to kill receiver processes that become zombies when the associated sender process completes without completing a handshake successfuly (such receivers may wait for senders forever and eat up resources). They may also hinder another student’s experiments if left running. Please be a good citizen and kill your zombies.

### Runtime Setup
* **Command-line tool *cpupower*** :
  - On Fedora, this can typically be installed with `sudo dnf install kernel-tools`
  - On Ubuntu, this can typically be installed with `sudo apt-get install linux-tools-<KERNEL-VERSION>-generic`

* **Before Task-1: Set CPU Frequency to a Stable Value** (ensures stable measurements).
  - First, check the default frequency governor set in the current policy: `cpupower frequency-info | grep governor`
  - Set the frequency governer to "performance":  `sudo cpupower frequency-set -g performance`
  - Read the frequency and make sure it is relatively stable: `for i in 1 2 3 4 5; do cpupower frequency-info | grep "current CPU frequency"; done`
  - Note the average frequency in MHz and set it in CPU_FREQ_MHZ in `receiver.c`.
	    
* **Before Task-2: Set the `CACHE_MISS_LATENCY` threshold** in `receiver.c`. You can get this threshold by subtracting around 50 cycles from the minimum latency at which MISSES start to appear in Task-1 results.

### Run
* Task-1: `./run_task1.sh` will run `taskset -c 0 ./histogram`, which will pin the histogram program to Core-0 and run it. 
* Task-2: `./run_task2.sh` will pin the receiver and sender to Core-0 and Core-2 respectively and run the covert-channel 5 times. You can individually run one iteration of the covert-channel by running the `taskset -c 0 ./receiver` and then in another terminal  `taskset -c 2 ./sender`.

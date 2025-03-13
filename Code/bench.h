#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// need this for multithreading
#include <pthread.h> 

#define KSIZE (16)
#define VSIZE (1000)

#define LINE "+-----------------------------+----------------+------------------------------+-------------------+\n"
#define LINE1 "---------------------------------------------------------------------------------------------------\n"

// max threads of our machine
#define available_threads 16

long long get_ustime_sec(void);
void _random_key(char *key, int length);

// function that reads without using any threads
void read_without_threads(long int count, int r);

// function that writes without using any threads
void write_without_threads(long int count, int r);

// function that gets called to the function thread_maker for write purposes
void* _write_test(void* arg);

// function that gets called to the function thread_maker for read purposes
void* _read_test(void* arg);

// function that creates threads for each operation
void thread_maker(char* operation, long int count, int num_threads);

// function that is capable of simultaneous reads and writes using percentages
void _readwrite_test(long int count, char* operation, int num_threads, int writePercentage);

// this struct initializes elements on each thread
typedef struct {

    // starting point of the first thread
    long int start;
    
    // final destination of the first thread
    long int end;
    
    // number of operations
    long int count;
    
    // random flag
    int r;
    
    // number of threads
    int num_threads;
    
    // name of operation
    char operation[10];
    
    // write percentage
    int write_percentage;
    
    // thread tid
    pthread_t tid;

} final;

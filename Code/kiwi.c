#include <string.h>
#include "../engine/db.h"
#include "../engine/variant.h"
#include "bench.h"

#define DATAS ("testdb")

// mutex initializer
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

// number of current readers
int readers = 0; 

// number of current writers
int writers = 0;

// number of pending writers
int pending_writers = 0; 

// mutex for writers
pthread_mutex_t mwrite = PTHREAD_MUTEX_INITIALIZER; 

// mutex for readers
pthread_mutex_t mread = PTHREAD_MUTEX_INITIALIZER;

// condition variable for readers
pthread_cond_t can_read = PTHREAD_COND_INITIALIZER;

// condition variable for writers 
pthread_cond_t can_write = PTHREAD_COND_INITIALIZER; 

//
pthread_mutex_t writeStatsToFile = PTHREAD_MUTEX_INITIALIZER;

//
FILE *fp;

// function that writes without using any threads
void write_without_threads(long int count, int r) {
	
	int i;
	double cost;
	long long start, end;
	Variant sk, sv;
	DB* db;

	char key[KSIZE + 1];
	char val[VSIZE + 1];
	char sbuf[1024];
    
	memset(key, 0, KSIZE + 1);
	memset(val, 0, VSIZE + 1);
	memset(sbuf,0,1024);
	
	db = db_open(DATAS);
	start = get_ustime_sec(); 
	
	for (i = 0; i < count; i++) {
	        if (r) {
            		_random_key(key, KSIZE);
        	} else {
            		snprintf(key, KSIZE, "key-%d", i);
		}

        	fprintf(stderr,"%d adding %s \n", i, key);
        	snprintf(val, VSIZE, "val-%d", i);

        	sk.length = KSIZE;
        	sk.mem = key;
        	sv.length = VSIZE;
        	sv.mem = val;

        	db_add(db, &sk, &sv);
    
		if ((i % 10000) == 0) {
		    	fprintf(stderr, "random write finished %d ops\n", i);
		    	fflush(stderr);
		}
	}

	db_close(db);
	
	end = get_ustime_sec();
	cost = end - start;

	printf(LINE);
	printf("|Random-Write (done:%ld): %.6f sec/op; %.1f writes/sec(estimated); cost:%.3f(sec);\n",
	        count, (double)(cost / count),
	        (double)(count / cost),
	        cost);
}



// function that reads without using any threads
void read_without_threads(long int count, int r) {
	
	int i;
	int ret;
	int found = 0;
	double cost;
	long long start, end;
	Variant sk, sv;
	DB* db;
	char key[KSIZE + 1];

	db = db_open(DATAS);
	start = get_ustime_sec();

	for (i = 0; i < count; i++) {
        
        	memset(key, 0, KSIZE + 1);

		if (r) {
			_random_key(key, KSIZE);
		} else {
			snprintf(key, KSIZE, "key-%d", i);
		}
		
		fprintf(stderr,"%d searching %s\n", i, key);
		sk.length = KSIZE;
		sk.mem = key;
		ret = db_get(db, &sk, &sv);

		if (ret) {
		    found++;
		} else {
		    INFO("not found key#%s", sk.mem);
		}

		if ((i % 10000) == 0) {
		    fprintf(stderr, "random read finished %d ops\n", i);
		    fflush(stderr);
		}
	}

	db_close(db);

	end = get_ustime_sec();
	cost = end - start;

	printf(LINE);
	printf("|Random-Read (done:%ld, found:%d): %.6f sec/op; %.1f reads /sec(estimated); cost:%.3f(sec)\n",
	       count, found,
	       (double)(cost / count),
	       (double)(count / cost),
		    cost);
}



// function that gets called to the function thread_maker for write purposes
void* _write_test(void * arg) {

    // creates a pointer of struct final type
	final* args = (final*) arg ;

    // initializes struct variables
	long int count = args->count;
	int r = args->r;

    // locking for write access
	pthread_mutex_lock(&mwrite); 
    
    // increment pending writers counter
	pending_writers++; 
	
	while (readers > 0 || writers > 0) {
		// wait until no readers or writers
		pthread_cond_wait(&can_write, &mwrite); 
	}

    // decrement pending writers counter
	pending_writers--;

    // increment writers counter
	writers++;

    // unlocking after write access
	pthread_mutex_unlock(&mwrite); 
	
	int i;
	double cost;
	long long start, end;
	Variant sk, sv;
	DB* db;

	char key[KSIZE + 1];
	char val[VSIZE + 1];
	char sbuf[1024];
    
	memset(key, 0, KSIZE + 1);
	memset(val, 0, VSIZE + 1);
	memset(sbuf,0,1024);
	
	db = db_open(DATAS);
	start = get_ustime_sec();

	for (i = 0; i < count; i++) {
		if (r) {            	
			_random_key(key, KSIZE);
		} else {
			snprintf(key, KSIZE, "key-%d", i);
		}
		    		
		fprintf(stderr,"%d adding %s \n",i,key);
		snprintf(val, VSIZE, "val-%d", i);

		sk.length = KSIZE;
		sk.mem = key;
		sv.length = VSIZE;
		sv.mem = val;
			
		// locking before write access
		pthread_mutex_lock(&mwrite);

		db_add(db, &sk, &sv);

		// unlocking after write access
		pthread_mutex_unlock(&mwrite);

		if ((i % 10000) == 0) {
		    fprintf(stderr, "random write finished %d ops\n", i);
		    fflush(stderr);
		}
	}

	db_close(db);

	end = get_ustime_sec();
	cost = end - start;

    // locking before modifying shared variables
	pthread_mutex_lock(&mwrite); 
	
    // decrement writers counter
	writers--;

    // if there are pending writers
	if (pending_writers > 0) {
        // signal one pending writer
	    pthread_cond_signal(&can_write); 
    } else { 
        // if no pending writers, signal readers
	    pthread_cond_broadcast(&can_read);
    }

    // unlocking after modifying shared variables
	pthread_mutex_unlock(&mwrite); 
	
	printf(LINE);

	pthread_mutex_lock(&writeStatsToFile); 
	fp = fopen("output.txt","a+");
	long int threadTotal = args->end-args->start;	
	fprintf(fp, "Thread %ld completed execution, %ld writes: %.6f sec/op; %.1f writes/sec(estimated); cost:%.3f(sec);\n"
		,args->tid, threadTotal, (double) (cost/threadTotal), (double) (threadTotal/cost), cost);
	fclose(fp);
	pthread_mutex_unlock(&writeStatsToFile); 

	printf("|Random-Write (done:%ld): %.6f sec/op; %.1f writes/sec(estimated); cost:%.3f(sec);\n",
	        count, (double)(cost / count),
	        (double)(count / cost),
	        cost);

	return NULL;
}



// function that gets called to the function thread_maker for read purposes
void* _read_test(void * arg) {

	// creates a pointer of struct final type
	final* args = (final*) arg ;

    // initializes struct variables
	long int count = args->count;
	int r = args->r; 
    
    // locking for read access
	pthread_mutex_lock(&mread);

	while (writers > 0 || pending_writers > 0) {
        // wait until no writers or pending writers
	    pthread_cond_wait(&can_read, &mread); 
	}

    // increment readers counter
	readers++;

    // unlocking after read access
	pthread_mutex_unlock(&mread); 

	int i;
	int ret;
	int found = 0;
	double cost;
	long long start, end;
	Variant sk, sv;
	DB* db;
	char key[KSIZE + 1];

	db = db_open(DATAS);
	start = get_ustime_sec();

	for (i = 0; i < count; i++) {
		memset(key, 0, KSIZE + 1);

		if (r) {
			_random_key(key, KSIZE);
        	} else {
			snprintf(key, KSIZE, "key-%d", i);
	     	}

		fprintf(stderr,"%d searching %s\n", i, key);
		sk.length = KSIZE;
		sk.mem = key;
		
		// locking before read access
		pthread_mutex_lock(&mread);

		ret = db_get(db, &sk, &sv);

		// unlocking after read access
		pthread_mutex_unlock(&mread);

		if (ret) {
		    found++;
		} else {
		    INFO("not found key#%s", sk.mem);
		}
	

		if ((i % 10000) == 0) {
		    fprintf(stderr, "random read finished %d ops\n", i);
		    fflush(stderr);
		}
	}

	db_close(db);

	end = get_ustime_sec();
	cost = end - start;

	pthread_mutex_lock(&mread); // Locking before modifying shared variables

	readers--; // Decrement readers counter

	if (readers == 0) // If no more readers
	        pthread_cond_signal(&can_write); // Signal a pending writer
	pthread_mutex_unlock(&mread); // Unlocking after modifying shared variables

	printf(LINE);
	
	pthread_mutex_lock(&writeStatsToFile); 

	fp = fopen("output.txt","a+");
	long int threadTotal = args->end-args->start;	

	fprintf(fp, "Thread %ld completed execution, found %d/%ld reads: %.6f sec/op; %.1f reads/sec(estimated); cost:%.3f(sec);\n"
		,args->tid, found, threadTotal, (double) (cost/threadTotal), (double) (threadTotal/cost), cost);
	fclose(fp);

	pthread_mutex_unlock(&writeStatsToFile); 

	printf("|Random-Read (done:%ld, found:%d): %.6f sec/op; %.1f reads /sec(estimated); cost:%.3f(sec)\n",
	       count, found,
	       (double)(cost / count),
	       (double)(count / cost),
		    cost);
	
    	return NULL;
}



// function that creates threads for each operation 
void thread_maker(char* operation, long int count, int num_threads) {
       	
	// number of each thread operations
	long int nth = count / num_threads;		 

	// remainder of operations - added to last thread
    	int remain = count % num_threads;	   
	
	// starting point of the first thread
    	long int start = 0;
	
	// final destination of the first thread			 
   	long int end = start + nth;                

	// defining thread pointer - memory allocation for threads
	final* threads;                                                 
	threads = (final*)malloc(available_threads*sizeof(final));

	if (strcmp(operation, "write") == 0){	
		if (num_threads<=available_threads){		
			for (int i = 0; i < num_threads; i++){
			
				// passing count into threads[i]
				threads[i].count = count; 

	        		// passing r into threads[i]
				threads[i].r = 1; 		
				
				// passing start into threads[i]				
				threads[i].start = start;		
				
				// passing end into threads[i]
				threads[i].end = end;

				// passing num_threads into threads[i]		
				threads[i].num_threads = num_threads;

				// passing operation into threads[i]
				strcpy(threads[i].operation, "write");

				// passing write_percentage into threads[i]
				threads[i].write_percentage = 0;	
			
				// creating a new thread, calls _write_test function, with the parameters of threads
				pthread_create(&threads[i].tid, NULL, _write_test, (void *) &threads[i]);                 				
			
				// change start for the next thread
				start = end;                                                                        
				
				// if the next thread, is the last thread
				if (i == num_threads-2) {                                                                        
					end = end + nth + remain; 
				} else {                                                                                
				    end = end + nth;                                                            
				}                                                                                              
			}  
                                                                                         
			for (int i = 0; i < num_threads; i++) {                                                                
			    pthread_join(threads[i].tid, NULL);
			}
		
			free(threads);

		} else {
			fprintf(stderr,"Usage: db-bench (<write | read> <count> <random>) (<readwrite> <count> <writePercentage> <random>) \n");
			exit(1);
		}

	} else if (strcmp(operation, "read") == 0) {	
		if (num_threads <= available_threads) {		
			for (int i = 0; i < num_threads; i++) {

                // passing count into threads[i]
				threads[i].count = count; 																	

                // passing r into threads[i]
                threads[i].r = 1; 																			

                // passing start into threads[i]
				threads[i].start = start;														

                // passing end into threads[i]
				threads[i].end = end;

                // passing num_threads into threads[i]	
				threads[i].num_threads = num_threads;

                // passing operation into threads[i]
				strcpy(threads[i].operation,"read");

                // passing write_percentage into threads[i]
				threads[i].write_percentage = 0;

                // creating a new thread, calls _read_test function, with the parameters of threads
				pthread_create(&threads[i].tid, NULL, _read_test, (void *) &threads[i]);                 

                // change start for the next thread
				start = end;                                                                       

                // if the next thread, is the last thread
				if (i == num_threads-2) {                                                       
					end = end + nth + remain;                                               
				} else {                                                                    
					end = end + nth;                                                        
				}                                                                                               
			}    
                                                                                       
			for (int i = 0; i < num_threads; i++){                                                               
			    pthread_join(threads[i].tid, NULL);                                         
			}

			free(threads);
		} else {
			fprintf(stderr,"Usage: db-bench (<write | read> <count> <random>) (<readwrite> <count> <writePercentage> <random>) \n");
			exit(1);
		}
	}
}



// function that is capable of simultaneous reads and writes using percentages
void _readwrite_test(long int count, char* operation, int num_threads, int writePercentage) {
	
    // defining thread pointer - memory allocation for threads
    final* threads;
	threads=(final*)malloc(num_threads*sizeof(final));
	
    // number of write operations
    int num_write = count * writePercentage/100;
	
    // number of read operations
    int num_read = count-num_write;
	
    // number of operations for each thread
    long int nth = count / num_threads;
	
    // number of remaining operations to be added to the last thread
    int remain = count % num_threads;

    // char arrays to save the word 'readwrite'
    char read[5]; 
    char write[6];
    memcpy(read, operation, 4);
	read[4] = '\0';
	memcpy(write, operation + 4, 5);
	write[5] = '\0';
    
	// starting point of the first thread
    long int start = 0;

    // final destination of the first thread
    long int end = start + nth;


	for(int i = 0; i < num_threads; i++){

        // passing r into threads[i]														
		threads[i].r = 1; 										
		
        // passing start into threads[i]
        threads[i].start = start;								
		
        // passing end into threads[i]
        threads[i].end = end;
		
        // passing num_threads into threads[i]
        threads[i].num_threads = num_threads;
		
        // passing write_percentage into threads[i]
        threads[i].write_percentage = writePercentage;
		
        
        if (strcmp(write, "write") == 0) {

            // passing count into threads[i]
			threads[i].count = num_write; 
			
            // passing operation into threads[i]
			strcpy(threads[i].operation, "write");

            // creating a new thread, calls _write_test function, with the parameters of threads
			pthread_create(&threads[i].tid, NULL, _write_test, (void *) &threads[i]);

            // change start for the next thread
			start = end;

            // if the next thread, is the last thread
			if (i == num_threads-2) {                                                          
			    end = end + nth + remain;                                                
			} else {                                                                     
			    end = end + nth;                                                         
			}                                                                                              
			                                   
			
		}

		if (strcmp(read, "read") == 0) {
			
            // passing count into threads[i]
			threads[i].count = num_read;

            // passing operation into threads[i]
			strcpy(threads[i].operation, "read");

            // creating a new thread, calls _read_test function, with the parameters of threads
			pthread_create(&threads[i].tid, NULL, _read_test, (void *) &threads[i]);

            // change start for the next thread
			start = end;

            // if the next thread, is the last thread               
			if (i == num_threads-2) {                                                       
			    end = end + nth + remain;
			} else {                                                                        
			    end = end + nth;                                                            
			}         
		}
	}

	for (int i = 0; i < num_threads; i++) {                                                                
		pthread_join(threads[i].tid, NULL);                                                 
	}

	free(threads);
}
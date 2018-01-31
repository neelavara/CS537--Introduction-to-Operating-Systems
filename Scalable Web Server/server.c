#include "cs537.h"
#include "request.h"
#include <pthread.h>
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_empty = PTHREAD_COND_INITIALIZER, buffer_full = PTHREAD_COND_INITIALIZER;
int count = 0;
int fill = 0;
int use = 0;
int *buffer;  // Pointer for creating dynamic buffer size
int listenfd, port, clientlen, threads, buffers;
// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// CS537: Parse the new arguments too

void getargs(int argc, char *argv[])
{
    if (argc != 4) {
	exit(1);
    }
    port = atoi(argv[1]);
    threads = atoi(argv[2]);
    if(threads < 1) exit(1);
    buffers = atoi(argv[3]);
    if(buffers < 1) exit(1);
}


void *worker(void *args){
	int connection_worker = 0;
	while(1){	
		pthread_mutex_lock(&mutex);
		while (count == 0) //buffer is empty
		{
			pthread_cond_wait(&buffer_full, &mutex);
		}
		connection_worker = buffer[use];
        	use = (use + 1);
        	use = use % buffers;
        	count--;
		pthread_cond_signal(&buffer_empty);
		pthread_mutex_unlock(&mutex);
		requestHandle(connection_worker);
		Close(connection_worker);
	}
}

int main(int argc, char *argv[])
{
    struct sockaddr_in clientaddr;
    int ret;
    if (argc != 4) {
        exit(1);
    }
    port = atoi(argv[1]);
    threads = atoi(argv[2]);
    if(threads < 1) exit(1);
    buffers = atoi(argv[3]);
    if(buffers < 1) exit(1);

    int i,connection_master=0;
    // 
    // CS537: Create some threads...
    //
    
    //buffer of size given in command line
    buffer = (int*) malloc(buffers * sizeof(buffer));
    
    pthread_t worker_t[threads];
    for(i=0;i<threads;i++){
    	ret = pthread_create(&worker_t[i], NULL, worker, NULL);
   	 if(ret < 0) exit(1);
	}

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connection_master = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        pthread_mutex_lock(&mutex);
        while (count == buffers) //buffer is full
        {
           pthread_cond_wait(&buffer_empty, &mutex);
        }
	buffer[fill] = connection_master;
        fill = (fill + 1);
        fill = fill %  buffers;
        count++;

        pthread_cond_signal(&buffer_full);
        pthread_mutex_unlock(&mutex);

	// CS537: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	//free(buffer);
    }
	free(buffer);//To handle Valgrind test case
}


    


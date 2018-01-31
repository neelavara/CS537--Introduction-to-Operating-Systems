#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include <sys/mman.h>
#include<string.h>
#include<signal.h>
#include<time.h>
#include<pthread.h>
#define SHM_NAME "neelavara_snatarajan"

pthread_mutex_t *mutex;
struct timespec start, end;

typedef struct {
    int pid;
    char birth[25];
    char clientString[10];
    int elapsed_sec;
    double elapsed_msec;
    int occupied; // 0 - Not Occupied and 1 - Occupied
} stats_t;

stats_t *ptr;
stats_t *client_details;
stats_t *temp;

void exit_handler(int sig) {
	pthread_mutex_lock(mutex);	
	//unallocate the memory
	temp->occupied = 0;
	pthread_mutex_unlock(mutex);
	exit(0);
}

char* birth_details(){
	time_t current_time;
        char *time_string;
        current_time = time(NULL);
        time_string = ctime(&current_time);
	return time_string;
}

void setclient_details(char *argv[]){
	client_details = temp;
	client_details->pid = getpid();
	client_details->occupied = 1;
	char *time_string = birth_details();
	time_string[strlen(time_string)-1] = '\0';
	strncpy(client_details->birth,time_string, 24);
	strncpy(client_details->clientString,argv[1], 9);
	client_details->elapsed_sec = end.tv_sec - start.tv_sec;
	client_details->elapsed_msec = (end.tv_nsec - start.tv_sec)/1000;
}

void print_active(stats_t *start_ptr){
	int iterate = 1;
	printf("Active Clients :");
	while(iterate <= 63){
		if(start_ptr->occupied == 1){
			printf(" %d",start_ptr->pid);
		}
		start_ptr +=1;
		iterate++;
	}	
	printf("\n");
}

int main(int argc, char *argv[]){
	int iterate = 0;
	int page_size = getpagesize();
	int fd = shm_open(SHM_NAME, O_RDWR, 0660);
//	printf("%d\n",sizeof(stats_t));
	if(fd == -1){
		exit(1);
		}
	struct sigaction act;
        act.sa_handler=exit_handler;
        sigaction(SIGINT, &act, NULL);
	sigaction(SIGKILL, &act, NULL);
        sigaction(SIGSTOP, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	ptr = (stats_t *) mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	stats_t *start_ptr = ptr;
	start_ptr +=1;
	mutex = (pthread_mutex_t *) ptr;
	temp = ptr;
	temp = temp + 1;
	//pthread_mutex_lock(mlock);
	while(iterate <= 63){
		pthread_mutex_lock(mutex);
			if(temp->occupied == 0){
				temp->occupied = 1;
				pthread_mutex_unlock(mutex);
				break;
			}
			iterate++;
			temp = temp + 1;
		pthread_mutex_unlock(mutex);
	}
	//pthread_mutex_unlock(mlock);
	if(iterate > 63)
		exit(1);
	while(1){
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
	setclient_details(argv);
	sleep(1);
	print_active(start_ptr);
	}

}


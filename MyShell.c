#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/wait.h>
#define DELIM " \t\r\n\a"
void standard(char *argv[]){
	if(strcmp(argv[0],"exit")== 0) exit(0);
	else if(strcmp(argv[0],"cd") == 0 && argv[1] == NULL){
                 char *path = getenv("HOME");
                 chdir(path);
        }
	else if(strcmp(argv[0],"cd") == 0 && argv[1] != NULL){
                        chdir(argv[1]);
	}
}

void caught_error(){
	char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));

}

int check_operation(char *argv[])
{	int i = 0;
	int flag=100;
	while(argv[i] != NULL){
		if(strcmp(argv[i], ">") == 0){
			if(flag == 1){
				flag = 10;
				continue;
			}
		  flag = 0;
		}
		else if(strcmp(argv[i], "<") == 0){
			if(flag == 0){
				flag = 10;
				continue;
			}
		  flag = 1;
		}
		else if(strcmp(argv[i], "|") == 0){
		  flag = 2;
		  break; 
		}
		else if(strcmp(argv[i], "&") == 0){
		  flag = 3;
		  break;
		}
		i++;
 	}
	return flag;
} 

void output_redirection(char *argv[],int n)
{
	int fd,ret,i=0,pid,k=0;
	char *argv1[20];
	while(argv[i] != NULL){
                if(strcmp(argv[i], ">") == 0){	
			break;
                }
		else{
			argv1[k] = strdup(argv[i]);
               	        k++;
		}
		i++;
	}
	argv1[k+1] = NULL;
	if(i == n || i == 0){
		caught_error();
                return;
	}
	if(argv[i+2] != NULL){
		 caught_error();
                 return;
	}
	fd = open(argv[i+1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
	if(fd == -1){
		 caught_error();
                 return;
	}
	fflush(stdout);
	pid = fork();
	if(pid == 0){
		ret = dup2(fd,STDOUT_FILENO);
		execvp(argv1[0], argv1);
		exit(0);
	}
	if(pid > 0)
		(void) wait(NULL);
	close(fd);
}

void input_redirection(char *argv[],int n)
{	
	int out,i=0,pid,k=0;
	char *argv1[20];
	while(argv[i] != NULL){
		if(strcmp(argv[i],"<") == 0){
			break;
		}
		else{
			argv1[k] = strdup(argv[i]);
			k++;
		}
		i++;
	}
	argv1[k+1] = NULL;
	if(argv[i+2] != NULL){
		 caught_error();
                 return;
	}
	out = open(argv[i+1],O_RDONLY);
	if(out == -1){
		 caught_error();
                 return;
	}
	fflush(stdout);
	pid = fork();
	if(pid == 0){
		dup2(out,STDIN_FILENO);
		execvp(argv1[0], argv1);
	}
	if(pid > 0)
		(void) wait(NULL);
	close(out);
}

void in_out(char *argv[])
{		
	char *command[20], *input, *output;
	int j=0;
	int i=0;
	while(strcmp(argv[i],"<") != 0 && strcmp(argv[i],">") != 0)
	{
		command[j] = strdup(argv[i]);
		j++;
		i++;
	}
	command[j] = NULL;
	j=0;
	if(strcmp(argv[i],"<") == 0){ //input and then output
		input = strdup(argv[i+1]);
		if(strcmp(argv[i+2],">")!=0 || argv[i+3] ==NULL || argv[i+4] != NULL){
			caught_error();
                 	return;
		}
		else
			output = strdup(argv[i+3]);
		i = i+2;
	}

	else if(strcmp(argv[i],">") == 0){//output and then input		
        	output = strdup(argv[i+1]);
                if(strcmp(argv[i+2],"<")!=0 || argv[i+3] ==NULL || argv[i+4] != NULL){
                        caught_error();
                        return;
                }
		else
			input = strdup(argv[i+3]);

	}
	int fd = open(output, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
        if(fd == -1){
                 caught_error();
                 return;
        }
	int out = open(input,O_RDONLY);
        if(out == -1){
                 caught_error();
                 return;
        }
	fflush(stdout);
        int pid = fork();
        if(pid == 0){
		dup2(fd, STDOUT_FILENO);
                dup2(out,STDIN_FILENO);
                execvp(command[0], command);
        }
        if(pid > 0)
                (void) wait(NULL);
        close(out);
	close(fd);
	
}	
int _pipe(char *argv[])
{	
	char *argv1[20];
	char *argv2[20];
	int i=0,k=0;
	int pid1,pid2;
	int pipefd[2];
        while(strcmp(argv[i],"|") != 0){
		argv1[k] = strdup(argv[i]);
		i++; k++;
        }
	argv1[k] = NULL;
	if(i==0){
	   caught_error();
	   return 0;
	}
	else
	   i++;
	int prev = i;
	k = 0;
	while(argv[i]!=NULL){
		argv2[k] = strdup(argv[i]);
		i++; k++;
	}
	if(i == prev){
		 caught_error();
	         return 0;
	}
	argv2[k] = NULL;
	pipe(pipefd);
	pid1 = fork();
		if (pid1 == 0) {
			close(STDOUT_FILENO);
     			close(pipefd[0]);
			dup2(pipefd[1], STDOUT_FILENO);
        		execvp(argv1[0], argv1);
      			perror("exec");
      			return 1;
   			}
   	pid2 = fork();
   		if (pid2 == 0) {
			close(STDIN_FILENO);
      			close(pipefd[1]);
			dup2(pipefd[0], STDIN_FILENO);
      			execvp(argv2[0], argv2);
      			perror("exec");
      			return 1;
			}
   close(pipefd[0]);
   close(pipefd[1]);
   waitpid(pid1,NULL,0);
   waitpid(pid2,NULL,0);
   return 0;
}

int main()
{
	int counter = 0;
	char str[150];
	char *argv[20];
	while(1){
		printf("mysh (%d)> ",++counter);
		fgets(str,150,stdin);
		if(strlen(str) == 1 && str[0] == '\n'){
			 continue;
		}
		if(strlen(str) > 128){
			caught_error();
			continue;
		}
		char *token = strtok(str,DELIM);
	        int i=0;
        	while(token!=NULL){
                	argv[i] = strdup(token);
                	i++;
                	token = strtok(NULL,DELIM);
        	}
		argv[i] = NULL;
		//built in commands
		if(strcmp(argv[0],"cd")== 0 || strcmp(argv[0],"exit") == 0){
			standard(argv);
			continue;
		}
		int flag =  check_operation(argv);
		//flag = 10;
		//Output Redirection - flag = 0
		if(flag == 0)
		{
			output_redirection(argv,i);	
		}
                //Input Redirection - flag = 1
		else if(flag == 1)
		{
			input_redirection(argv,i);
		}
		//Combination of input and output
		else if(flag == 10)
		{
			in_out(argv);
		}
		//Pipe
		else if(flag == 2)
		{
			_pipe(argv);
		}
		//Background process
		else if(flag == 3)
		{ 
		
		}
		else if(flag == 100){
			fflush(stdout);
                	int ret = fork();
                	int err;
                        	if(ret == 0){   
                                	err = execvp(argv[0],argv);

                                	if(err == -1){
                                        	caught_error();
                                    	}
                          	}
                        	else if(ret > 0){
                                	(void) wait(NULL);
                          	}
            	}

	}
}


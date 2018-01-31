#include<stdio.h>
#include <sys/stat.h>
#include<stdlib.h>
#include<string.h>
int main(int argc, char *argv[]){
	if(argc!=5){ //Arguments check
		fprintf(stderr,"Usage: shuffle -i inputfile -o outputfile\n");
		exit(1);
	}
	//Switched arguments
        char *input = "-i";
        char *output = "-o";
	char *input_file;
	char *output_file;
        if(!strcmp(argv[1],output) && !strcmp(argv[3],input))
        {       input_file = argv[4];
		output_file = argv[2];
        }
	else{
		input_file = argv[2];
                output_file = argv[4];
	}
	//Opening a file	
	FILE *fp;
        fp = fopen(input_file,"r");
        if(fp == NULL){ //File doesn't exists
                fprintf(stderr,"Error: Cannot open file %s\n",argv[2]);
                exit(1);
        }
	//Incorrect passage in command line
	if(argc == 5){
		int flag = 0;
		if(!strcmp(argv[1],input) && !strcmp(argv[3],output) || !strcmp(argv[3],input) && !strcmp(argv[1],output))
			flag = 1;
	        if(flag!=1){
			fprintf(stderr,"Usage: shuffle -i inputfile -o outputfile\n");
	                exit(1);
		}		
	}
	 
	struct stat st;
	stat(input_file, &st);
	int size = st.st_size;
	if(size == 0){ // File without content
	       FILE *fp_null = fopen(output_file,"w");
               fclose(fp_null);
               exit(0);
	}
	char *arr = (char *) malloc(size * sizeof (char)); // Allocating memory
	fread(arr,1,size,fp);
	int i,count=0;
	for(i=0;i<size;i++){
		if(arr[i]=='\n'){
			count++;
		}
	}
	char **addr = (char**) malloc(count * sizeof(char*)); // Array to store address
	int j=0;
	addr[0] = &arr[0];
	j++;
	for(i=0;i<size-1;i++){ // Store address
		if(arr[i]=='\n'){
			addr[j] = &arr[i+1];
		       	j++;
		}
	}
	
	FILE *f_out = fopen(output_file,"w");  //Print the contents to file
	if(count==1){
		fwrite(addr[0],1,size-1 ,f_out);
	}
	else{
		fwrite(addr[0],1,addr[1] - addr[0],f_out);
		fwrite(addr[count-1],1,size- (addr[count-1] - addr[0]),f_out);
		for(i=1,j=count-2;i!=j&&i<j;i++,j--){
			fwrite(addr[i],1,addr[i+1] - addr[i],f_out);
			fwrite(addr[j],1,addr[j+1] - addr[j],f_out);
		}
		if(i==j)
			fwrite(addr[i],1,addr[i+1] - addr[i],f_out);
	}
	fclose(f_out);
	fclose(fp);
}	

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>

char storage[100];

int main(int argc, char **argv){

	int accumulator = 0;
	
	//Initializing
	int bytes = atoi(argv[1]);
	if (bytes <= 0){
		printf("Invalid argument. Enter a valid byte size.\n");
		return EXIT_FAILURE;
	}
	
	int open_file, close_status = 0;
	
	if (argc == 2){
		//Read from Standard Input.
	}
	char *filename = argv[2];
	char *buffer = (char *) malloc(sizeof(char)*bytes+1);
	char *print_buf = (char *) malloc(sizeof(char)*bytes+1);
	
	//Open file 
	open_file = open(filename, O_RDONLY, 0);
	if(open_file == -1)  {  
		perror("Cannot open file.\n");
      		//printf("The file cannot be opened\n");  
      		return EXIT_FAILURE;
   	} 
   	
   	
   	
   	
   	//PROCESSING
   	while (read(open_file, buffer, bytes) != 0){
   		for (int i = 0; i < bytes-1; i++){
   			if (buffer[i] == '\n'){
   				if (accumulator == bytes){
   					accumulator = 0;
   					printf("%s\n", print_buf);
   					continue;
   				} else if (accumulator > bytes){
   					printf("%s\n", print_buf);
   					return EXIT_FAILURE;
   				} else {	
   					//accumulator < bytes
   					continue;
   					//Need to handle this part properly.
   				}
   			}
   			
   			if (buffer[i] == ' '){
   				if (accumulator == bytes){
   					accumulator = 0;
   					printf("%s\n", print_buf);
   					continue;
   				} else if (accumulator > bytes){
   					printf("%s\n", print_buf);
   					return EXIT_FAILURE;
   				} else {
   					//accumulator < bytes
   					continue;
   					//Need to handle this part properly.
   				}
   			}
   			
   			if (isalnum(buffer[i])){
   				//This should never happen technically.
   				if (accumulator == bytes){
   					strcat(storage, (char)buffer[i]);
   					continue;
   				} else {
   					strcat(print_buf, (char)buffer[i]);
   					accumulator++;
   					continue;
   				}
   			}
   		}
   	}
   	
   	
   	
   	
   	
   	//If file cannot be closed.
   	close_status = close(open_file);  
      	if(close_status == -1){ 
      		perror("Cannot close file.\n"); 
         	//printf("The file cannot be closed\n"); 
         	return EXIT_FAILURE; 
      	}
      	
      	return EXIT_SUCCESS;
}

/*OPTION 2:
	-> Have a print buffer which you keep checking its length.
	-> Keep appending characters (for/while loop) to it through a method that only adds chars and eliminates double spaces.
	-> After buffer has been filled, if it is still less than bytes_size, read once more.
	-> If buffer is filled and read buffer isn't read completely, keep it stored. Maybe remove the ones already used from the buffer.
*/
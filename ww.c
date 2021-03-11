#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "strbuf.c"
#define BUFFER_SIZE 64	//defines the size of the buffer that read() fills up
#define SPACE " "		// set this to '_' to see where spaces are printed
#define DEBUG_WW 0		//setting to 1 will print out some debugging info

//Global Variables and Structs
	strbuf_t sb;
	char *filename;
	char *buffer;
	int space_ct = 0, newline_ct = 0, accumulator = 0, bytes_read = 0;
	int open_file = 0, close_status = 0, exit_status = 0;
	int first_text_found = 0; //this boolean will indicate if we have read in the first bit of text yet
	char *path;
	struct stat data;
	int err;
	ssize_t nrd;


int wrap(int bytes_read, int bytes, int fd){
	
	for (int i = 0; i < bytes_read; i++){
   		
		   //is the current character some non-whitespace?
			if(!isspace(buffer[i])){

				//if there was 2 or more '\n' between the previous token and the current one
				if(newline_ct >= 2){

					if(DEBUG_WW) printf("\t\tnew para. accumlation = %d/%d. Line #: %d", accumulator, bytes, __LINE__);
					
					//output two newline characters, this starts a new paragraph
					//printf("%s", "\n\n");
					write(fd, "\n\n", 2);
					accumulator = 0;
				}

				//store the current character in the strbuf_t
				strbuf_append(&sb, buffer[i]);

				//since we found some text, reset the space and newline counters
				space_ct = 0;
				newline_ct = 0;

				//if the current char is the first char of the file, set this flag to true.
				if(!first_text_found) first_text_found = 1;	

			//only whitespace characters will reach this block. 
			//whitespace should only be processed if we found the first char in the file
			} else if(first_text_found){
				
				//check if the current char is the first whitespace following a token
				if(newline_ct == 0 && space_ct == 0){

					//check to see if a ' ' and the current token are too large for the current line
					if (accumulator + 1 + sb.used > bytes && accumulator != 0){

						//nothing else can be added to the current line: end it and reset the accumulator.
						//if(DEBUG_WW) printf("\t\tline filled. accumlation = %d/%d. Line #: %d", accumulator, bytes, __LINE__);
						//printf("%s", "\n");
						write(fd, "\n", 1);
						accumulator = 0;


					}  else if(accumulator != 0){

						//this is not the first character on the current line, so print a space first
						//printf("%s", SPACE);
						write(fd, SPACE, 1);
						accumulator++;

					} 

					//output the string stored in the strbuf_t. update the accumulator.
					//printf("%s", sb.data);
					write(fd, sb.data, sb.used);
					accumulator += sb.used;
					if(sb.used > bytes) exit_status = 1;

					//clear the strbuf
					sb.used = 0;
					sb.data[0] = '\0';

				}

				//see what type of whitespace the current char is, increment the relevent counter
				if (buffer[i] == '\n'){
					newline_ct++;	
				} else {
					space_ct++;
				}

			}
   		
   		} //end for loop
   		
   		return 0;
	
}

void flushBuffer(int bytes, int fd){
	//Flushing out storage buffer.
	if(sb.used != 0){

		if (accumulator + 1 + sb.used > bytes && accumulator != 0){

			if(DEBUG_WW) printf("\t\tline filled. accumlation = %d/%d. Line #: %d", accumulator, bytes, __LINE__);
			//printf("%s", "\n");
			write(fd, "\n", 1);
			
			accumulator = 0;

		}  else if(accumulator != 0){

			//printf("%s", SPACE);
			write(fd, SPACE, 1);
			accumulator++;

		} 

		//printf("%s", sb.data);
		write(fd, sb.data, sb.used);
		accumulator += sb.used;
		if(sb.used > bytes) exit_status = 1;

	}
}

int main(int argc, char **argv){

	//Intialize variables and data structures
	int mode = 0; //0 -> stdin, 1 -> file, 2 -> could be for directory.
	strbuf_init(&sb, BUFFER_SIZE);
	buffer = (char *) malloc(sizeof(char) * BUFFER_SIZE);
	
	//handle argument 1 from the command line
	int bytes = atoi(argv[1]);
	if (bytes <= 0){
		printf("Invalid argument. Enter a valid byte size.\n");
		return EXIT_FAILURE;
	}
	
	if (argc == 3){
		path = argv[2];
		err = stat(path, &data);
		if (err){
			perror(path);
			return EXIT_FAILURE;
		}
	}
	
	//handle argument 2 from the command line
	if (argc == 2){
		bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
   	
	   	while (bytes_read > 0){
	   		
			wrap(bytes_read, bytes, 1);
			accumulator = 0;
			bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
	   		
	   	} // end while loop

	   	
	} else if (S_ISREG(data.st_mode)) {
		filename = argv[2];
		mode = 1;
	} else if (S_ISDIR(data.st_mode)){
		mode = 2;
		//Set directory variable.
	}
   	
	//Wrapping from text file.
	if (mode == 1){
		open_file = open(filename, O_RDONLY, 0);
		if(open_file == -1)  {  
			perror("Cannot open file");
	      	//printf("The file cannot be opened\n");  
	      		return EXIT_FAILURE;
	   	} 

		bytes_read = read(open_file, buffer, BUFFER_SIZE);
	   	
	   	while (bytes_read > 0){
	   		
			wrap(bytes_read, bytes, 1);

			bytes_read = read(open_file, buffer, BUFFER_SIZE);
	   		
	   	} // end while loop
	   	
	   	close_status = close(open_file);  
		if(close_status == -1){ 
		      	perror("Cannot close file.\n"); 
		    	return EXIT_FAILURE;
		}
   	}
   	
   	//Wrapping from directory.
   	if (mode == 2){
   		DIR *d;
		struct dirent *dir;
		d = opendir(path);
		if (d){
	
			//While files exist in directory.
			while ((dir = readdir(d)) != NULL){
			
			    //Skip files with '.' and 'wrap.' prefix
			    if ((strncmp(dir->d_name, ".", 1) == 0)){
					printf(". prefix found in file |%s|, skipping this iteration\n", dir->d_name);
					continue;
				}
			    if ((strncmp(dir->d_name, "wrap.", 5) == 0)){
					printf("wrap. prefix found in file |%s|, skipping this iteration\n", dir->d_name);
					continue;
				}
			    if(dir->d_type == DT_DIR){
					printf("sub-directory |%s| found, skipping this iteration\n", dir->d_name);
					continue;
			    }
			    
			    //Form wrap file name.
			    printf("File existing: %s\n", dir->d_name);
				int wrap_size = strlen(path) + 7 + strlen(dir->d_name);	//calculate the array size
			    char* wrap_path = malloc(sizeof(char)*wrap_size);			//create blank array
				strcpy(wrap_path, path);				//copy the directory name to the wrap array
				strcat(wrap_path, "/wrap.");			//cocat the slash and prefix to wrap
			    strcat(wrap_path, dir->d_name);		//cocat the original file name to wrap

				//construct the path to open the specified file
				int file_path_size = strlen(path) + 2 + strlen(dir->d_name);	//calculate the array size
				char* file_path = malloc(sizeof(char)*file_path_size);
				strcpy(file_path, path);
				strcat(file_path, "/");
				strcat(file_path, dir->d_name);

			    //Opening existing file and creating the new 'wrap' prefixed file.
			    int fd = open(file_path, O_RDONLY);
				printf("\toriginal file path: = %s\n", file_path);
			    int fd1 = open(wrap_path, O_WRONLY | O_TRUNC | O_CREAT, 0666); //<- added wrap here
			    if (fd) perror("ERROR: Path file could not be opened");
			    if (fd1) perror("ERROR: Wrap could not be opened or created");
			    
			    printf("\tWrap file path: %s\n", wrap_path);
			    
			    //Reading original file.
			    nrd = read(fd, buffer, BUFFER_SIZE);
				printf("\tnrd = %ld\n", nrd);
			    
			    //While there is text to be read, we write to the new file.
			    //For now we just print "Hello" to the new file.
			    while (nrd > 0) {
					wrap(nrd, bytes, fd1);
					nrd = read(fd, buffer, BUFFER_SIZE);
			    } 
			    
			    if(close(fd)) perror("ERROR: File cannot be closed");
			    if(close(fd1)) perror("ERROR: File cannot be closed");
			    	free(wrap_path);
			    	free(file_path);
				strcpy(buffer, "");	
				accumulator = 0;	//<-replaced free(buffer);
			    printf("Complete\n");
			}
			
			int close_stat = closedir(d);
			if (close_stat) perror("ERROR: Directory cannot be closed");
		} /*else {
			perror("ERROR: Directory could not be opened.");
		    }
		*/
   	}

	//check to see if theres anything left in storage that hasnt been sent out yet
	flushBuffer(bytes, 1); 
				//	   ^ need to make this a variable!!!!!!

	if(DEBUG_WW) printf("\t\tEOF. accumlation = %d/%d. Line #: %d", accumulator, bytes, __LINE__);

	//tack on a newline after the final token.
	printf("%s", "\n");


	strbuf_destroy(&sb);
	free(buffer);
	
    if (exit_status == 1) return EXIT_FAILURE;
      	
  	return EXIT_SUCCESS;
}

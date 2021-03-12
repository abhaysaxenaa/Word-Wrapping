#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFFER_SIZE 64	//defines the size of the buffer that read() fills up
#define SPACE " "		// set this to '_' to see where spaces are printed
#define DEBUG_WW 0		//setting to 1 will print out some debugging info

//Global Variables and Structs
typedef struct {
    size_t length;
    size_t used;
    char *data;
} strbuf_t;

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

int strbuf_init(strbuf_t *L, size_t length){
	L->data = malloc(sizeof(char) * (length));
	if (!L->data) return 1;

	L->length = length;
	L->used = 0;
	L->data[L->used] = '\0';

	return 0;
}


void strbuf_destroy(strbuf_t *L){
    free(L->data);
}


int strbuf_append(strbuf_t *L, char item){
	//using (used + 1) here to account for the presence of the null terminator
	if ((L->used + 1) == L->length) {
		//double the array length
		size_t size = L->length * 2;
		char *p = realloc(L->data, sizeof(char) * size);
		if (!p) return 1;

		L->data = p;
		L->length = size;

	}

	//append the item to the list, & move null byte
	L->data[L->used] = item;
	L->data[L->used + 1] = '\0';
	++L->used;

	return 0;
}


int wrap(int bytes_read, int bytes, int fd){
	
	for (int i = 0; i < bytes_read; i++){
   		
		  //is the current character some non-whitespace?
		if(!isspace(buffer[i])){

			//if there was 2 or more '\n' between the previous token and the current one
			if(newline_ct >= 2){

				//output two newline characters, this starts a new paragraph
				write(fd, "\n\n", 2);
				accumulator = 0;
			}

			//store the current character in the strbuf_t. reset whitespace counters
			strbuf_append(&sb, buffer[i]);
			space_ct = 0;
			newline_ct = 0;

			//if the current char is the first char of the file, set this flag to true.
			if(!first_text_found) first_text_found = 1;	

		//only whitespace characters will reach this block. 
		//whitespace should only be processed if we found the first char in the file
		} else if(first_text_found){
				
			//check if the current char is the first whitespace following a token
			if(newline_ct == 0 && space_ct == 0){

				//check to see if a ' ' plus the current token are too large for the current line
				if (accumulator + 1 + sb.used > bytes && accumulator != 0){

					//nothing else can be added to the current line: end it and reset the accumulator.
					write(fd, "\n", 1);
					accumulator = 0;

				}  else if(accumulator != 0){

					//this is not the first character on the current line, so print a space first
					write(fd, SPACE, 1);
					accumulator++;

				} 

				//output the string stored in the strbuf_t. update the accumulator.
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

			write(fd, "\n", 1);	
			accumulator = 0;

		}  else if(accumulator != 0){

			write(fd, SPACE, 1);
			accumulator++;

		} 

		write(fd, sb.data, sb.used);
		accumulator += sb.used;
		if(sb.used > bytes) exit_status = 1;


	}

	//make sure the last line ends with a newline char
	write(fd, "\n", 1);

}


int main(int argc, char **argv){

	//Intialize variables and data structures
	int mode = 0; //0 -> stdin, 1 -> file, 2 -> directory.
	int bytes;
	strbuf_init(&sb, BUFFER_SIZE);
	buffer = (char *) malloc(sizeof(char) * BUFFER_SIZE);
	
	//handle argument 1 from the command line
	if(argc >= 2) {
		bytes = atoi(argv[1]);
		if (bytes <= 0){
			printf("Invalid argument. Enter a valid byte size.\n");
			return EXIT_FAILURE;
		}
	} else {
		printf("ERROR: insufficient number of arguments.\n");
		return EXIT_FAILURE;
	}
	
	//handle argument 2 if needed
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
			bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
	   		
	   	} // end while loop
		flushBuffer(bytes, 1);

	   	
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
			perror("ERROR: Cannot open file specified in the path.");
	      	return EXIT_FAILURE;
	   	} 

		//wrap the file's text
		bytes_read = read(open_file, buffer, BUFFER_SIZE);
	   	while (bytes_read > 0){
			wrap(bytes_read, bytes, 1);
			bytes_read = read(open_file, buffer, BUFFER_SIZE);
	   	} 
		flushBuffer(bytes, 1);
	   	
	   	close_status = close(open_file);  
		if(close_status == -1){ 
		    perror("ERROR: Cannot close file specified in path..\n"); 
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
			    if (fd == -1){ 
			    	perror("ERROR: Path file could not be opened");
			    	return EXIT_FAILURE;
			    }
			    if (fd1 == -1){
			    	perror("ERROR: Wrap file could not be opened or created");
			    	return EXIT_FAILURE;
			    }
			    
			    printf("\tWrap file path: %s\n", wrap_path);
			    
			    first_text_found = 0;
			    space_ct = 0;
			    newline_ct = 0;
			    
			    //Reading original file.
			    nrd = read(fd, buffer, BUFFER_SIZE);
				printf("\tnrd = %ld\n", nrd);
			    
			    //While there is text to be read, we write to the new file.
			    while (nrd > 0) {
					wrap(nrd, bytes, fd1);
					nrd = read(fd, buffer, BUFFER_SIZE);
			    } 

				flushBuffer(bytes, fd1);
			    
			    if(close(fd) == -1){
			    	perror("ERROR: File on path could not be closed");
			    	return EXIT_FAILURE;
			    }
			    if(close(fd1) == -1){
			    	perror("ERROR: Wrap file on path could not be closed");
			    	return EXIT_FAILURE;
			    }
			    free(wrap_path);
			    free(file_path);
				strcpy(buffer, "");
				sb.used = 0;
				sb.data[0] = '\0';	
				accumulator = 0;
				first_text_found = 0;
			    printf("Complete\n");
			}
			
			if (closedir(d) == -1){
				perror("ERROR: Directory specified could not be closed");
				return EXIT_FAILURE;
			}
		} else {
			perror("ERROR: Directory specified could not be opened.");
			return EXIT_FAILURE;
		}
   	}

	strbuf_destroy(&sb);
	free(buffer);
	
    if (exit_status == 1) return EXIT_FAILURE;
      	
  	return EXIT_SUCCESS;
}
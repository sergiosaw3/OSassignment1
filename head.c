#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

#define BUFFER_LEN (4096)
#define FIRST_LINE_ALLOC_SIZE ((size_t) 2)
#define FIRST_ARRAY_SIZE ((size_t) 2)

/* 
Calls write() until all bytes are written or 
until an error occurs. Returns 0 on success -1 on failure
*/
int my_write(int fd, const char *buf, size_t bytes){
  size_t bytes_to_be_written;
  size_t bytes_already_written;
  ssize_t bytes_written_this_time;
  bytes_to_be_written = bytes;
  bytes_already_written = (size_t) 0;
  while (bytes_to_be_written > ((size_t) 0)) {
    bytes_written_this_time = write(fd, &buf[bytes_already_written], bytes_to_be_written);
    if (bytes_written_this_time < ((size_t) 0)){
      return -1;
    }
    bytes_to_be_written -= (size_t) bytes_written_this_time;
    bytes_already_written += (size_t) bytes_written_this_time;
  }

  return 0;
}

int my_atoi(char* str){
  int r = 0;

  for(int i=0; str[i]!='\0'; i++){
    r = r*10+str[i] - '0';
  }
  return r;
}

int my_strcmp(char* strg1, char *strg2){
  while((*strg1!='\0' && *strg2!='\0') && *strg1==*strg2){
    strg1++;
    strg2++;
  }
  if(*strg1==*strg2){
    return 0;
  } else{
    return *strg1 - *strg2;
  }
}


int main(int argc, char **argv){
  char buffer[BUFFER_LEN];
  ssize_t read_res; /* signed size */
  size_t read_bytes; /* unsigned size */
  size_t i;
  char c;
  char *current_line;
  size_t current_line_len, current_line_size;
  char *ptr;
  char **lines;
  char **lptr;
  char *llptr;
  size_t lines_len, lines_size;
  char *lines_lengths;

  int temp=0;
  int highest =10;

  char *filename;
  int fd;
  off_t lseek_res;
  size_t filesize;
  //void *ptr;
  
  int isfile=0;
  int n_lines_printed;
  /* We keep on reading until the file hits the end-of-file condition */
  current_line_len = (size_t) 0;
  current_line_size = (size_t) 0;
  current_line = NULL;

  lines_len = (size_t) 0;
  lines_size = (size_t) 0;
  lines = NULL;
  lines_lengths = NULL;

  if(argc>1){
    int nfound =0;
    for(int i=0;i<argc;i++){
      if(my_strcmp(argv[i],"-n")==0){
	highest = my_atoi(argv[i+1]);
	nfound=1;
	if(argc>3){
	  if(i+2>=argc){
	    filename = argv[i-1];
	  }else{
	    filename = argv[i+2];
	  }
	}
      }
    }
    if(nfound==0){
      filename = argv[1];
      isfile=1;
    }
    if(highest<0){
      fprintf(stderr,"Can not input negative value\n");
      return 1;
    }
  }

  if(isfile==1){
    fd = open(filename, O_RDONLY);
    if(fd<0){
      fprintf(stderr, "Error opening file \"%s\": %s\n", filename, strerror(errno));
      return 1;
    }

    /*while (n_lines_printed < highest){
      read_res = read()
      }*/
    
    }
   
  while(1){
    /* Try to read into the buffer, up to sizeof(buffer) bytes */
    read_res = read(0, buffer, sizeof(buffer));

    /* Handle the return values of the read system call */

    /*If the returne dvalue is zero, we are done, as this is end-of-file*/
    if (read_res == ((ssize_t) 0)) break;

    /* If the returned value is negative, we have an error and we die */

    if (read_res < ((ssize_t) 0)) {
      /* Display the appropriate error message and die */
      fprintf(stderr, "Error reading: %s\n", strerror(errno));
      /* Deallocate everything that has been allocated */
      for (i=(size_t) 0; i<lines_len; i++){
	free(lines[i]);
      }
      free(lines);
      free(lines_lengths);
      return 1; //Could also return any other number, its arbitrary
    }

    /* We know that read_res is positive */
    read_bytes = (size_t) read_res;

    if(isfile==1){
      read_res = read(fd,buffer, sizeof(buffer));
      read_bytes = (size_t) read_res;
    }

    /* Here, we need to handle the input and put it into memory */
    for(i=(size_t) 0; i < read_bytes; i++){
      /* Get current character */
      c = buffer[i];

      /* Put the current character into the current line */
      if ((current_line_len + ((size_t) 1)) > current_line_size){
	/* Allocate Memory */
	if ((current_line_len == ((size_t) 0)) ||
	    (current_line == NULL)) {
	  /* First allocation */
	  ptr = (char *) malloc(FIRST_LINE_ALLOC_SIZE * sizeof(char)); //Each char is 1 byte
	  if (ptr == NULL) {
	    fprintf(stderr, "Could not allocate any more memory.\n");
	    /* Deallocate everything that has been allocated */
	    for (i=(size_t) 0; i<lines_len; i++){
	      free(lines[i]);
	    }
	    free(lines);
	    free(lines_lengths);
	    return 1;
	  }
	  current_line = ptr;
	  current_line_size = FIRST_LINE_ALLOC_SIZE;
	} else{
	  /* Reallocation */
	  ptr = (char *) realloc(current_line, current_line_size * ((size_t) 2) * sizeof(char));
	  /* TODO: check overflow of the multiplication */
	  if (ptr == NULL){
	    fprintf(stderr, "Could not allocate any more memory.\n");
	    /* Deallocate everything that has been allocated */
	    for (i=(size_t) 0; i<lines_len; i++){
	      free(lines[i]);
	    }
	    free(lines);
	    free(lines_lengths);
	    return 1;
	  }
	  current_line = ptr;
	  current_line_size *= (size_t) 2;
	}
      }

      /* Here, we are sure to have the right space in memory. 
	 We put the character in and increment the length */
      current_line[current_line_len] = c;
      current_line_len++;


      /* If this is a newline character, start a new line */
      if (c == '\n'){
	if ((lines_len + ((size_t) 1)) > lines_size) {
	  /* Allocate memory :) */
	  if ((lines_len == ((size_t) 0)) ||
	      (lines == NULL)) {
	    /* First allocation */
	    lptr = (char **) malloc(FIRST_ARRAY_SIZE * sizeof(char *));
	    if(lptr == NULL){
	      fprintf(stderr, "Could not allocate any more memory.\n");
	      /* Deallocate everything that has been allocated */
	      for (i=(size_t) 0; i<lines_len; i++){
		free(lines[i]);
	      }
	      free(lines);
	      free(lines_lengths);
	      return 1;
	    }
	    lines = lptr;
	    lines_size = FIRST_ARRAY_SIZE;
	    llptr = (char *) malloc(FIRST_ARRAY_SIZE * sizeof(char));
	    if(llptr == NULL){
	      fprintf(stderr, "Could not allocate any more memory.\n");
	      /* Deallocate everything that has been allocated */
	      for (i=(size_t) 0; i<lines_len; i++){
		free(lines[i]);
	      }
	      free(lines);
	      free(lines_lengths);
	      return 1;
	    }
	    lines_lengths = llptr;
	  } else {
	    /* Reallocation */
	    lptr = (char **) realloc(lines, lines_size * ((size_t) 2) * sizeof(char *));

	    if(lptr == NULL){
	      fprintf(stderr, "Could not allocate any more memory.\n");
	      /* Deallocate everything that has been allocated */
	      for (i=(size_t) 0; i<lines_len; i++){
		free(lines[i]);
	      }
	      free(lines);
	      free(lines_lengths);
	      return 1;
	    }
	    lines = lptr;
	    lines_size *= (size_t) 2;

	    /* Reallocation */
	    llptr = (char *) realloc(lines_lengths, lines_size * ((size_t) 2) * sizeof(char));

	    if(llptr == NULL){
	      fprintf(stderr, "Could not allocate any more memory.\n");
	      /* Deallocate everything that has been allocated */
	      for (i=(size_t) 0; i<lines_len; i++){
		free(lines[i]);
	      }
	      free(lines);
	      free(lines_lengths);
	      return 1;
	    }
	    lines_lengths = llptr;
	  }
	}
	lines[lines_len] = current_line;
	lines_lengths[lines_len] = current_line_len;
	lines_len++;
	current_line = NULL;
	current_line_len = (size_t) 0;
	current_line_size = (size_t) 0;
      }
    }

  }

  /* In the case when the last line has no new line character at the
     end we need to put the line into the array of lines nevertheless */
  if ( !((current_line == NULL) || (current_line_len == (size_t) 0)) ) {
    if ((lines_len + ((size_t) 1)) > lines_size) {
      /* Allocate memory :) */
      if ((lines_len == ((size_t) 0)) ||
	  (lines == NULL)) {
	/* First allocation */
	lptr = (char **) malloc(FIRST_ARRAY_SIZE * sizeof(char *));
	if(lptr == NULL){
	  fprintf(stderr, "Could not allocate any more memory.\n");
	  /* Deallocate everything that has been allocated */
	  for (i=(size_t) 0; i<lines_len; i++){
	    free(lines[i]);
	  }
	  free(lines);
	  free(lines_lengths);
	  return 1;
	}
	lines = lptr;
	lines_size = FIRST_ARRAY_SIZE;
	llptr = (char *) malloc(FIRST_ARRAY_SIZE * sizeof(char));
	if(llptr == NULL){
	  fprintf(stderr, "Could not allocate any more memory.\n");
	  /* Deallocate everything that has been allocated */
	  for (i=(size_t) 0; i<lines_len; i++){
	    free(lines[i]);
	  }
	  free(lines);
	  free(lines_lengths);
	  return 1;
	}
	lines_lengths = llptr;
      } else {
	/* Reallocation */
	lptr = (char **) realloc(lines, lines_size * ((size_t) 2) * sizeof(char *));

	if(lptr == NULL){
	  fprintf(stderr, "Could not allocate any more memory.\n");
	  /* Deallocate everything that has been allocated */
	  for (i=(size_t) 0; i<lines_len; i++){
	    free(lines[i]);
	  }
	  free(lines);
	  free(lines_lengths);
	  return 1;
	}
	lines = lptr;
	lines_size *= (size_t) 2;

	/* Reallocation */
	llptr = (char *) realloc(lines_lengths, lines_size * ((size_t) 2) * sizeof(char));

	if(llptr == NULL){
	  fprintf(stderr, "Could not allocate any more memory.\n");
	  /* Deallocate everything that has been allocated */
	  for (i=(size_t) 0; i<lines_len; i++){
	    free(lines[i]);
	  }
	  free(lines);
	  free(lines_lengths);
	  return 1;
	}
	lines_lengths = llptr;
      }

      lines[lines_len] = current_line;
      lines_lengths[lines_len] = current_line_len;
      lines_len++;
      current_line = NULL;
      current_line_len = (size_t) 0;
      current_line_size = (size_t) 0;
    }
  }

  /* Here, we have an array lines of "strings", i.e. array lines of 
     pointers to characters. There are lines_len valid entries */
  
  for (i=(size_t) 0; i<lines_len; i++){
    if(temp<highest){
      my_write(1, lines[i], lines_lengths[i]);
      temp++;
    }
    else break;
  }
  

  /* Deallocate everything that has been allocated */
  for (i=(size_t) 0; i<lines_len; i++){
    free(lines[i]);
  }
  free(lines);
  free(lines_lengths);

  /* Signal success */
  return 0;
}

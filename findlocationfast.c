#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

typedef struct {
  char number[6];
  char location[25];
  char newline;
} entry_t;

/* Assumes str is null terminated */
size_t my_strlen(const char *str) {
  size_t length = (size_t) 0;
  const char *curr = str;
  while(*curr != '\0') {
    length++;
    curr++;
  }
  return length;
}

int my_write(int fd, const char *buf, size_t bytes) {
  size_t bytes_to_write;
  size_t bytes_already_written;
  size_t bytes_written_this_time;

  bytes_to_write = bytes;
  bytes_already_written = (size_t) 0;
  while(bytes_to_write > ((size_t) 0)) {
    bytes_written_this_time = write(fd, &buf[bytes_already_written], bytes_to_write);
    if(bytes_written_this_time < ((size_t) 0)) {
      return -1;
    }
    bytes_to_write -= (size_t) bytes_written_this_time;
    bytes_already_written += (size_t) bytes_written_this_time;
  }
  return 0;
}

void display_error_message(const char *msg) {
  size_t len = my_strlen(msg);
  my_write(2, msg, len);
}


void usage() {
  display_error_message("To search on nanpa:\nfindlocationfast <6-digit prefix>\n");
  display_error_message("To search on any file:\nfindlocationfast <filename> <6-digit prefix>\n");
}

void print_trimmed(const char *str) {
  const char *curr;
  size_t bytes;
  /*
    Move curr to the end of str and increment backwards until you reach
    a non space character 
  */
  curr = str + 24;
  while(*curr == ' ') {
    curr--;
  }

  bytes = (size_t) (curr - str + 1);
  if (my_write(1, str, bytes) < 0) {
    display_error_message("An error occurred while writing location to std out.\n");
  }
  
}
  
int compare_entries(const char *word_a, const char *word_b) {
  /* Returns -1 if word_a comes before word_b and +1 if b comes before a
     
     I know that each prefix string is 6 chars long.  
     I do not need to check for \0 or ' ' to determine if a string has ended.
     
   */
  int pos;

  for (pos = 0; pos < 6; pos++) {
    if (word_a[pos] < word_b[pos]) return -1; //a comes before b
    if (word_a[pos] > word_b[pos]) return 1;  //b comes before a
  }
  /* If it gets here, word_a == word_b */
  return 0;
}
  
char *lookup_doit(entry_t entry_list[], ssize_t num_entries, const char *word) {
  ssize_t l, r, m;
  int cmp;
  
  l = (ssize_t) 0;
  r = num_entries - ((ssize_t) 1);
  while (l <= r) {
    m = (l + r) / ((ssize_t) 2);
    cmp = compare_entries(entry_list[m].number, word);
    if (cmp == 0) return entry_list[m].location;
    if (cmp < 0) {
      l = m + ((ssize_t) 1);
    } else {
      r = m - ((ssize_t) 1);
    }
    
  }
  return NULL;
}

int lookup(const char *filename, const char *numstr) {
  int fd;
  off_t lseek_res;
  size_t filesize;
  void *map_ptr;
  char *location;
  
  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    display_error_message("An error occurred while opening the file.\n");
    return 1;
  }
  
  //Seek to the end of the file to find length and if the file is seekable and regular
  lseek_res = lseek(fd, (off_t) 0, SEEK_END );
  
  if(lseek_res == (off_t) -1) {
    display_error_message("An error occurred: File is not seekable\n");
    if (close(fd) < 0) {
      display_error_message("An error occurred while closing file\n");
    }
    return 1;
  }

  //Cast result of lseek to size_t
  filesize = (size_t) lseek_res;

  //Check if file is in proper format
  if ((filesize % sizeof(entry_t)) != ((size_t) 0)) {
    display_error_message("An error occurred while reading file:  File not properly formatted\n");
    if (close(fd) < 0) {
      display_error_message("An error occurred while closing the file.\n");
    }
    return 1;
  }

  //Use mmap to load file contents into memory.
  map_ptr = mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, (off_t) 0 );

  //Check if mmap succeeded and if not, close the file and return
  if (map_ptr == MAP_FAILED) {
    display_error_message("An error occurred while mapping file.\n");
    if (close(fd) < 0) {
      display_error_message("An error occurred while closing file.\n");
    }
    return 1;
  }


  /* Actually look for the prefix within the mapped memory
   */
  location = lookup_doit((entry_t *) map_ptr, ((size_t) (filesize / sizeof(entry_t))), numstr);
  /* If the prefix is not in the file, unmap the file from memory,
     close the file, and return 1 
   */
  if (location == NULL) {
    display_error_message("The number prefix was not found within the file.\n");
    if (munmap(map_ptr, filesize) <0 ) {
      display_error_message("An error occurred while unmapping file.\n");
      if (close(fd) < 0) {
	display_error_message("An error occurred while closing file.\n");
      }
      return 1;
    }
  } else {
    /* If prefix is in the file, then print the location
       that corresponds to that prefix.
     */
    print_trimmed(location);
    my_write(1, "\n", (size_t) 1);
  }

  /* Unmap file from memory */
  if (munmap(map_ptr, filesize) <0 ) {
    display_error_message("An error occurred while unmapping file.\n");
    if (close(fd) < 0) {
      display_error_message("An error occurred while closing file.\n");
    }
    return 1;
  }
  
  /* Close file and handle error */
  if (close(fd) < 0) {
    display_error_message("An error occurred while closing file.\n");
    return 1;
  }
  return 0;
}

int check_number_string(const char *str) {
  /* Already know that the string is 6 long.
     Returns 1 if string is not all numbers.
     Returns 0 if string is all numbers.
   */
  int i;
  for(i = 0; i < 6; i++) {
    if((str[i] - '0' < 0) || (str[i] - '0' > 9)) return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  char *prefix;
  char *filename;

  /* Error is there are an improper amount of arguments */
  if(argc < 2 || argc > 3) {
    usage();
    return 1;
  }

  if (argc == 2) {
    filename = "nanpa";
    prefix = argv[1];
  }else {
    filename = argv[1];
    prefix = argv[2];
  }
  
  if(my_strlen(prefix) != ((size_t) 6)) {
    usage();
    return 1;
  }
  if(check_number_string(prefix)) {
    usage();
    return 1;
  }
  
  if(lookup(filename, prefix)) return 1;
  
  
  return 0;
}

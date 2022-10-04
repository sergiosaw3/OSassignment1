#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

int childOne(int ear, int mouth, char *call,char *argv[], int fd){
  if(close(mouth)<0){
    fprintf(stderr,"close() did not work:%s\n",strerror(errno));
    if(close(ear)<0){
      fprintf(stderr,"close() did not work:%s\n",strerror(errno));
    }
    return 2;
  }

  if(close(fd)<0){
    fprintf(stderr,"close() did not work:%s\n",strerror(errno));
    return 2;
  }
  
  if(dup2(ear,1)<0){
    fprintf(stderr,"dup2() did not work:%s\n",strerror(errno));
    if(close(ear)<0){
      fprintf(stderr,"close() did not work:%s\n",strerror(errno));
    }
    return 2;
  }

  if(execvp(call,argv)<0){ // argv[0]
    fprintf(stderr,"execvp() did not work:%s\n",strerror(errno));
    return 2;
  }

  return 0;
}


int main(int argc, char **argv){
  int fd;
  int pipefd[2];
  int ear, mouth;
  int pidChildOne;
  int *argv2[argc-2];

  if(argc<3){
    fprintf(stderr,"Not enough command line arguments\n");
    return 1;
  }
  else{
    int temp =0;
    for(int i=2;i<argc;i++){
      *argv2[temp]=*argv[i];
      temp++;
    }
  }

  fd=open(argv[1], O_RDONLY, 0644);
  if(fd<0){
    fprintf(stderr,"Error opening file\"%s\": %s\n", argv[1], strerror(errno));
    if(close(fd)<0){
      fprintf(stderr,"Error closing file\"%s\": %s\n", argv[1], strerror(errno));
    }
    return 1;
  }

  if(pipe(pipefd)<0){
    fprintf(stderr,"pipe() did not work:%s\n",strerror(errno));
    if(close(fd)<0){
      fprintf(stderr,"Error closing file\"%s\": %s\n", argv[1], strerror(errno));
    }
    return 1;
  }

  mouth = pipefd[0];
  ear = pipefd[1];
  
  pidChildOne=fork();
  if(pidChildOne <((pid_t) 0)){
    fprintf(stderr, "fork() did not work:%s\n",strerror(errno));
    if(close(mouth)<0){
      fprintf(stderr,"close() did not work:%s\n",strerror(errno));
    }
    if(close(ear)<0){
      fprintf(stderr,"close() did not work:%s\n",strerror(errno));
    }
    if(close(fd)<0){
      fprintf(stderr,"Error closing file\"%s\": %s\n", argv[1], strerror(errno));
    }
    return 1;
  }
  if(pidChildOne==((pid_t) 0)){
    return childOne(ear,mouth, argv[2],argv2, fd);
  }

  if(close(mouth)<0){
    fprintf(stderr,"close() did not work:%s\n",strerror(errno));
    if(close(fd)<0){
      fprintf(stderr,"Error closing file\"%s\": %s\n", argv[1], strerror(errno));
    }
    return 1;
  }
  wait(NULL);

  if(close(ear)<0){
    fprintf(stderr,"close() did not work:%s\n",strerror(errno));
  }
  
  if(close(fd)<0){
    fprintf(stderr,"Error closing file\"%s\": %s\n", argv[1], strerror(errno));
  }
  return 0;  
  
}

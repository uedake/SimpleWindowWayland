#include <iostream>
#include "WaylandCore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/inotify.h>

#define WIDTH 640
#define HEIGHT 480
#define TITLE  "DocomoTest"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
#define BUFF_SIZE (256)

using namespace std;

int EVENT_LOOP_WAIT_USEC=10*1000;
string FIRST_PROMPT = ">>";
string AFTER_STDOUT_PROMPT = "--";

WaylandCore* mCore;
int old_stdin_flag;

int handle_cmd(string cmd){
  if (cmd=="exit")
    return 1;
  else if (cmd=="help")
    cout << "Here is help" << endl;
  else
    cout << cmd  << " is not a command" << endl;
  
  return 0;
}

void set_stdin_nonblocking(bool enable){
  if(enable){
    old_stdin_flag = fcntl(0,F_GETFL); 
    fcntl(0,F_SETFL, old_stdin_flag | O_NONBLOCK);
  }
  else{
    if(old_stdin_flag) 
      fcntl(0,F_SETFL, old_stdin_flag);
  }
}

int get_non_blocking_inotify_fd(char *fn){
  int fd = inotify_init();
  if (fd == -1) {
    perror( "inotify_init" );
    return -1;
  }
  fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK);

  cout << "watching " << fn << endl;  
  int wd = inotify_add_watch( fd, fn, 
            IN_MODIFY | IN_DELETE | IN_CREATE);
  if (wd==-1){
    perror( "inotify_add_watch" );
    close(fd);
    return -1;
  }
    
  return fd;
}

void handle_file_modify(const char* name){
  cout << "\n" << name << " is modified" << endl;
  cout << AFTER_STDOUT_PROMPT << flush;
}
void handle_file_create(const char* name){
  cout << "\n" << name << " is created" << endl;
  cout << AFTER_STDOUT_PROMPT << flush;
}
void handle_file_delete(const char* name){
  cout << "\n" << name << " is deleted" << endl;
  cout << AFTER_STDOUT_PROMPT << flush;
}

int inotify_read_events( int fd )
{
  char buffer[BUF_LEN];
  int length = read( fd, buffer, BUF_LEN );
  if( length < 0 ) return -1;

  int count = 0;
  int i=0;
  while ( i < length ) {
        struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];


        char* name=(char*)"file";
        if(event->len>0)
          name=event->name;

        if(event->mask & IN_CREATE ) {
          handle_file_create(name);
        }
        if(event->mask & IN_MODIFY ) {
          handle_file_modify(name);
        }
        if(event->mask & IN_DELETE){
          handle_file_delete(name);
        }

        i += EVENT_SIZE + event->len;
        count++;
  }
  return count;
}

int main(int argc, char **argv ){

  if  ( argc != 2 ) {
    cerr <<  "Usage:" << argv[0] << " filepath" << endl;
    return 1;
  }
 
  int fd =  get_non_blocking_inotify_fd(argv[1]);
  if(fd==-1)
    return 2;

  mCore = new WaylandCore(WIDTH, HEIGHT, TITLE);
  
  set_stdin_nonblocking(true);
  cout << "old stdin flag:" << old_stdin_flag << endl;

  while(1){
    cout << FIRST_PROMPT << flush;

    char buff[BUFF_SIZE]={};

    while (!mCore->isShouldClose()) {
        if(read(0, buff, BUFF_SIZE) != -1)
          break;
        inotify_read_events(fd);
        mCore->pollEvents();
        usleep(EVENT_LOOP_WAIT_USEC);
    }

    string cmd="";
    for(int n=0;n<BUFF_SIZE;n++){
      if(buff[n]==EOF || buff[n]=='\n')
        break;
      cmd+=buff[n];
    }

    if(handle_cmd(cmd)==1)
      break;
  }
  close(fd);
  set_stdin_nonblocking(false);

  delete mCore;mCore = NULL;
  return 0;
}


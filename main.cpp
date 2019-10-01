#include <iostream>
#include "WaylandCore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/inotify.h>

#define INOTIFY_EVENT_SIZE  ( sizeof (struct inotify_event) )
#define INOTIFY_BUF_LEN     ( 1024 * ( INOTIFY_EVENT_SIZE + 16 ) )
#define CMD_BUF_SIZE (256)

#define FIRST_PROMPT  ">>"
#define AFTER_STDOUT_PROMPT  "--"

#define WIDTH 640
#define HEIGHT 480
#define TITLE "DocomoTest"

using namespace std;

static int EVENT_LOOP_WAIT_USEC=10*1000;
static WaylandCore* mCore=NULL;

static void init_window(){
  if(mCore==NULL)
    mCore = new WaylandCore(WIDTH, HEIGHT, TITLE);
}

static int handle_cmd(string cmd){
  if (cmd=="exit")
    return 1;
  else if (cmd=="help")
    cout << "Here is help" << endl;
  else if (cmd=="show")
    init_window();
  else if (cmd=="exception")
    throw "test exception";
  else
    cout << cmd  << " is not a command" << endl;
  
  return 0;
}

static int set_stdin_nonblocking(bool enable){
  static bool inited=false;
  static int old_stdin_flag;
  if(!inited){
    old_stdin_flag = fcntl(0,F_GETFL); 
    inited=true;
  }

  int ret;

  if(enable)
    ret=fcntl(0,F_SETFL, old_stdin_flag | O_NONBLOCK);
  else
    ret=fcntl(0,F_SETFL, old_stdin_flag & ~O_NONBLOCK);
  
  if (ret == -1) {
    perror( "set_stdin_nonblocking" );
    return -1;
  }
  return 0;
}

static int get_non_blocking_inotify_fd(char *fn){
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

static void print_in_prompt(string str){
  cout << "\n" << str << endl;
  cout << AFTER_STDOUT_PROMPT << flush;
}

static void handle_file_modify(string name){
  print_in_prompt(name + " is modified");
}
static void handle_file_create(string name){
  print_in_prompt(name + " is created");
}
static void handle_file_delete(string name){
  print_in_prompt(name + " is deleted");
}

static int inotify_read_events( int fd )
{
  char buffer[INOTIFY_BUF_LEN];
  int length = read( fd, buffer, INOTIFY_BUF_LEN );
  if( length < 0 ) return -1;

  int count = 0;
  int i=0;
  while ( i < length ) {
        struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];

        char* name=(char*)"file";
        if(event->len>0)
          name=event->name;

        if(event->mask & IN_CREATE )
          handle_file_create(name);
        if(event->mask & IN_MODIFY )
          handle_file_modify(name);
        if(event->mask & IN_DELETE)
          handle_file_delete(name);

        i += INOTIFY_EVENT_SIZE + event->len;
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

  try{

    //init_window();
    
    if(set_stdin_nonblocking(true)==-1){
      return 3;
    }

    char buff[CMD_BUF_SIZE]={};
    cout << FIRST_PROMPT << flush;
    while (mCore == NULL || !mCore->isShouldClose()) {
        int read_cnt=read(0, buff, CMD_BUF_SIZE);
        if(read_cnt > 0){
          string cmd="";
          for(int n=0;n<read_cnt;n++){
            if(buff[n]==EOF || buff[n]=='\n')
              break;
            cmd+=buff[n];
          }
          if(handle_cmd(cmd)==1)
            break;
          cout << FIRST_PROMPT << flush;
        }
        inotify_read_events(fd);
        if(mCore!=NULL)
          mCore->pollEvents();
        usleep(EVENT_LOOP_WAIT_USEC);
    }
  }
  catch(...){
    cout << "exception occured" << endl;
  }
  close(fd);
  set_stdin_nonblocking(false);
  delete mCore;mCore = NULL;
  cout << "Exited" << endl;
  return 0;
}


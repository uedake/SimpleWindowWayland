#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/inotify.h>
#include <vector>

#include "WaylandCore.h"
#include "DirectoryWatcher.h"

#define CMD_BUF_SIZE (256)

#define FIRST_PROMPT  ">>"
#define AFTER_STDOUT_PROMPT  "--"

using namespace std;

static int EVENT_LOOP_WAIT_USEC=10*1000;
static WaylandCore* mCore=NULL;
static int WIDTH=640;
static int HEIGHT=480;
static const char* TITLE="DocomoTest";

#define WL_CORE 0
#define WL_REDRAW 1
#define WL_REDRAW_SAMPLE 2

static void print_in_prompt(string str){
  cout << "\n" << str << endl;
  cout << AFTER_STDOUT_PROMPT << flush;
}

static void init_window(int type){
  if(mCore!=nullptr)
    delete mCore;mCore = nullptr;
  
  if(type==WL_CORE)
    mCore = new WaylandCore(WIDTH, HEIGHT, TITLE);
  else if(type==WL_REDRAW)
    mCore = new WaylandRedrawable(WIDTH, HEIGHT, TITLE);
  else if(type==WL_REDRAW_SAMPLE)
    mCore = new SampleWaylandRedrawable(WIDTH, HEIGHT, TITLE);
}

static vector<string> split(const string &s, char delim) {
    vector<string> elems;
    string item;
    for (char ch: s) {
        if (ch == delim) {
            if (!item.empty())
                elems.push_back(item);
            item.clear();
        }
        else {
            item += ch;
        }
    }
    if (!item.empty())
        elems.push_back(item);
    return elems;
}

static int handle_cmd(string cmd,string program){
  vector<string> args=split(cmd,' ');
  int argc =args.size();

  if (args[0]=="exit")
    return 1;
  else if (args[0]=="help")
    cerr <<  "Usage:" << program << " [filepath]" << endl;
  else if(args[0]=="show" && argc==2 && args[1]=="core")
    init_window(WL_CORE);
  else if(args[0]=="show" && argc==2 && args[1]=="redraw")
    init_window(WL_REDRAW);
  else if (args[0]=="show")
    init_window(WL_REDRAW_SAMPLE);
  else if (args[0]=="top"){
    if(mCore)
      mCore->setFullscreen(false);
    else
      cout << "window has not inited yet" << endl;    
  }
  else if (args[0]=="full"){
    if(mCore)
      mCore->setFullscreen(true);
    else
      cout << "window has not inited yet" << endl;    
  }
  else if (args[0]=="reflect"){
    if(mCore)
      mCore->refrectBuffer();
    else
      cout << "window has not inited yet" << endl;    
  }
  else if (args[0]=="resize" && argc==3){
    try{
      if(mCore){
        int32_t w=stoi(args[1],nullptr,0);
        int32_t h=stoi(args[2],nullptr,0);
        mCore->on_resize(w,h);
      }
      else
        cout << "window has not inited yet" << endl;    
    }
    catch(invalid_argument ex){
      cout << args[1] << " " << args[2]  << " is not a number" << endl;
    }
  }
  else if (args[0]=="fill" && argc==2){
    try{
      if(mCore){
        int32_t col=stoi(args[1],nullptr,0);
        mCore->setFillColor(col);
      }
      else
        cout << "window has not inited yet" << endl;    
    }
    catch(invalid_argument ex){
      cout << args[1]  << " is not a number" << endl;
    }
  }
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

class ReflectImageTrigger: public FileSync{
  using FileSync::FileSync;
  void on_receive(int counter) override{
    if(mCore)
      mCore->refrectBuffer();
    else
      print_in_prompt("window has not inited yet");    
  }
};

int main(int argc, char **argv ){
  const char* path = NULL;

  if  ( argc >= 2 ) {
    path = argv[1];
  }
  else{
    path = getenv("XDG_RUNTIME_DIR");
  }
  if( !path) {
    cerr << "set XDG_RUNTIME_DIR or supply argv[1]" << endl;
    return 1;
  }

  cout << "set watching dir:" << path << endl;  

  ReflectImageTrigger* dw;
  try{ 
    dw = new ReflectImageTrigger(path,"rcv","ack",print_in_prompt);
  }
  catch(...){
    cerr << "cannot create triger" << endl;
    return 2;
  }

  try{
    if(set_stdin_nonblocking(true)==-1){
      cerr << "cannot set stdin non-blocking" << endl;
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
          if(cmd!=""){
            if(handle_cmd(cmd,argv[0])==1)
              break;
          }
          cout << FIRST_PROMPT << flush;
        }
        dw->poll();
        if(mCore!=NULL)
          mCore->pollEvents();
        usleep(EVENT_LOOP_WAIT_USEC);
    }
  }
  catch(...){
    cerr << "exception occured" << endl;
  }
  delete dw; dw=nullptr;
  set_stdin_nonblocking(false);
  delete mCore;mCore = NULL;
  cout << "Exited" << endl;
  return 0;
}


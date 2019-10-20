#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/stat.h>

#include "DirectoryWatcher.h"

using namespace std;
 
void simple_print(string str){
  cout << str << endl;
}

void simple_callback(string str,int counter){
  cout << "rcv = " + to_string(counter) << endl;
}


DirectoryWatcher::DirectoryWatcher(string dir_path,void (*log_func)(string)){
  fd = inotify_init();
  log=log_func;
  if (fd == -1) {
    perror( "inotify_init" );
    throw "Exception";
  }
  fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK);

  wd = inotify_add_watch( fd, dir_path.c_str(), 
            IN_MODIFY | IN_DELETE | IN_CREATE);
  if (wd==-1){
    perror( "inotify_add_watch" );
    close(fd);
    throw "Exception";
  }    
}
DirectoryWatcher::~DirectoryWatcher(){
    if(fd){
        close(fd);
    }
}

void DirectoryWatcher::handle_file_modify(string name){
  log(name + " is modified");
}
void DirectoryWatcher::handle_file_create(string name){
  log(name + " is created");
}
void DirectoryWatcher::handle_file_delete(string name){
  log(name + " is deleted");
}

int DirectoryWatcher::poll()
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

FileSync::FileSync(string dir_path,string rcv_file_name,string ack_file_name,void (*log_func)(string),void (*receive_callback_func)(string,int))
: DirectoryWatcher(dir_path,log_func){
    receive_callback=receive_callback_func;
    rcv_fn=rcv_file_name;
    ack_fn=ack_file_name;

    rcv_path=dir_path+"/"+rcv_file_name;
    ack_path=dir_path+"/"+ack_file_name;

    ifstream ifs(rcv_path);
    if (!ifs)
    {
        cout << "cannot open file:" << rcv_path << ". create it." << endl;
        mode_t before=umask(0);
        int fd = creat(rcv_path.c_str(), 0666);
        if (fd == -1){
            throw "Exception";
        }
        close(fd);
        umask(before);
        ofstream ofs(rcv_path);
        ofs << "0";
        ofs.close();
        rcv=0;
    }
    else{
        ifs >> rcv;
        ifs.close();
    }
    cout << "rcv = " << rcv << endl;
}
void FileSync::on_receive(int counter){
    receive_callback(rcv_path,counter);
}

void FileSync::handle_file_modify(string name){
    if(rcv_fn==name){
        ifstream ifs(rcv_path);
        int r;
        ifs >> r;
        ifs.close();

        if(rcv<r){
            rcv=r;
            on_receive(r);
            ofstream ofs(ack_path);
            ofs << rcv;
            ofs.close();
        }
    }
}
void FileSync::handle_file_create(string name){
}
void FileSync::handle_file_delete(string name){
}

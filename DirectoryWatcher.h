#pragma once

#include <iostream>

#define INOTIFY_EVENT_SIZE  ( sizeof (struct inotify_event) )
#define INOTIFY_BUF_LEN     ( 1024 * ( INOTIFY_EVENT_SIZE + 16 ) )

using namespace std;

void simple_print(string str);

class DirectoryWatcher {
    int fd;
    int wd;

  public:
    DirectoryWatcher(string dir_path,void (*log_func)(string) = simple_print);
    ~DirectoryWatcher();
    int poll();

  protected:
    void (*log)(string);
    virtual void handle_file_modify(string name) ;
    virtual void handle_file_delete(string name) ;
    virtual void handle_file_create(string name) ;
};
class FileSync:public DirectoryWatcher {
  string rcv_fn;
  string ack_fn;
  string rcv_path;
  string ack_path;

  int rcv;

  public:
    FileSync(string dir_path,string rcv_file_name,string ack_file_name,void (*log_func)(string) = simple_print);

  protected:
    void handle_file_modify(string name) override;
    void handle_file_delete(string name) override;
    void handle_file_create(string name) override;

    virtual void on_receive(int counter) ;
};
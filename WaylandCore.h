#pragma once

#include <wayland-client.h>
#include "WaylandRegister.h"
#include "ImgBuf.h"
#include "DirectoryWatcher.h"

class WaylandCore;

class ReflectImageTrigger: public FileSync{
  private:
    WaylandCore* mCore;

  public:
    ReflectImageTrigger(WaylandCore* core,string dir_path,string rcv_file_name,string ack_file_name);
    void on_receive(int counter) override;
};

class ReflectImageTrigger2{
  string rcv_fn;
  string ack_fn;
  string rcv_path;
  string ack_path;
  int rcv;

  private:
    WaylandCore* mCore;

  public:
    ReflectImageTrigger2(WaylandCore* core,string dir_path,string rcv_file_name,string ack_file_name);
    void on_receive(int counter);
    int poll();
};


class WaylandCore {

  protected:
    wl_surface* mSurface;
    ImgBuf* mImgBuf;
    bool debug_print=true;
    int32_t mFillColor;

  private:
    wl_display* mDisplay;
    wl_shell_surface* mShellSurface;
    WaylandRegister* mReg;
    bool mShouldClose;
    ReflectImageTrigger2* mTrigger;    

  public:
    bool isShouldClose() const { return mShouldClose; }

  public:
    WaylandCore( int width, int height, const char* title );
    WaylandCore();
    virtual ~WaylandCore();
    
    void waitEvents();
    void pollEvents();  
    void pollImgbufChange();
    void setFullscreen(bool enable);
    void setFillColor(int32_t col);
    void refrectBuffer();

    void on_resize(int width,int height);
  
  protected:
    void on_imgbuf_created(string path);
};

class WaylandRedrawable :public WaylandCore{
  public:
    WaylandRedrawable( int width, int height, const char* title );
    void redrawWindow();
    void startRedraw();
    void stopRedraw();
  protected:
    virtual bool on_redraw();
  private:
    bool mRedraw;
};

class SampleWaylandRedrawable :public WaylandRedrawable{
  using WaylandRedrawable::WaylandRedrawable;
  bool on_redraw() override;
};
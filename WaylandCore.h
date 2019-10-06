#pragma once

#include <iostream>
#include <string.h>
#include <stdint.h>
#include <wayland-client.h>

using namespace std;

struct WaylandRegister{
  wl_registry* mRegistry;
  wl_compositor* mCompositor;
  wl_shell*   mShell;
  wl_shm* mShm;  

  WaylandRegister(wl_display* display);
  ~WaylandRegister();
  void registry_listener_global( wl_registry* reg, uint32_t name, const char* interface, uint32_t version );
  void registry_listener_global_remove( wl_registry* reg, uint32_t name );
};

struct ImgBuf {
  wl_buffer*  buffer;
  void*       memory;
  int         size;
  int         width;
  int         height;
  string      filepath;

  ImgBuf(wl_shm* shm,int width, int height);
  ~ImgBuf();
  void fill(uint32_t val);
};

class WaylandCore {

  protected:
    wl_surface* mSurface;
    ImgBuf* mImgBuf;
    bool debug_print=true;
    bool mShouldClose;
    int32_t mFillColor;

  private:
    wl_display* mDisplay;
    wl_shell_surface* mShellSurface;
    WaylandRegister* mReg;

  public:
    bool isShouldClose() const { return mShouldClose; }

  public:
    WaylandCore( int width, int height, const char* title );
    WaylandCore();
    virtual ~WaylandCore();
    
    void waitEvents();
    void pollEvents();  
    void setFullscreen(bool enable);
    void setFillColor(int32_t col);
    void refrectBuffer();

    virtual void on_resize(int width,int height);
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
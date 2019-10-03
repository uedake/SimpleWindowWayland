#pragma once

#include <stdint.h>
#include <wayland-client.h>

using namespace std;

struct ImgBuf {
  wl_buffer*  buffer;
  void*       memory;
  bool        ready;
};

class WaylandCore {

protected:
  wl_surface* mSurface;
  struct ImgBuf mImgBuf;

private:
  wl_display* mDisplay;
  wl_registry* mRegistry;
  wl_compositor* mCompositor;
  wl_shell*   mShell;
  wl_shm* mShm;  
  wl_shell_surface* mShellSurface;

public:
  bool isShouldClose() const { return mShouldClose; }
  int  getWidth() const   { return mWidth; }
  int  getHeight() const  { return mHeight; }
  void setFillColor(int32_t col);

protected:
  int  mWidth, mHeight;
  bool mShouldClose;
  int32_t mFillColor;

public:
  WaylandCore( int width, int height, const char* title );
  WaylandCore();
  ~WaylandCore();
  
  void waitEvents();
  void pollEvents();  
  void setFullscreen(bool enable);

  void registry_listener_global( wl_registry* reg, uint32_t name, const char* interface, uint32_t version );
  void registry_listener_global_remove( wl_registry* reg, uint32_t name );

private:
  void createWindow( int width, int height, const char* title, bool fullscreen);  
  void setup_registry_handlers();    
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
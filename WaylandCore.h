#pragma once

#include <wayland-client.h>
#include "WaylandRegister.h"
#include "ImgBuf.h"

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
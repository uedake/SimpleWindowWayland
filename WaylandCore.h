#pragma once

#include <stdint.h>
#include <wayland-client.h>


class WaylandCore {
public:
  WaylandCore( int width, int height, const char* title );
  WaylandCore();
  ~WaylandCore();
  
  void waitEvents();
  void pollEvents();
  
  bool isShouldClose();

  int  getWidth() const   { return mWidth; }
  int  getHeight() const  { return mHeight; }
  void setFullscreen(bool enable);
  void redrawWindow();
  virtual void on_redraw();

private:
  void createWindow( int width, int height, const char* title );  
  void setup_registry_handlers();
  
public:
  void registry_listener_global( wl_registry* reg, uint32_t name, const char* interface, uint32_t version );
  void registry_listener_global_remove( wl_registry* reg, uint32_t name );

protected:
  int  mWidth, mHeight;
  bool mShouldClose;

  struct Surface {
    wl_surface* surface;
    wl_buffer*  buffer;
    void*       memory;
  } mSurface;

private:
  wl_display* mDisplay;
  wl_registry* mRegistry;
  wl_compositor* mCompositor;
  wl_shell*   mShell;
  wl_shm* mShm;
  wl_pointer* mPointer;
  wl_keyboard* mKeyboard;
  
  wl_shell_surface* mShellSurface;

};
class SampleWaylandCore :WaylandCore{
  void on_redraw() override;
};
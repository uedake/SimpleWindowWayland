#pragma once

#include <wayland-client.h>

struct WaylandRegister{
  wl_registry* registry;
  wl_compositor* compositor;
  wl_shell*   shell;
  wl_shm* shm;  

  WaylandRegister(wl_display* display);
  ~WaylandRegister();
  void registry_listener_global( wl_registry* reg, uint32_t name, const char* interface, uint32_t version );
  void registry_listener_global_remove( wl_registry* reg, uint32_t name );
};

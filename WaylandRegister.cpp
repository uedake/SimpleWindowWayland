#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>

#include "WaylandCore.h"

static void handler_global ( 
              void* data,
              wl_registry* reg, 
              uint32_t name, 
              const char* interface, 
              uint32_t version )
{
  WaylandRegister* core = static_cast<WaylandRegister*>(data);
  if( core ) {
    core->registry_listener_global( reg, name, interface, version );
  }
}

static void handler_global_remove(
              void* data,
              wl_registry* reg,
              uint32_t name )
{
  WaylandRegister* core = static_cast<WaylandRegister*>(data);
  if( core ) {
    core->registry_listener_global_remove( reg, name );
  }
}
  
static wl_registry_listener global_handlers = {
  handler_global, handler_global_remove
};

void WaylandRegister::WaylandRegister(wl_display* display)
{  
  registry = wl_display_get_registry( display );
  if( registry ) {
    wl_registry_add_listener( registry, &global_handlers, this );
    wl_display_dispatch( display );
    wl_display_roundtrip( display );
  }
}

WaylandRegister::~WaylandRegister(){
  if( shm ) {
    wl_shm_destroy( shm );
    shm = NULL;
  }
  if( compositor ) {
    wl_compositor_destroy( compositor );
    compositor = NULL;
  }
  if( registry ) {
    wl_registry_destroy( registry );
    registry = NULL;
  }
}

void WaylandRegister::registry_listener_global(
  wl_registry* reg, 
  uint32_t name, 
  const char* interface, 
  uint32_t version )
{
  void* obj = 0;
  if( strcmp( interface, "wl_compositor" ) == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_compositor_interface, 1 );
    compositor = static_cast<wl_compositor*>(obj);
    obj = NULL;
  }
  if( strcmp( interface, "wl_shm") == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_shm_interface, 1 );
    shm = static_cast<wl_shm*>(obj);
    obj = 0;
  }
  if( strcmp( interface, "wl_shell") == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_shell_interface, 1 );
    shell = static_cast<wl_shell*>(obj);
    obj = 0;
  }
}
 
void WaylandRegister::registry_listener_global_remove(
  wl_registry* reg, 
  uint32_t name )
{
}



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
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if( core ) {
    core->registry_listener_global( reg, name, interface, version );
  }
}
static void handler_global_remove(
              void* data,
              wl_registry* reg,
              uint32_t name )
{
  WaylandCore* core = static_cast<WaylandCore*>(data);
  if( core ) {
    core->registry_listener_global_remove( reg, name );
  }
}
  
static wl_registry_listener global_handlers = {
  handler_global, handler_global_remove
};

void WaylandCore::setup_registry_handlers()
{
  if( mDisplay == NULL ) {
    return;
  }
  
  mRegistry = wl_display_get_registry( mDisplay );
  if( mRegistry ) {
    wl_registry_add_listener( mRegistry, &global_handlers, this );
    wl_display_dispatch( mDisplay );
    wl_display_roundtrip( mDisplay );
  }
}

void WaylandCore::registry_listener_global(
  wl_registry* reg, 
  uint32_t name, 
  const char* interface, 
  uint32_t version )
{
  void* obj = 0;
  if( strcmp( interface, "wl_compositor" ) == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_compositor_interface, 1 );
    mCompositor = static_cast<wl_compositor*>(obj);
    obj = NULL;
  }
  if( strcmp( interface, "wl_shm") == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_shm_interface, 1 );
    mShm = static_cast<wl_shm*>(obj);
    obj = 0;
  }
  if( strcmp( interface, "wl_shell") == 0 ) {
    obj = wl_registry_bind( reg, name, &wl_shell_interface, 1 );
    mShell = static_cast<wl_shell*>(obj);
    obj = 0;
  }
}
 
void WaylandCore::registry_listener_global_remove(
  wl_registry* reg, 
  uint32_t name )
{
}



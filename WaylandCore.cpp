#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <linux/input.h>

#include "WaylandCore.h"

WaylandCore::WaylandCore( int width, int height, const char* title )
: mDisplay(NULL),mRegistry(NULL),mCompositor(NULL),mShm(NULL),
  mShouldClose(false),mWidth(0),mHeight(0),mFillColor(0xFF0000FF)
{
  mDisplay = wl_display_connect(NULL);
  setup_registry_handlers();//wait till registory is available
  
  if( mDisplay ) {
    mRegistry = wl_display_get_registry( mDisplay );
  }
  createWindow( width, height, title, false);
}

WaylandCore::~WaylandCore(){
  if( mShm ) {
    wl_shm_destroy( mShm );
    mShm = NULL;
  }
  if( mCompositor ) {
    wl_compositor_destroy( mCompositor );
    mCompositor = NULL;
  }
  if( mRegistry ) {
    wl_registry_destroy( mRegistry );
    mRegistry = NULL;
  }
  if( mDisplay ) {
    wl_display_flush( mDisplay );
    wl_display_disconnect( mDisplay );
    mDisplay = NULL;
  }
}

static void shell_surface_handler_ping(
  void *data, 
  struct wl_shell_surface *shell_surface,
  uint32_t serial )
{
	wl_shell_surface_pong( shell_surface, serial );
}
static void shell_surface_handler_configure(
  void *data, 
  struct wl_shell_surface *shell_surface,
  uint32_t edges, 
  int32_t width, 
  int32_t height )
{
}

static void shell_surface_handler_popup_done( void *data, struct wl_shell_surface *shell_surface )
{
}

static int create_shared_fd( int size ) {
  static const char fmt[] = "/weston-shared-XXXXXX";
  const char* path = NULL;
  char* name = NULL;
  int fd = -1;
  path = getenv("XDG_RUNTIME_DIR");
  if( !path ) {
    return -1;
  }
  name = (char*) malloc( strlen(path) + sizeof(fmt) );
  if( !name ) {
    return -1;
  }
  strcpy( name, path );
  strcat( name, fmt );
  
  fd = mkostemp( name, O_CLOEXEC );
  if( fd >= 0 ) {
    unlink(name);
  }
  free(name); name = NULL;
  if( ftruncate( fd, size ) < 0 ) {
    close(fd);
    return -1;
  }
  return fd;
}

static struct ImgBuf createBuffer(wl_shm* shm,int width, int height){
  struct ImgBuf imgbuf;
  int stride = width * sizeof(uint32_t);
  int size = stride * height;

  int fd = create_shared_fd( size );
  if( fd < 0 ) {
    return imgbuf;
  }
  void* image_ptr = mmap( NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );
  wl_shm_pool* pool = wl_shm_create_pool( shm, fd, size );
  memset( image_ptr, 0x00, size );
  wl_buffer* wb = wl_shm_pool_create_buffer( pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888 );
  wl_shm_pool_destroy( pool ); pool = NULL;

  imgbuf.buffer = wb;
  imgbuf.memory  = image_ptr;
  imgbuf.ready = true;
  
  return imgbuf;
}

static wl_shell_surface* createShellSurface(const char* title,wl_shell*   shell,wl_surface* surface,void * data){
  wl_shell_surface* shellSurface = wl_shell_get_shell_surface( shell, surface );
  wl_shell_surface_set_title( shellSurface, title );
  static wl_shell_surface_listener shell_surf_listeners = {
    shell_surface_handler_ping,
    shell_surface_handler_configure,
    shell_surface_handler_popup_done,
  };  
  wl_shell_surface_add_listener( shellSurface, &shell_surf_listeners, data );
  return shellSurface;
}

static void fill_buf(uint32_t val,void* mem,int width,int height){
  for(int y=0;y<height;++y) {
    uint8_t* p = static_cast<uint8_t*>( mem ) + width * y * sizeof(uint32_t);
    for(int x=0;x<width;++x) {
      reinterpret_cast<uint32_t*>(p)[x] = val;
    }
  }
}

void WaylandCore::createWindow( int width, int height, const char* title, bool fullscreen)
{
  if( mDisplay == NULL || mCompositor == NULL ) {
    throw "createWindow: unexpected error";
  }
  mWidth = width;
  mHeight = height;

  mSurface = wl_compositor_create_surface( mCompositor );
  mShellSurface = createShellSurface(title, mShell, mSurface, this);
  this->setFullscreen(fullscreen);

  mImgBuf = createBuffer(mShm,mWidth,mHeight);
  if(!mImgBuf.ready){
    throw "createWindow: failed to create buffer";
  }
  fill_buf(mFillColor,mImgBuf.memory,mWidth, mHeight);
  wl_surface_attach( mSurface, mImgBuf.buffer, 0, 0 );
  wl_surface_damage( mSurface, 0, 0, mWidth, mHeight );  
  wl_surface_commit( mSurface );
}

void WaylandCore::waitEvents() {
  if( mDisplay == NULL || mRegistry == NULL ) {
    mShouldClose = true;
    return;
  }
  int ret = wl_display_dispatch(mDisplay);
  if( ret == -1 ) {
    mShouldClose = true;
    return;
  }
  
  return;
}

void WaylandCore::pollEvents() {
  if( mDisplay == NULL || mRegistry == NULL ) {
    mShouldClose = true;
    return;
  }
  pollfd fds[] = {
    { wl_display_get_fd(mDisplay), POLLIN },
  };
  while( wl_display_prepare_read(mDisplay) != 0 )
  {
    wl_display_dispatch_pending(mDisplay);
  }
  wl_display_flush(mDisplay);
  if( poll(fds,1,0) > 0 )
  {
    wl_display_read_events(mDisplay);
    wl_display_dispatch_pending(mDisplay);
  } else {
    wl_display_cancel_read(mDisplay);
  }
}

void WaylandCore::setFullscreen(bool enable)
{
	if (enable) {
		wl_shell_surface_set_fullscreen(mShellSurface,
						WL_SHELL_SURFACE_FULLSCREEN_METHOD_FILL,
						0, NULL);
	} else {
		wl_shell_surface_set_toplevel(mShellSurface);
  }
}

WaylandRedrawable::WaylandRedrawable( int width, int height, const char* title )
  : WaylandCore(width,height,title){
  startRedraw();  
}
WaylandRedrawable::~WaylandRedrawable( int width, int height, const char* title )
  : ~WaylandCore(width,height,title){
}


static void frame_redraw( void* data, wl_callback* callback, uint32_t time )
{
  WaylandRedrawable* wl = static_cast<WaylandRedrawable*>(data);
  if(wl){
	  wl_callback_destroy( callback );  
    wl->redrawWindow();
  }
}

static wl_callback_listener frame_listeners = {
  frame_redraw,
};

static void setFrameCallback(wl_surface* surface,void * data){
  wl_callback* callback = wl_surface_frame( surface );
  wl_callback_add_listener( callback, &frame_listeners, data);
}

void WaylandRedrawable::startRedraw(){
  mRedraw=true;
  setFrameCallback(mSurface,this);
}

void WaylandRedrawable::stopRedraw(){
  mRedraw=false;
}

void WaylandRedrawable::redrawWindow()
{
  bool changed=on_redraw();
  if(mRedraw){ //set next frame callback
    startRedraw();
    if(changed)
      wl_surface_attach( mSurface, mImgBuf.buffer, 0, 0 );  
    wl_surface_commit( mSurface ); 
  }
}

bool WaylandRedrawable::on_redraw(){
  fill_buf(mFillColor,mImgBuf.memory,mWidth, mHeight);
  wl_surface_damage( mSurface, 0, 0, mWidth, mHeight );
  return true;
}

uint32_t calcColor()
{
  static int hue = 0;
  hue += 10;
  if( hue >= 360 ) { hue = 0; }
  
  float max = 1.0f;
  float min = 0.0f;
  float s = 1.0f;
  float v = 1.0f;
  
  int dh = hue / 60;
  float p = v * (1.0f - s);
  float q = v * (1.0f - s * (hue / 60.0f - dh) );
  float t = v * (1.0f - s * (1.0f - (hue / 60.0f - dh ) ) );
  
  int r,g,b;
  switch( dh ) {
  case 0: r = v*255; g = t*255; b = p*255; break;
  case 1: r = q*255; g = v*255; b = p*255; break;
  case 2: r = p*255; g = v*255; b = t*255; break;
  case 3: r = p*255; g = q*255; b = v*255; break;
  case 4: r = t*255; g = p*255; b = v*255; break;
  case 5: r = v*255; g = p*255; b = q*255; break;
  }
  return (r << 16) | (g << 8) | b;
}

bool SampleWaylandRedrawable::on_redraw(){
  static int HEIGHT = mHeight;
  HEIGHT -= 5;
  if( HEIGHT < 0 ) { HEIGHT = mHeight; }
  wl_surface_damage( mSurface, 0, 0, mWidth, HEIGHT );
  
  uint32_t val = calcColor();
  val |= 0xFF000000;
  for(int y=0;y<mHeight;++y) {
    uint8_t* p = static_cast<uint8_t*>( mImgBuf.memory ) + mWidth * y * sizeof(uint32_t);
    for(int x=0;x<mWidth;++x) {
      reinterpret_cast<uint32_t*>(p)[x] = val;
    }
  }
  return true;
}
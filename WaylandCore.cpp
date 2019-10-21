#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

#include <fstream>
#include <sys/stat.h>

#include "WaylandCore.h"

using namespace std;

ReflectImageTrigger::ReflectImageTrigger(WaylandCore* core,string dir_path,string rcv_file_name,string ack_file_name)
:FileSync(dir_path,rcv_file_name,ack_file_name){
    mCore=core;
}

void ReflectImageTrigger::on_receive(int counter){
    if(mCore)
      mCore->refrectBuffer();
}

ReflectImageTrigger2::ReflectImageTrigger2(WaylandCore* core,string dir_path,string rcv_file_name,string ack_file_name){
    mCore=core;

    rcv_fn=rcv_file_name;
    ack_fn=ack_file_name;

    rcv_path=dir_path+"/"+rcv_file_name;
    ack_path=dir_path+"/"+ack_file_name;

    ifstream ifs(rcv_path);
    if (!ifs)
    {
        cout << "cannot open file:" << rcv_path << ". create it." << endl;
        mode_t before=umask(0);
        int fd = creat(rcv_path.c_str(), 0666);
        if (fd == -1){
            throw "Exception";
        }
        close(fd);
        umask(before);
        ofstream ofs(rcv_path);
        ofs.fill( '0' );    
        ofs.width( 8 );
        ofs << 0;
        ofs.close();
        rcv=0;
    }
    else{
        string line;
        ifs >> line;
        ifs.close();
        rcv=stoi(line,nullptr,10);
    }
    cout << "rcv = " << rcv << endl;
}

int ReflectImageTrigger2::poll(){
    ifstream ifs(rcv_path);
    string line;
    ifs >> line;
    ifs.close();
    int r=stoi(line,nullptr,10);

    if(rcv<r){
        rcv=r;
        on_receive(r);
        ofstream ofs(ack_path);
        ofs.fill( '0' );    
        ofs.width( 8 );
        ofs << rcv;
        ofs.close();
    }
}

void ReflectImageTrigger2::on_receive(int counter){
    if(mCore)
      mCore->refrectBuffer();
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
  WaylandCore* wl = static_cast<WaylandCore*>(data);
  wl->on_resize(width,height);
}

static void shell_surface_handler_popup_done( void *data, struct wl_shell_surface *shell_surface )
{
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

WaylandCore::WaylandCore( int width, int height, const char* title )
: mImgBuf(NULL),mTrigger(NULL),mShouldClose(false),mFillColor(0xFF0000FF)
{
  mDisplay = wl_display_connect(NULL);
  mReg = new WaylandRegister(mDisplay);//wait till registory is available
  
  if( mDisplay == NULL || mReg->compositor == NULL ) {
    throw "WaylandCore: unexpected error";
  }

  mSurface = wl_compositor_create_surface( mReg->compositor );
  mShellSurface = createShellSurface(title, mReg->shell, mSurface, this);
  setFullscreen(false);
  on_resize(width,height);
  if(debug_print)
    cout << "WaylandCore created" << endl;
}

WaylandCore::~WaylandCore(){
  if(debug_print)
    cout << "WaylandCore start destroying" << endl;
  if(mTrigger){
    delete mTrigger;
    mTrigger = NULL;
  }
  if(mReg){
    delete mReg; 
    mReg = NULL;
  }
  if( mDisplay ) {
    wl_display_flush( mDisplay );
    wl_display_disconnect( mDisplay );
    mDisplay = NULL;
  }

  if(debug_print)
    cout << "WaylandCore try to delete mImgBuf" << endl;
  if( mImgBuf ) {
    delete mImgBuf;
    mImgBuf = NULL;
  }

  if(debug_print)
    cout << "WaylandCore destructed" << endl;
}

void WaylandCore::on_resize(int width,int height){
  if(debug_print)
    cout << "Resize requested: w="<< width << ",h="<<height<< endl;

  if(mImgBuf && mImgBuf->width==width && mImgBuf->height==height)
    return;

  try{
    if(mImgBuf){
      delete mImgBuf;
      mImgBuf = NULL;
    }
    if(debug_print)
      cout << "start creating buffer: w="<< width << ",h="<<height<< endl;
    mImgBuf = new ImgBuf(mReg->shm,width,height);
    setFillColor( mFillColor);
    on_imgbuf_created(mImgBuf->filepath);
  }
  catch(...){
    throw "WaylandCore: failed to create buffer";
  }
}

void WaylandCore::refrectBuffer(){
  wl_surface_attach( mSurface, mImgBuf->buffer, 0, 0 );
  wl_surface_damage( mSurface, 0, 0, mImgBuf->width, mImgBuf->height );  
  wl_surface_commit( mSurface );
}

void WaylandCore::setFillColor(int32_t col) {
  mFillColor=col;
  mImgBuf->fill(mFillColor);
  refrectBuffer();
}


void WaylandCore::waitEvents() {
  if( mDisplay == NULL || mReg->registry == NULL ) {
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

void WaylandCore::on_imgbuf_created(string file_path){
  try{
    size_t botDirPos = file_path.find_last_of("/");
    string dir = file_path.substr(0, botDirPos);
    string fn = file_path.substr(botDirPos+1);

    if(mTrigger){
      delete mTrigger;
    }
    mTrigger = new ReflectImageTrigger2(this,dir,fn+".rcv",fn+".ack");
  }
  catch(...){
    cerr << "cannot create triger" << endl;
  }
}

void WaylandCore::pollImgbufChange(){
  if( mDisplay == NULL || mReg->registry == NULL ) {
    mShouldClose = true;
    return;
  }
  if(mTrigger){
    mTrigger->poll();
  }
}
void WaylandCore::pollEvents() {
  if( mDisplay == NULL || mReg->registry == NULL ) {
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

//////////////////////////////////////////////////////////////////////////////////

WaylandRedrawable::WaylandRedrawable( int width, int height, const char* title )
  : WaylandCore(width,height,title){
  startRedraw();
  if(debug_print)
    cout << "WaylandRedrawable created" << endl;
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
  wl_surface_attach( mSurface, mImgBuf->buffer, 0, 0 );
  wl_surface_commit( mSurface ); 
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
      wl_surface_attach( mSurface, mImgBuf->buffer, 0, 0 );  
    wl_surface_commit( mSurface ); 
  }
}

bool WaylandRedrawable::on_redraw(){
  mImgBuf->fill(mFillColor);
  wl_surface_damage( mSurface, 0, 0, mImgBuf->width, mImgBuf->height );
  return true;
}

//////////////////////////////////////////////////////////////////////////////////

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
  static int HEIGHT = mImgBuf->height;
  HEIGHT -= 5;
  if( HEIGHT < 0 ) { HEIGHT = mImgBuf->height; }
  wl_surface_damage( mSurface, 0, 0, mImgBuf->width, HEIGHT );
  
  uint32_t val = calcColor();
  val |= 0xFF000000;
  for(int y=0;y<mImgBuf->height;++y) {
    uint8_t* p = static_cast<uint8_t*>( mImgBuf->memory ) + mImgBuf->width * y * sizeof(uint32_t);
    for(int x=0;x<mImgBuf->width;++x) {
      reinterpret_cast<uint32_t*>(p)[x] = val;
    }
  }
  return true;
}
#include <iostream>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "ImgBuf.h"

using namespace std;

static int create_shared_fd( int size, string path, bool keep_filename_visible=true) {
  int fd = open( path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC);
  if( fd >= 0 && !keep_filename_visible) {
    unlink(path.c_str()); //make other process cannot find filename to access the imgbuf
  }
  if( ftruncate( fd, size ) < 0 ) {
    close(fd);
    return -1;
  }
  return fd;
}

static int create_shared_fd_auto( int size, bool keep_filename_visible=false) {
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
  if( fd >= 0 && !keep_filename_visible) {
    unlink(name); //make other process cannot find filename to access the imgbuf
  }
  free(name); name = NULL;
  if( ftruncate( fd, size ) < 0 ) {
    close(fd);
    return -1;
  }
  return fd;
}

ImgBuf::ImgBuf(wl_shm* shm,int w, int h, string file_prefix){
  width=w;
  height=h;
  int stride = width * sizeof(uint32_t);
  size = stride * height;

  string filename = file_prefix+"ImgBuf_"+to_string(w) + "_" +to_string(h);
  char* dir = getenv("XDG_RUNTIME_DIR");
  if(dir==nullptr)
    throw "cannot create ImgBuf: XDG_RUNTIME_DIR is not set";
  filepath = dir + "/"s + filename;

  int fd = create_shared_fd( size ,filepath);

  if( fd < 0 ) {
    throw "cannot create ImgBuf: fail to create shared fd";
  }

  memory = mmap( NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );
  wl_shm_pool* pool = wl_shm_create_pool( shm, fd, size );
  memset( memory, 0x00, size );
  buffer = wl_shm_pool_create_buffer( pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888 );
  wl_shm_pool_destroy( pool ); pool = NULL;

  cout << "ImgBuf created: file path is " << filepath << endl;
}

ImgBuf::~ImgBuf(){
  cout << "ImgBuf try to unlink " << filepath << endl;
  unlink(filepath.c_str()); //make other process cannot find filename to access the imgbuf

  /*
  cout << "ImgBuf try to destroy buffer" << endl;
  if(buffer)
    wl_buffer_destroy(buffer);
  */

  cout << "ImgBuf try to unmap memory" << endl;
  munmap(memory, size);

  cout << "ImgBuf destroyed" << endl;
}

void ImgBuf::fill(uint32_t val){
  for(int y=0;y<height;++y) {
    uint8_t* p = static_cast<uint8_t*>( memory ) + width * y * sizeof(uint32_t);
    for(int x=0;x<width;++x) {
      reinterpret_cast<uint32_t*>(p)[x] = val;
    }
  }
}

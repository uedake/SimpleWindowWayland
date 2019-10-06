#pragma once

#include <wayland-client.h>

using namespace std;

struct ImgBuf {
  wl_buffer*  buffer;
  void*       memory;
  int         size;
  int         width;
  int         height;
  string      filepath;

  ImgBuf(wl_shm* shm,int width, int height,string file_prefix="");
  ~ImgBuf();
  void fill(uint32_t val);
};

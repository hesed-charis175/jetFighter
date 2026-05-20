#include "config.h"
#include <utility>

struct Size {
  int w, h;
};

struct Window {
  SDL_Window *win;
  SDL_GLContext ctx;
  Size size;

  Window(int width, int height, bool fullscreen = false);
  Size getSize();
  ~Window();
};

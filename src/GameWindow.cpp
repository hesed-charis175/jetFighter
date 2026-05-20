#include "GameWindow.h"
#include <SDL2/SDL_quit.h>
#include <SDL2/SDL_video.h>

Window::Window(int width, int height, bool fullscreen) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
  if (fullscreen)
    flags |= SDL_WINDOW_FULLSCREEN;
  win = SDL_CreateWindow("JetFighter", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, width, height, flags);
  ctx = SDL_GL_CreateContext(win);
  SDL_GL_SetSwapInterval(0);
  glewExperimental = GL_TRUE;
  glewInit();
  glEnable(GL_DEPTH_TEST);

  getSize();
}

Window::~Window() {
  SDL_GL_DeleteContext(ctx);
  SDL_DestroyWindow(win);
  SDL_Quit();
}

Size Window::getSize() {
  SDL_GetWindowSize(win, &size.w, &size.h);
  return size;
}

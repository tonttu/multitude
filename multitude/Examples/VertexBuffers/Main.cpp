#include <SDL/SDL.h>
#include <Luminous/Luminous.hpp>
#include <Luminous/VertexBuffer.hpp>

Luminous::VertexBuffer * vertexBuffer = 0;
Luminous::IndexBuffer * indexBuffer = 0;

void initBuffers()
{
  GLfloat vertices[][2] = {
    {1, 2},
    {0, 0},
    {2, 0},
  };

  vertexBuffer = new Luminous::VertexBuffer();
  vertexBuffer->fill(vertices, sizeof(vertices), Luminous::VertexBuffer::STATIC_DRAW);

  GLubyte indices[][3] = {
    {0, 1, 2},
  };

  indexBuffer = new Luminous::IndexBuffer();
  indexBuffer->fill(indices, sizeof(indices), Luminous::IndexBuffer::STATIC_DRAW);
}

int main(int , char ** )
{
  // Open a window
  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16 );
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1 );
  // SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1 );

  SDL_SetVideoMode(400 , 400, 0, SDL_OPENGL);

  Luminous::initLuminous();

  // Setup our buffer objects
  initBuffers();

  // Setup matrix stacks
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, 2, 0, 2, 0, 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  bool running = true;

  while(running) {
    SDL_Event event;

    if(SDL_PollEvent(&event)) {
      switch(event.type) {
        case SDL_QUIT:
          running = false;
          break;
      };
    }

    // Clear the window
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    // Setup vertex buffer and the pointer for it
    vertexBuffer->bind();
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, BUFFER_OFFSET(0));

    // Setup index buffer & draw
    indexBuffer->bind();
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));

    // Unbind the buffers (not needed here, but just for example...)
    indexBuffer->unbind();
    vertexBuffer->unbind();

    // Display the rendered buffer
    SDL_GL_SwapBuffers();
  }

  return 0;
}

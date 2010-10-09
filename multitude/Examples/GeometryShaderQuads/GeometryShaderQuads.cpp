
#include <Luminous/Luminous.hpp>

#include <SDL/SDL.h>

#include <Luminous/VertexBuffer.hpp>
#include <Luminous/GLResources.hpp>
#include <Luminous/Image.hpp>
#include <Luminous/Shader.hpp>
#include <Luminous/Utils.hpp>

#include <Nimble/Random.hpp>

#include <Radiant/Sleep.hpp>
#include <Radiant/Trace.hpp>

#include <Valuable/CmdParser.hpp>

using namespace Radiant;

class Item
{
public:
  Nimble::Vector2 pos;
  float size;
  float alpha;
};

int main(int /*argc*/, char ** /*argv*/)
{
  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16 );
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1 );

  Nimble::Vector2 size(1000, 600);

  SDL_SetVideoMode(size.x, size.y, 0, SDL_OPENGL);

  Luminous::initLuminous();

  glViewport(0, 0, size.x, size.y);

  Luminous::GLResources rsc(Radiant::ResourceLocator::instance());
  Luminous::GLResources::setThreadResources( & rsc, 0, 0);

  Luminous::GLSLProgramObject prog;

  Luminous::GLSLShaderObject * geoshader =
      Luminous::GLSLShaderObject::fromFile(GL_GEOMETRY_SHADER_EXT, "shader-quads.gs");
  Luminous::GLSLShaderObject * vertexhader =
      Luminous::GLSLShaderObject::fromFile(GL_VERTEX_SHADER, "shader-quads.vs");
  Luminous::GLSLShaderObject * pixelshader =
      Luminous::GLSLShaderObject::fromFile(GL_FRAGMENT_SHADER, "shader-quads.ps");

  if(!geoshader || !vertexhader || !pixelshader) {
    error("Shader compilation failed.");
    return -1;
  }

  prog.addObject(pixelshader);
  prog.addObject(geoshader);
  prog.addObject(vertexhader);

  bool stop = false;


  prog.setProgramParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS);
  Luminous::Utils::glCheck("Creating the geometry shader 1");
  prog.setProgramParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
  prog.setProgramParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 6);

  Luminous::Utils::glCheck("Creating the geometry shader");

  const int n = 40000;
  std::vector<Item> items(n);

  Nimble::Rectf rect(Nimble::Vector2(0,0), size);

  for(int i = 0; i < n; i++) {
    items[i].pos   = Nimble::RandomUniform::instance().randVec2InRect(rect);
    items[i].size  = Nimble::RandomUniform::instance().randMinMax(5, 20);
    items[i].alpha = Nimble::RandomUniform::instance().randMinMax(0.01f, 0.1f);
  }

  Luminous::VertexBuffer vbo;
  vbo.fill( & items[0], n * sizeof(Item), Luminous::VertexBuffer::DYNAMIC_DRAW);

  if(!prog.link()) {
    error("When linking program: %s", prog.linkerLog());
    return -1;
  }

  prog.bind();

  prog.setUniformVector2("vsiz", size);

  int ppos = prog.getAttribLoc("pos");
  int psiz = prog.getAttribLoc("size");
  int palp = prog.getAttribLoc("alpha");

  info("Attribute locations: %d %d %d", ppos, psiz, palp);

  Luminous::Utils::glUsualBlend();

  Radiant::TimeStamp begin(Radiant::TimeStamp::getTime());

  int frames = 0;

  for( ; !stop; frames++) {
    SDL_Event event;

    if(SDL_PollEvent(&event)) {
      switch(event.type) {
      case SDL_QUIT:
        Radiant::info("Quit called, stopping now");
        stop = true;
      };
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, size.x, 0, size.y, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor4f(1, 1, 1, 1);


    for(int i = 0; i < n; i++) {
      items[i].pos = Nimble::RandomUniform::instance().randVec2InRect(rect);
    }

    vbo.fill( & items[0], n * sizeof(Item), Luminous::VertexBuffer::DYNAMIC_DRAW);

    prog.bind();
    vbo.bind();
    glEnableVertexAttribArray(ppos);
    glEnableVertexAttribArray(psiz);
    glEnableVertexAttribArray(palp);

    glVertexAttribPointer(ppos, 2, GL_FLOAT, GL_FALSE, sizeof(Item), BUFFER_OFFSET(0));
    glVertexAttribPointer(psiz, 1, GL_FLOAT, GL_FALSE, sizeof(Item), BUFFER_OFFSET(sizeof(float) * 2));
    glVertexAttribPointer(palp, 1, GL_FLOAT, GL_FALSE, sizeof(Item), BUFFER_OFFSET(sizeof(float) * 3));

    glDrawArrays(GL_POINTS, 0, n);

    glDisableVertexAttribArray(ppos);
    glDisableVertexAttribArray(psiz);
    glDisableVertexAttribArray(palp);

    vbo.unbind();

    prog.unbind();

    Luminous::Utils::glCheck("After rendering a frame");

    SDL_GL_SwapBuffers();
  }

  float fps = frames / begin.sinceSecondsD();
  info("Rendered %d quads per frames, %.2f fps ", n, fps);

  return 0;
}

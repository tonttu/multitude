#import "Window.hpp"

namespace Posh
{

  class Window::D {
  public:
    D() {
      window = nil;
    }

    id window;
    id pixelFormat;
    id context;

    WindowEventHook * eventHook;
  };

  Window::Window()
    : m_d(new D())
  {}

  Window::~Window()
  {
    delete m_d;
  }

  void Window::makeCurrent()
  {
    [m_d->context makeCurrentContext];
  }

  void Window::swapBuffers()
  {

  }

  void Window::eventHook()
  {
    return m_d->eventHook;
  }

}

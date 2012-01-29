#ifdef RADIANT_OSX_LION

#import <Cocoa/Cocoa.h>
#import "CocoaWindow.hpp"

@interface CocoaView : NSOpenGLView
{
  int colorBits;
  int depthBits;
  bool runningFullScreen;
  NSTimer* timer;

  NSOpenGLContext context;

  ThreadedRendering::CocoaWindow * m_window;
}

- (id) initWithFrame:(NSRect)frame
colorBits:(int)numColorBits depthBits:(int)numDepthBits fullscreen:(bool)runFullScreen
m_window:(ThreadedRendering::CocoaWindow *)parent;

- (void) dealloc;

- (NSOpenGLPixelFormat *) createPixelFormat:(NSRect)frame;

- (BOOL)acceptsFirstResponder;
- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent;

- (bool) isFullScreen;

- (void) keyDown:(NSEvent *)theEvent;
- (void) keyUp:(NSEvent *)theEvent;
- (void) mouseDown:(NSEvent *)theEvent;
- (void) mouseUp:(NSEvent *)theEvent;
- (void) rightMouseDown:(NSEvent *)theEvent;
- (void) rightMouseUp:(NSEvent *)theEvent;
- (void) mouseDragged:(NSEvent *)theEvent;
- (void) scrollWheel:(NSEvent *)theEvent;
- (void) mouseMoved:(NSEvent *)theEvent;

- (void) hideCursor:(NSTimer *)theTimer;

@end

@implementation CocoaView

- (id) initWithFrame:(NSRect)frame
colorBits:(int)numColorBits depthBits:(int)numDepthBits fullscreen:(bool)runFullScreen
m_window:(ThreadedRendering::CocoaWindow *)parent;
{
NSOpenGLPixelFormat *pixelFormat;

m_window = parent;
colorBits = numColorBits;
depthBits = numDepthBits;
runningFullScreen = runFullScreen;

pixelFormat = [self createPixelFormat:frame];

if(pixelFormat != nil)
{
  self = [super initWithFrame:frame pixelFormat:pixelFormat];
  [pixelFormat release];

  if(self)
  {
    [ [self openGLContext] makeCurrentContext];
    if(runningFullScreen) {
    }
  }
}
else {
Radiant::error("Failed to create View");
self = nil;
}

return self;

}

- (void) dealloc
{
  [super dealloc];
}

- (BOOL)acceptsFirstResponder
{
  return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
  (void) theEvent;
  return YES;
}

- (NSOpenGLPixelFormat *) createPixelFormat:(NSRect)frame
{
  (void) frame;

  NSOpenGLPixelFormatAttribute attributes [16];

  NSOpenGLPixelFormat *pixelFormat;

  int index = 0;

  attributes[ index++ ] = NSOpenGLPFADoubleBuffer;
  attributes[ index++ ] = NSOpenGLPFAAccelerated;
  attributes[ index++ ] = NSOpenGLPFAColorSize;
  attributes[ index++ ] = colorBits;
  attributes[ index++ ] = NSOpenGLPFADepthSize;
  attributes[ index++ ] = depthBits;

  attributes[index] = 0;
  pixelFormat = [ [NSOpenGLPixelFormat alloc] initWithAttributes:attributes];

  return pixelFormat;
}

- (bool) isFullScreen
{
  return runningFullScreen;
}

- (void) keyDown:(NSEvent *)theEvent
{
  ThreadedRendering::WindowEventHook * hook = m_window->eventHook();
  if(!hook) {
    Radiant::error("Cannot obtain WindowEventHook");
    return;
  }

  unichar key;
  key = [ [ theEvent characters ] characterAtIndex:0 ];

  bool repeat = false;

  if([theEvent isARepeat])
    repeat = true;

  switch(key) {

  //esc
  case 27 :
    if( [[self window] styleMask] & NSFullScreenWindowMask) {
      [[self window] toggleFullScreen:nil];
    }
    break;

  default:
    hook->handleKeyboardEvent(key, true, 0, repeat);
    break;
  };

}

-(void) keyUp:(NSEvent *)theEvent
{
  ThreadedRendering::WindowEventHook * hook = m_window->eventHook();
  if(!hook) {
    Radiant::error("Cannot obtain WindowEventHook");
    return;
  }

  unichar key;
  key = [ [ theEvent characters ] characterAtIndex:0 ];

  switch(key) {

  //esc
  case 27 :


    break;

  default:
    hook->handleKeyboardEvent(key, false, 0, false);
    break;
  };
}

-(void) mouseDown:(NSEvent *)theEvent
{
  ThreadedRendering::WindowEventHook * hook = m_window->eventHook();
  if(!hook) {
    Radiant::error("Cannot obtain WindowEventHook");
    return;
  }

  ThreadedRendering::WindowEventHook::MouseButtonMask button =
      ThreadedRendering::WindowEventHook::NoButton;

  int buttonNumber = [theEvent buttonNumber];

  if(buttonNumber == 0)
    button = ThreadedRendering::WindowEventHook::LeftButton;

  NSPoint location;
  location = [theEvent locationInWindow];

  float x = location.x;
  //invert y
  float y = [self frame].size.height - location.y;

  hook->handleMouseButton(button, x, y, true);

  [timer invalidate];
  timer = [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(hideCursor:) userInfo:nil repeats:NO];
}

-(void) mouseUp:(NSEvent *)theEvent
{
  ThreadedRendering::WindowEventHook * hook = m_window->eventHook();
  if(!hook) {
    Radiant::error("Cannot obtain WindowEventHook");
    return;
  }

  ThreadedRendering::WindowEventHook::MouseButtonMask button =
      ThreadedRendering::WindowEventHook::NoButton;

  int buttonNumber = [theEvent buttonNumber];

  if(buttonNumber == 0)
    button = ThreadedRendering::WindowEventHook::LeftButton;

  NSPoint location;
  location = [theEvent locationInWindow];

  float x = location.x;
  //invert y
  float y = [self frame].size.height - location.y;

  hook->handleMouseButton(button, x, y, false);

  [timer invalidate];
  timer = [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(hideCursor:) userInfo:nil repeats:NO];
}

-(void) rightMouseDown:(NSEvent *)theEvent
{
  ThreadedRendering::WindowEventHook * hook = m_window->eventHook();
  if(!hook) {
    Radiant::error("Cannot obtain WindowEventHook");
    return;
  }


  ThreadedRendering::WindowEventHook::MouseButtonMask button =
      ThreadedRendering::WindowEventHook::NoButton;

  int buttonNumber = [theEvent buttonNumber];

  if(buttonNumber == 1)
    button = ThreadedRendering::WindowEventHook::RightButton;

  NSPoint location;
  location = [theEvent locationInWindow];

  float x = location.x;
  //invert y
  float y = [self frame].size.height - location.y;

  hook->handleMouseButton(button, x, y, true);

  [timer invalidate];
  timer = [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(hideCursor:) userInfo:nil repeats:NO];
}

-(void) rightMouseUp:(NSEvent *)theEvent
{
  ThreadedRendering::WindowEventHook * hook = m_window->eventHook();
  if(!hook) {
    Radiant::error("Cannot obtain WindowEventHook");
    return;
  }


  ThreadedRendering::WindowEventHook::MouseButtonMask button =
      ThreadedRendering::WindowEventHook::NoButton;

  int buttonNumber = [theEvent buttonNumber];

  if(buttonNumber == 1)
    button = ThreadedRendering::WindowEventHook::RightButton;

  NSPoint location;
  location = [theEvent locationInWindow];

  float x = location.x;
  //invert y
  float y = [self frame].size.height - location.y;

  hook->handleMouseButton(button, x, y, false);

  [timer invalidate];
  timer = [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(hideCursor:) userInfo:nil repeats:NO];

}



-(void) mouseDragged:(NSEvent *)theEvent
{
  ThreadedRendering::WindowEventHook * hook = m_window->eventHook();
  if(!hook) {
    Radiant::error("Cannot obtain WindowEventHook");
    return;
  }

  NSPoint location;
  location = [theEvent locationInWindow];

  float x = location.x;
  //invert y
  float y = [self frame].size.height - location.y;

  hook->handleMouseMove(x, y, ThreadedRendering::WindowEventHook::LeftButton);

  [timer invalidate];
  timer = [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(hideCursor:) userInfo:nil repeats:NO];
}

-(void) mouseMoved:(NSEvent *)theEvent
{
 (void) theEvent;
 [timer invalidate];
 timer = [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(hideCursor:) userInfo:nil repeats:NO];
}

-(void) scrollWheel:(NSEvent *)theEvent
{

  (void) theEvent;

  ThreadedRendering::WindowEventHook * hook = m_window->eventHook();
  if(!hook) {
    Radiant::error("Cannot obtain WindowEventHook");
    return;
  }

  ThreadedRendering::WindowEventHook::MouseButtonMask button =
      ThreadedRendering::WindowEventHook::NoButton;

  float delta = [theEvent scrollingDeltaY];

  if( delta > 0.0 )
    button = ThreadedRendering::WindowEventHook::WheelForward;
  else button = ThreadedRendering::WindowEventHook::WheelBackward;

  NSPoint loc;
  loc = [theEvent locationInWindow];
  float x = loc.x;
  float y = loc.y;

  hook->handleMouseButton(button, x, y, true);

  [timer invalidate];
  timer = [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(hideCursor:) userInfo:nil repeats:NO];
}

-(void) hideCursor:(NSTimer *)theTimer
{
 (void) theTimer;
 [NSCursor setHiddenUntilMouseMoves:YES];
 timer = nil;
}
@end

@interface CocoaWindow : NSWindow
{
}

-(BOOL) canBecomeKeyWindow;
-(BOOL) acceptsFirstResponder;

@end

@implementation CocoaWindow

-(BOOL) canBecomeKeyWindow { return YES; }
-(BOOL) acceptsFirstResponder { return YES; }

@end



@interface Controller : NSWindowController
{

  CocoaWindow *glWindow;

  CocoaView *glView;

  ThreadedRendering::CocoaWindow * m_window;

  const ThreadedRendering::WindowConfig * m_hint;

}
- (Controller *) initialize:(ThreadedRendering::CocoaWindow *)parent
                           :(const ThreadedRendering::WindowConfig &)hint;

- (void) dealloc;

- (CocoaView*) glView;

- (void) newWindow;

- (BOOL) shouldCascadeWindows;
- (BOOL) canBecomeFirstResponder;

@end

@implementation Controller

- (Controller *) initialize:(ThreadedRendering::CocoaWindow *)parent
                           :(const ThreadedRendering::WindowConfig &)hint
{

  m_window = parent;
  m_hint = & hint;

  [self performSelectorOnMainThread:@selector(newWindow) withObject:nil waitUntilDone:true];

  return self;
}

- (BOOL) shouldCascadeWindows
{
  return NO;
}

- (BOOL) canBecomeFirstResponder
{
  return YES;
}

- (void) newWindow
{
  int display = m_hint->display.toUtf8().data()[1] - '0'; // first number after colon

  unsigned int styleMask = 0;

  if(m_hint->frameless)
    styleMask |= NSBorderlessWindowMask;
  else if(!m_hint->fullscreen)
    styleMask |= NSTitledWindowMask;

  NSScreen * screen = [[NSScreen screens] objectAtIndex:display];
  NSRect displayRect = [screen frame];

  displayRect.origin.x = m_hint->x;
  displayRect.origin.y = displayRect.size.height - m_hint->y - m_hint->height;
  displayRect.size.width = m_hint->width;
  displayRect.size.height = m_hint->height;
  glWindow = [[CocoaWindow alloc] initWithContentRect: displayRect styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];

  [glWindow setHasShadow:NO];

  [glWindow setAcceptsMouseMovedEvents:YES];

  glView = [ [CocoaView alloc] initWithFrame:[glWindow frame]
      colorBits:8 depthBits:8 fullscreen:m_hint->fullscreen
      m_window:m_window];

  if(glView != nil)
  {
    [glWindow setContentView:glView];
    [glWindow makeKeyAndOrderFront:glView];
  } else
  {
    Radiant::info("NSOpenGLView creation failed");
  }

  // fullscreen button
  if(!m_hint->fullscreen)
    [glWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
  else {
    [glWindow setLevel:CGShieldingWindowLevel()]; //go on top of everything if fullscreen
    [glWindow makeFirstResponder:glView];
  }


}

- (void) dealloc
{
  [glView release];
  [glWindow release];

  [super dealloc];
}

- (CocoaView*) glView { return glView; }


@end


namespace ThreadedRendering
{


class CocoaWindow::D {
public:
  D(CocoaWindow * window, const WindowConfig & hint) : m_window(window) {

    [NSApp activateIgnoringOtherApps:YES]; // get keyboard focus
    controller = [[Controller alloc] initialize:m_window:hint ];
  }

  ~D()
  {
    [controller release];
  }

  Controller * controller;

  CocoaWindow * m_window;

};

CocoaWindow::CocoaWindow(const WindowConfig & hint)
  : m_d(new D(this, hint))
{
}

CocoaWindow::~CocoaWindow()
{
  delete m_d;
}

void CocoaWindow::poll()
{

}

void CocoaWindow::makeCurrent()
{
  CocoaView * view = [m_d->controller glView];
  NSOpenGLContext * context = [view openGLContext];
  [context makeCurrentContext];
}

void CocoaWindow::swapBuffers()
{
  CocoaView * view = [m_d->controller glView];
  NSOpenGLContext * context = [view openGLContext];
  [context flushBuffer];
}

void CocoaWindow::minimize()
{
  Radiant::error("CocoaWindow::minimize # unimplemented");
}

void CocoaWindow::restore()
{
  Radiant::error("CocoaWindow::restore # unimplemented");
}

}

#endif

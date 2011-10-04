#import <Cocoa/Cocoa.h>

#include <Luminous/MultiHead.hpp>

struct
{
  id autoreleasePool;
} g_platform;

struct
{
  id window;
  id pixelFormat;
  id context;
} g_window;

int poshPlatformInit()
{
  g_platform.autoreleasePool = [[NSAutoreleasePool alloc] init];

  // Create application instance
  [NSApplication sharedApplication];

  [NSApp finishLaunching];

  return 0;
}

int poshPlatformTerminate()
{
  // Free the auto-release pool
  [g_platform.autoreleasePool release];
  g_platform.autoreleasePool = nil;

  return 0;
}

int poshWindowCreate(const Luminous::MultiHead::Window & wc)
{
  unsigned int styleMask = NSBorderlessWindowMask;

  g_window.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(wc.location().x, wc.location().y, wc.size().x, wc.size().y) styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];

  // Select pixel format
  std::vector<NSOpenGLPixelFormatAttribute> attributes;

  if(wc.fullscreen()) {
    attributes.push_back(NSOpenGLPFAFullScreen);
    attributes.push_back(NSOpenGLPFANoRecovery);
    attributes.push_back(NSOpenGLPFAScreenMask);
    attributes.push_back(CGDisplayIDToOpenGLDisplayMask(CGMainDisplayID()));
  }

  attributes.push_back(NSOpenGLPFAColorSize);
  attributes.push_back(24);

  attributes.push_back(NSOpenGLPFAAlphaSize);
  attributes.push_back(8);

  attributes.push_back(NSOpenGLPFADepthSize);
  attributes.push_back(24);

  attributes.push_back(0);

  g_window.pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes.data()];
  if(g_window.pixelFormat == nil)
    return -1;

  // Create context
  g_window.context = [[NSOpenGLContext alloc] initWithFormat:g_window.pixelFormat shareContext:nil];
  if(g_window.context == nil)
    return -1;

  [g_window.window makeKeyAndOrderFront:nil];
  [g_window.context setView:[g_window.window contentView]];

  // Make current
  [g_window.context makeCurrentContext];

  return 0;
}


#include <Foundation/NSPathUtilities.h>
#include <Foundation/NSBundle.h>

#include <QString>


namespace Radiant
{

namespace PlatformUtils
{

QString getExecutablePath()
{
#if 1
  NSBundle *b = [NSBundle mainBundle];
  NSString *dir = [b resourcePath];
  return QString([dir fileSystemRepresentation]);
#else
  NSString * str = NSHomeDirectory();
  return QString([str fileSystemRepresentation]);
#endif
}

}
}

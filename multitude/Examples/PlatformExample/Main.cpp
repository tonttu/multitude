
#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Trace.hpp>

int main()
{
  Radiant::info("Application path = %s",
		Radiant::PlatformUtils::getExecutablePath().c_str());
  Radiant::info("Application system-wide resource dir = %s",
		Radiant::PlatformUtils::getModuleGlobalDataPath
		("MultiTude/PlatformTest", true).c_str());
  Radiant::info("Application user-specific resource dir = %s",
		Radiant::PlatformUtils::getModuleUserDataPath
		("MultiTude/PlatformTest", true).c_str());
  Radiant::info("Application memory footprint = %lld", (long long)
		Radiant::PlatformUtils::processMemoryUsage());
}

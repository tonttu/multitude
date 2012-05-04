-- Include the Windows specific stuff
includedirs { "." }

configuration { "x64", "windows" }
  libdirs { "Win64x/lib64" }
  includedirs { "Win64x/include" }
configuration { "x32", "windows" }
  libdirs { "Win32x/lib32" }
  includedirs { "Win32x/include" }
configuration {}

include "Patterns"
include "Nimble"
include "Radiant"
include "Valuable"
include "Poetic"
include "Resonant"
include "VideoDisplay"
include "Squish"
include "Luminous"
include "Screenplay"
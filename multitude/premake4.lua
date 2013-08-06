-- Include the Windows specific stuff
includedirs { "." }

--
-- Platform
--
configuration { "windows", "x64" }
  libdirs { "Win64x/lib64" }
  includedirs { "Win64x/include" }
configuration { "windows", "x32" }
  libdirs { "Win32x/lib32" }
  includedirs { "Win32x/include" }
configuration {}

include "Box2D"
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

include "Applications"

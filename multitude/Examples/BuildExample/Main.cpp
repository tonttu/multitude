/* Just include random files to see that it is working correctly. */
#include <Poetic/GPUFont.hpp>

#include <Luminous/Utils.hpp>

#include <Nimble/Vector4.hpp>

#include <Radiant/Thread.hpp>

#include <Resonant/ModuleGain.hpp>

#include <Screenplay/VideoFFMPEG.hpp>

#include <ValueIO/HasValues.hpp>

int main()
{
  Nimble::Vector2 v1(0,0);
  Nimble::Vector2 v2(1,1);

  for(int i = 0; i < 10; i++)
    v1 += v2;

  return 0;
}

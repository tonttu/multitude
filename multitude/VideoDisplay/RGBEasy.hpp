#ifndef RGBEASY_HPP
#define RGBEASY_HPP

#include "WindowsVideoHelpers.hpp"

#include <atomic>
#include <QLibrary>
#include <windows.h>

#ifdef RGBEASY
#include <RGB.H>

namespace VideoDisplay
{

  struct RGBEasyAPI
  {
    // The RGBEasy library comes with a file RGBAPI.H which just have invocations
    // to API-macro, which is undefined by default. The file has lines like this:
    //
    //   API ( unsigned long, RGBAPI, RGBGetNumberOfInputs, ( unsigned long  *pNumberOfInputs )
    //
    // It's up to whoever #includes that file to define that API-macro.
    //
    // Here we will declare function pointers with correct type and name,
    // and define the default value for those pointers to null.
    //
    // For example, it will expand the previous example to this:
    //
    //   unsigned long (RGBAPI *RGBGetNumberOfInputs) ( unsigned long  *pNumberOfInputs ) = nullptr;
    //
    // This API-struct will neatly contain all functions that the RGB API defines.
    // This is needed since we are loading the library dynamically.
    #define API(TYPE, CONV, NAME, ARGS) TYPE (CONV * NAME) ARGS = nullptr;
    #include <RGBAPI.H>
 };

  struct RGBEasySource : public Source
  {
    RGBEasySource(const VideoInput& vi, const AudioInput& ai)
      : Source(vi,ai)
    {
      assert(vi.rgbIndex >= 0);
    }

    virtual SourceState update() override;

    bool failed = false;
  };

  extern struct RGBEasyLib
  {
    RGBEasyLib();
    ~RGBEasyLib();

    void loadDll();

    HRGBDLL apiHandle = 0;
    RGBEasyAPI api;

    QLibrary rgbDll{"rgbeasy"};

    int getRGBIndex(const VideoInput& video);

    bool isEasyRGBSource(const VideoInput& video);

    std::unique_ptr<RGBEasySource>
    createEasyRGBSource(const VideoInput& videoInput, const AudioInput& audioInput);

    void initInput(VideoInput& vi);
    int possibleInputs();

    float score(const VideoInput& vi, const AudioInput& ai);

  } easyrgb;


} // namespace VideoDisplay

#endif // RGBEASY

#endif // RGBEASY_HPP
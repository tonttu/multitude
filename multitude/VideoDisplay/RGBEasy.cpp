#include "RGBEasy.hpp"

#include <Radiant/Mutex.hpp>

#include <QtGlobal>
#include <QRegularExpression>

namespace VideoDisplay
{

  RGBEasyLib easyrgb;

  SourceState RGBEasySource::update()
  {
    SourceState state;
    state.enabled = false;

    SIGNALTYPE signalType = RGB_SIGNALTYPE_NOSIGNAL;
    unsigned long w = 0, h = 0, rate = 0;

    if (easyrgb.api.RGBGetInputSignalType(video.rgbIndex, &signalType, &w, &h, &rate) != 0) {
      if(!failed) {
        Radiant::warning("RGBEasyMonitor # RGBGetInputSignalType failed");
        failed = true;
      }
    } else {
      failed = false;
      state.resolution.make(w, h);
      state.enabled = signalType != RGB_SIGNALTYPE_NOSIGNAL &&
                      signalType != RGB_SIGNALTYPE_OUTOFRANGE;
    }
    return state;
  }


  RGBEasyLib::RGBEasyLib()
  {
  }

  RGBEasyLib::~RGBEasyLib()
  {
    if (apiHandle) {
      api.RGBFree(apiHandle);
    }

    if (rgbDll.isLoaded()) {
      rgbDll.unload();
    }
  }

  void RGBEasyLib::loadDll()
  {
    if(rgbDll.isLoaded())
      return;

    // Don't fail even if RGB wouldn't be available
    if(!rgbDll.load()) {
      Radiant::error("RGBEasyMonitor # %s", rgbDll.errorString().toUtf8().data());
      return;
    }

    // See also the comment in the API-struct.
    //
    // Here we will assign a proper function pointers to that struct. This
    // functions-array will contain pairs, the function pointer inside our
    // struct, and the name of that function. For instance:
    //
    //  { (QFunctionPointer*) &easyrgb.api.RGBGetNumberOfInputs, "RGBGetNumberOfInputs" }
    //
    // Now when we know the name of the function and the target function
    // pointer, we can dynamically load the symbol from the library and assign
    // it to the function pointer.
    std::pair<QFunctionPointer*, const char*> functions[] = {
#define API(TYPE, CONV, NAME, ARGS) { (QFunctionPointer*)&easyrgb.api.NAME, #NAME },
#include <RGBAPI.H>
    };

    for (auto p: functions) {
      *p.first = rgbDll.resolve(p.second);
      if (!*p.first) {
        Radiant::error("RGBEasyMonitor # Failed to resolve %s: %s", p.second,
                       rgbDll.errorString().toUtf8().data());
        return;
      }
    }

    // RGBAPI functions will typically return an error code, 0 meaning success

    if (api.RGBLoad(&easyrgb.apiHandle) != 0) {
      apiHandle = 0;
      Radiant::error("RGBEasyMonitor # Failed to initialize RGB driver");
      return;
    }
  }

  int RGBEasyLib::getRGBIndex(const VideoInput& vi)
  {
    static QRegularExpression numberRe("\\b0*(\\d+)\\b",
                                       QRegularExpression::OptimizeOnFirstUsageOption);
    QRegularExpressionMatch match = numberRe.match(vi.friendlyName);
    if(!match.hasMatch()) {
      return -1;
    } else {
      return match.captured(1).toInt() - 1;
    }
  }

  void RGBEasyLib::initInput(VideoInput& vi)
  {
    if(possibleInputs() == 0)
      return;

    int rgbIndex = getRGBIndex(vi);
    RGBINPUTINFOA info;
    memset(&info, 0, sizeof(info));
    info.Size = sizeof(info);
    if (api.RGBGetInputInfoA(rgbIndex, &info) != 0) {
      return;
    }

    QString deviceName = info.DeviceName;
    if(!vi.friendlyName.contains(deviceName)) {
      return;
    }

    vi.rgbIndex = rgbIndex;
    vi.rgbDeviceName = deviceName;
  }

  int RGBEasyLib::possibleInputs()
  {
    static unsigned long numberOfInputs = 0;
    MULTI_ONCE {
      if (apiHandle && api.RGBGetNumberOfInputs(&numberOfInputs) != 0) {
        Radiant::error("RGBEasyMonitor # Failed to get the number of inputs");
      }
    }
    return numberOfInputs;
  }

  float RGBEasyLib::score(const VideoInput& vi, const AudioInput& ai)
  {
    assert(vi.rgbIndex >= 0);

    const QString& audioName = ai.friendlyName;
    QRegularExpression re(QString("\\b0*%1\\b").arg(vi.rgbIndex + 1));
    if(audioName.contains(vi.rgbDeviceName) && re.match(audioName).hasMatch())
      return 100000; /// We are fairly sure that this is a perfect match
    return 0;
  }

  std::unique_ptr<RGBEasySource>
  RGBEasyLib::createEasyRGBSource(const VideoInput& videoInput,
                                  const AudioInput& audioInput)
  {
    /// Basically this is only needed to override update-logic
    std::unique_ptr<RGBEasySource> src;
    src.reset(new RGBEasySource(videoInput, audioInput));
    return src;
  }

}

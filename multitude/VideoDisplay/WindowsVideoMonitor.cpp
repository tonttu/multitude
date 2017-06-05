#include "VideoCaptureMonitor.hpp"

#include <Shlwapi.h>
#include <windows.h>
#include <dshow.h>
#include <propvarutil.h>

#include <QLibrary>

#ifdef RGBEASY
#include <RGB.H>
#endif

namespace VideoDisplay
{
  // Functions for querying av capture devices

  QStringList scanAVInputDevices(GUID guid)
  {
    ICreateDevEnum * sysDevEnum = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum, (void **)&sysDevEnum);
    QStringList devices;
    if (FAILED(hr))
      return devices;

    IEnumMoniker * enumCat = nullptr;
    if (sysDevEnum->CreateClassEnumerator(guid, &enumCat, 0) == S_OK) {
      IMoniker * moniker = nullptr;
      while (enumCat->Next(1, &moniker, nullptr) == S_OK) {
        IPropertyBag * propBag = nullptr;
        if (SUCCEEDED(moniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&propBag))) {
          VARIANT varName;
          VariantInit(&varName);
          if (SUCCEEDED(propBag->Read(L"FriendlyName", &varName, 0)) && IsVariantString(varName)) {
            WCHAR name[256];
            VariantToString(varName, name, sizeof(name));
            devices << QString::fromWCharArray(name);
          }
          VariantClear(&varName);

          propBag->Release();
        }
      }
      moniker->Release();
    }
    if(enumCat)
      enumCat->Release();
    sysDevEnum->Release();
    return devices;
  }

  QStringList scanAudioInputDevices()
  {
    return scanAVInputDevices(CLSID_AudioInputDeviceCategory);
  }

  QStringList scanVideoInputDevices()
  {
    return scanAVInputDevices(CLSID_VideoInputDeviceCategory);
  }

  // -----------------------------------------------------------------

  /// Snapshot of the state of single AV input source
  struct SourceState
  {
    Nimble::Size resolution{0, 0};
    bool enabled = false;
    /// if the source is dead it can be removed
    bool isDead = false;
  };

  /// Single AV source
  struct Source
  {
    virtual SourceState update();

    SourceState previousState;
    QByteArray ffmpegName;
  };


  // Below is the bindings to RGBEasy-library
#ifdef RGBEASY
  struct RGBEasyAPI
  {
    // The RGBEasy library comes with a file RGBAPI.H which just have invokations
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
    virtual SourceState update() override;

    unsigned long rgbIndex;
    bool failed = false;
  };

  struct RGBEasyLib
  {
    ~RGBEasyLib();
    QLibrary rgbDll{"rgbeasy"};
    HRGBDLL apiHandle = 0;
    RGBEasyAPI api;

    RGBEasySource* createEasyRGBSource(const QString& videoInput,
                                       QStringList &audioDevices);

  } easyrgb;

  SourceState RGBEasySource::update()
  {
    SIGNALTYPE signalType = RGB_SIGNALTYPE_NOSIGNAL;
    unsigned long w = 0, h = 0, rate = 0;

    SourceState state;
    state.enabled = false;

    if (easyrgb.api.RGBGetInputSignalType(rgbIndex, &signalType, &w, &h, &rate) != 0) {
      if(!failed) {
        Radiant::warning("RGBEasyMonitor # RGBGetInputSignalType failed");
        failed = true;
      }
    } else {
      failed = false;
      state.resolution.make(w, h);
      state.enabled = signalType != RGB_SIGNALTYPE_NOSIGNAL && signalType != RGB_SIGNALTYPE_OUTOFRANGE;
    }
    return state;
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

  void initRGBEasy()
  {
    // Don't fail even if RGB wouldn't be available
    if(!easyrgb.rgbDll.load()) {
      Radiant::error("RGBEasyMonitor # %s", easyrgb.rgbDll.errorString().toUtf8().data());
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
      *p.first = easyrgb.rgbDll.resolve(p.second);
      if (!*p.first) {
        Radiant::error("RGBEasyMonitor # Failed to resolve %s: %s", p.second,
                       easyrgb.rgbDll.errorString().toUtf8().data());
        return;
      }
    }

    // RGBAPI functions will typically return an error code, 0 meaning success

    if (easyrgb.api.RGBLoad(&easyrgb.apiHandle) != 0) {
      easyrgb.apiHandle = 0;
      Radiant::error("RGBEasyMonitor # Failed to initialize RGB driver");
      return;
    }
  }

  RGBEasySource* RGBEasyLib::createEasyRGBSource(const QString &videoInput,
                                                 QStringList &audioDevices)
  {
    static unsigned long numberOfInputs = 0;
    MULTI_ONCE {
      if (easyrgb.api.RGBGetNumberOfInputs(&numberOfInputs) != 0) {
        Radiant::error("RGBEasyMonitor # Failed to get the number of inputs");
      }
    }
    if(numberOfInputs == 0)
      return nullptr;

    QRegExp numberRe("\\b0*(\\d+)\\b");
    int pos = numberRe.indexIn(videoInput);
    if(pos < 0) {
      return nullptr;
    }
    int rgbIndex = numberRe.cap(1).toInt()-1;

    RGBINPUTINFOA info;
    memset(&info, 0, sizeof(info));
    info.Size = sizeof(info);
    if (easyrgb.api.RGBGetInputInfoA(rgbIndex, &info) != 0) {
      return nullptr;
    }
    QString deviceName = info.DeviceName;
    if(!videoInput.contains(deviceName)) {
      return nullptr;
    }

    RGBEasySource* src = new RGBEasySource();
    src->rgbIndex = rgbIndex;
    src->ffmpegName = "video="+videoInput.toUtf8();

    QRegExp thisNumberRe(QString("\\b0*%1\\b").arg(rgbIndex + 1));
    for(auto it = audioDevices.begin(); it != audioDevices.end(); ++it) {
      const QString& audioInput = *it;
      if(audioInput.contains(deviceName) && thisNumberRe.indexIn(audioInput) >= 0) {
        src->ffmpegName += ":audio=" + audioInput;
        audioDevices.erase(it);
        break;
      }
    }
    return src;
  }

#endif

  SourceState Source::update()
  {
    SourceState state;
    state.enabled = true;
    return state;
  }

  // -----------------------------------------------------------------

  class VideoCaptureMonitor::D
  {
  public:
    D(VideoCaptureMonitor& host) : m_host(host) {}
    ~D();

    bool init();
    void initExternalLibs();
    void poll();

    void updateSource(Source& src, const SourceState& state);

    /// Creates source using the given videoInput
    Source* createSource(const QString& videoInput, QStringList& audioDevices);
    /// Creates source without external APIs
    Source* createGenericSource(const QString& videoInput, QStringList& audioDevices);

    /// Mapping from 'FriendlyName' to source
    std::map<QByteArray, std::unique_ptr<Source>> m_sources;

    VideoCaptureMonitor& m_host;

    bool m_initialized = false;
    double m_pollInterval = 1.0;
  };

  // -----------------------------------------------------------------

  VideoCaptureMonitor::D::~D()
  {
  }

  bool VideoCaptureMonitor::D::init()
  {
    MULTI_ONCE m_d->initExternalLibs();
    return true;
  }

  void VideoCaptureMonitor::D::poll()
  {
    QStringList videoDevices = scanVideoInputDevices();
    QStringList audioDevices = scanAudioInputDevices();

    for(const QString& videoInput : videoDevices) {
      /// Check new devices
      if(m_sources.find(videoInput.toUtf8()) != m_sources.end())
        continue; // present and handled below

      std::unique_ptr<Source> src(createSource(videoInput, audioDevices));
      if(src.get()) {
        m_sources[videoInput.toUtf8()] = std::move(src);
      }
    }

    for(auto it = m_sources.begin(); it != m_sources.end(); ) {
      std::unique_ptr<Source>& src = it->second;
      SourceState state = src->update();
      bool remove = state.isDead || !videoDevices.contains(it->first);
      if(remove)
        state.enabled = false;

      updateSource(*src.get(), state);

      if(remove) {
        it = m_sources.erase(it);
      } else {
        ++it;
      }
    }
  }

  void VideoCaptureMonitor::D::initExternalLibs()
  {
#ifdef RGBEASY
    initRGBEasy();
#endif
  }

  Source* VideoCaptureMonitor::D::createGenericSource(const QString &videoInput,
                                                      QStringList &audioDevices)
  {
    Source* src = new Source();
    src->ffmpegName = "video="+videoInput.toUtf8();
    for(auto it = audioDevices.begin(); it != audioDevices.end(); ++it) {
      /// The following logic is based on small sample size while testing different
      /// devices
      if(it->contains(videoInput)) {
        src->ffmpegName += ":audio=" + *it;
        audioDevices.erase(it);
        break;
      }
    }
    return src;
  }

  Source* VideoCaptureMonitor::D::createSource(const QString &videoInput,
                                               QStringList &audioDevices)
  {
#ifdef RGBEASY
    if(easyrgb.apiHandle != 0) {
      /// Check if this device is accessable with EasyRGB-device
      Source* res = easyrgb.createEasyRGBSource(videoInput, audioDevices);
      if(res)
        return res;
    }
#endif

    return createGenericSource(videoInput, audioDevices);
  }


  void VideoCaptureMonitor::D::updateSource(Source& src, const SourceState& state)
  {
    const SourceState& oldState = src.previousState;
    if (oldState.enabled != state.enabled) {
      if (state.enabled) {
        m_host.eventSend("source-added", src.ffmpegName, state.resolution.toVector());
      } else {
        m_host.eventSend("source-removed", src.ffmpegName);
      }
    } else if (state.enabled && oldState.resolution != state.resolution) {
      m_host.eventSend("resolution-changed", src.ffmpegName, state.resolution.toVector());
    }
    src.previousState = state;
  }


  // -----------------------------------------------------------------

  void VideoCaptureMonitor::resetSource(const QByteArray & device)
  {
    (void) device;
    assert(false && "Not implemented on this platform");
  }

}

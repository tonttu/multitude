#include "VideoCaptureMonitor.hpp"

#include <Shlwapi.h>
#include <windows.h>
#include <dshow.h>
#include <propvarutil.h>

#include <QLibrary>
#include <QRegularExpression>

#include <functional>

#include "WindowsVideoHelpers.hpp"
#include "RGBEasy.hpp"

namespace VideoDisplay
{

  /// scanAudioInputDevices & scanVideoInputDevices are essentially the same
  /// with some minor differences

  std::vector<AudioInput> scanAudioInputDevices()
  {
    ICreateDevEnum * sysDevEnum = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum, (void **)&sysDevEnum);
    std::vector<AudioInput> devices;
    if (FAILED(hr))
      return devices;

    IEnumMoniker * enumCat = nullptr;
    if (sysDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &enumCat, 0) == S_OK) {
      IMoniker * moniker = nullptr;
      while (enumCat->Next(1, &moniker, nullptr) == S_OK) {
        AudioInput source;

        IPropertyBag * propBag = nullptr;
        if (!SUCCEEDED(moniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&propBag)))
          continue;

        VARIANT varName;
        VariantInit(&varName);
        if (SUCCEEDED(propBag->Read(L"FriendlyName", &varName, 0)) && IsVariantString(varName)) {
          WCHAR name[256];
          VariantToString(varName, name, sizeof(name));
          source.friendlyName = QString::fromWCharArray(name);
        }
        VariantClear(&varName);

        if (SUCCEEDED(propBag->Read(L"WaveInID", &varName, 0))) {
          source.waveInId = varName.lVal;
        }
        VariantClear(&varName);

        propBag->Release();

        devices.emplace_back(source);
      }
      moniker->Release();
    }
    if(enumCat)
      enumCat->Release();
    sysDevEnum->Release();
    return devices;
  }

  // -----------------------------------------------------------------

  std::vector<VideoInput> scanVideoInputDevices()
  {
    ICreateDevEnum * sysDevEnum = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum, (void **)&sysDevEnum);
    std::vector<VideoInput> devices;
    if (FAILED(hr))
      return devices;

    IEnumMoniker * enumCat = nullptr;
    if (sysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enumCat, 0) == S_OK) {
      IMoniker * moniker = nullptr;
      while (enumCat->Next(1, &moniker, nullptr) == S_OK) {
        IPropertyBag * propBag = nullptr;
        if (!SUCCEEDED(moniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&propBag)))
          continue;

        VARIANT varName;
        VariantInit(&varName);
        VideoInput source;

        if (SUCCEEDED(propBag->Read(L"FriendlyName", &varName, 0)) && IsVariantString(varName)) {
          WCHAR name[256];
          VariantToString(varName, name, sizeof(name));
          source.friendlyName = QString::fromWCharArray(name);
        }
        VariantClear(&varName);

        if (SUCCEEDED(propBag->Read(L"DevicePath", &varName, 0)) && IsVariantString(varName)) {
          WCHAR path[256];
          VariantToString(varName, path, sizeof(path));
          source.devicePath = QString::fromWCharArray(path);
        }
        VariantClear(&varName);

        /// Mangle device path for ffmpeg form
        if(!source.devicePath.isEmpty() && source.devicePath.contains("usb#vid"))
          source.devicePath = QString("%1%2").arg("@device_pnp_", source.devicePath);

        devices.emplace_back(source);

        propBag->Release();
      }
      moniker->Release();
    }
    if(enumCat)
      enumCat->Release();
    sysDevEnum->Release();
    return devices;
  }

  // -----------------------------------------------------------------

  template <typename T>
  struct CompareUnderlyingObjects
  {
    bool operator()(const std::unique_ptr<T>& p1, const std::unique_ptr<T>& p2) const
    {
      return *p1.get() < *p2.get();
    }
  };

  // -----------------------------------------------------------------

  class VideoCaptureMonitor::D
  {
  public:
    typedef std::vector<std::vector<float>> Scores;

    D(VideoCaptureMonitor& host) : m_host(host) {}
    ~D();

    void initExternalLibs();
    void poll();

    Source parseSource(const QString& device) const;
    void updateSource(SourcePtr& src);

    /// Check if we have source formed of these inputs
    bool contains(const VideoInput& vi, const AudioInput& ai) const;
    bool isHinted(const VideoInput& vi, const AudioInput& ai) const;

    Scores scores(const std::vector<VideoInput>& videos,
                  const std::vector<AudioInput>& audios) const;

    std::vector<float>
    scores(const VideoInput& video, const std::vector<AudioInput>& audios) const;

    /// This assumes that there is no additional information about the video or audio
    /// @see scores-function that calculates scores for single row
    float score(const VideoInput& audio, const AudioInput& video) const;

    std::map<int, int> formPairsGreedily(const Scores& scores);

    void initInput(AudioInput& ai) const;
    void initInput(VideoInput& vi) const;

    SourcePtr createSource(const VideoInput &videoInput, const AudioInput& audioInput);
    SourcePtr createSource(const VideoInput &videoInput);

    void removeSource(const SourcePtr& s);
    void addSource(const SourcePtr& s, Nimble::Size resolution);
    void resolutionChanged(const SourcePtr& s, Nimble::Size resolution);

    void updateSources(std::vector<SourcePtr>& currentSources);


    /// Current sources
    Radiant::Mutex m_sourcesMutex;
    std::vector<SourcePtr> m_sources;

    VideoCaptureMonitor& m_host;

    Radiant::Mutex m_removedSourcesMutex;
    std::vector<Source> m_removedSources;

    // Contains sources that are suggested to be pairs by the application
    mutable Radiant::Mutex m_hintMutex;
    std::set<Source> m_hintedSources;

    double m_pollInterval = 1.0;
  };

  // -----------------------------------------------------------------

  VideoCaptureMonitor::D::~D()
  {
  }

  bool VideoCaptureMonitor::D::contains(const VideoInput &vi, const AudioInput &ai) const
  {
    for(const SourcePtr& s : m_sources) {
      if(s->video == vi && s->audio == ai) {
        return true;
      }
    }
    return false;
  }

  bool VideoCaptureMonitor::D::isHinted(const VideoInput &vi, const AudioInput &ai) const
  {
    Source s(vi, ai);
    Radiant::Guard g(m_hintMutex);
    for(const Source& hint : m_hintedSources) {
      if(s.ffmpegName() == hint.ffmpegName())
        return true;
    }
    return false;
  }

  float VideoCaptureMonitor::D::score(const VideoInput& video, const AudioInput& audio) const
  {
    /// Pretty random heuristics follow. By investigating usb paths we could (???)
    /// achieve more precise guesses

    float score = 0;
    if(audio.friendlyName.contains(video.friendlyName))
      score += 10;
    else if(video.friendlyName == "QCMEVB" && audio.friendlyName.contains("Surface Hub"))
      score += 10;

    /// If this pair already existed we may be adding USB cameras sequentially
    /// into the system. In that case give some extra scores if the pair
    /// already existed
    if(contains(video, audio))
      score += 2;

    /// Hint trumps the existing detection
    if(isHinted(video, audio))
      score += 3;

    return score;
  }

  std::vector<float>
  VideoCaptureMonitor::D::scores(const VideoInput& video,
                                 const std::vector<AudioInput>& audios) const
  {
    using namespace std::placeholders; /// for _1 & _2

    std::function<float(const VideoInput&, const AudioInput&)> scoreFunc =
        std::bind(&VideoCaptureMonitor::D::score, this, _1, _2);

    if(video.rgbIndex >= 0) {
      scoreFunc = std::bind(&RGBEasyLib::score, &easyrgb, _1, _2);
    }

    std::vector<float> row;
    for(size_t i = 0; i < audios.size(); ++i) {
      row.emplace_back(scoreFunc(video, audios[i]));
    }
    return row;
  }

  VideoCaptureMonitor::D::Scores
  VideoCaptureMonitor::D::scores(const std::vector<VideoInput>& videos,
                                 const std::vector<AudioInput>& audios) const
  {
    std::vector<std::vector<float>> scoreMatrix;
    for(size_t i = 0; i < videos.size(); ++i) {
      scoreMatrix.emplace_back(scores(videos[i], audios));
    }
    return scoreMatrix;
  }

  void VideoCaptureMonitor::D::initInput(AudioInput& ai) const
  {
    (void) ai; // no special procedures... yet
  }

  void VideoCaptureMonitor::D::initInput(VideoInput& vi) const
  {
    easyrgb.initInput(vi);
  }

  std::map<int, int>
  VideoCaptureMonitor::D::formPairsGreedily(const Scores& scores)
  {
    std::vector<std::tuple<float, int, int>> tmp;
    for(size_t video = 0; video < scores.size(); ++video) {
      for(size_t audio = 0; audio < scores[video].size(); ++audio) {
        float score = -scores[video][audio]; // lazy way to set up comparisons..
        tmp.push_back(std::make_tuple(score, video, audio));
      }
    }

    std::sort(tmp.begin(), tmp.end());

    std::set<int> usedAudios;

    std::map<int, int> results;
    int added = 0, toAdd = int(scores.size());
    for(const std::tuple<float, int, int>& t : tmp) {
      float score;
      int video, audio;
      std::tie(score, video, audio) = t;

      if(results.find(video) != results.end())
        continue;
      if(usedAudios.find(audio) != usedAudios.end())
        continue;

      results[video] = score < 0 ? audio : -1; // note inverted scores
      usedAudios.insert(audio);
      if(++added == toAdd)
        break;
    }
    return results;
  }


  void VideoCaptureMonitor::D::poll()
  {
    MULTI_ONCE initExternalLibs();

    Radiant::Guard g(m_sourcesMutex);

    {
      Radiant::Guard g(m_removedSourcesMutex);
      for(const Source& s : m_removedSources) {

        for(auto it = m_sources.begin(); it != m_sources.end(); ++it) {
          if(s == *it->get()) {
            m_sources.erase(it);
            break;
          }
        }

      }
      m_removedSources.clear();
    }


    /// Polling logic is following:
    /// 1) Query all audio and video capture devices
    /// 2) Calculate heuristic score for each (video,audio) pairs
    /// 3) Based on the heuristical scores, pick the pairs greedily
    ///    and consider these as available sources

    std::vector<VideoInput> videoDevices = scanVideoInputDevices();
    std::vector<AudioInput> audioDevices = scanAudioInputDevices();

    for(VideoInput& vi : videoDevices) initInput(vi);
    for(AudioInput& ai : audioDevices) initInput(ai);

    /// Calculate scores for each (Video, Audio) -pair
    /// This will create N vectors of length M, where
    ///   N = number of video inputs
    ///   M = number of audio inputs

    Scores scoreSheet = scores(videoDevices, audioDevices);

    // remove hints, there is a slight change that hint is added but
    // never used. That is unlikely to occur
    {
      Radiant::Guard g(m_hintMutex);
      m_hintedSources.clear();
    }

    /// Create N Sources (video, audio)-pairs with or without audio device.
    /// Use each scanned device at most once and only form pairs that
    /// have positive scores.

    std::map<int, int> pairs = formPairsGreedily(scoreSheet);

    std::vector<SourcePtr> currentSources;
    for(const std::pair<int, int>& p : pairs) {
      int video = p.first,
          audio = p.second;

      if(audio >= 0)
        currentSources.emplace_back(createSource(videoDevices[video], audioDevices[audio]));
      else
        currentSources.emplace_back(createSource(videoDevices[video]));
    }

    updateSources(currentSources);
  }

  void VideoCaptureMonitor::D::updateSources(std::vector<SourcePtr> &currentSources)
  {
    std::sort(currentSources.begin(), currentSources.end(),
              CompareUnderlyingObjects<Source>());

    /// Partition the union of current and old video sources to the
    /// following sets
    std::vector<SourcePtr> sourcesToRemove;
    std::vector<SourcePtr> updatedSources;

    auto currentIt  = currentSources.begin();
    auto currentEnd = currentSources.end();
    auto oldIt      = m_sources.begin();
    auto oldEnd     = m_sources.end();

    for( ; currentIt != currentEnd && oldIt != oldEnd; ) {
      SourcePtr& current = *currentIt;
      SourcePtr& old     = *oldIt;
      if(*current < *old) {
        updatedSources.emplace_back(std::move(current));
        ++currentIt;
      } else if(*old < *current) {
        sourcesToRemove.emplace_back(std::move(old));
        ++oldIt;
      } else {
        assert(*current == *old);
        /// Note that we need to use Source from existing vector, as it has
        /// some additional state stored into it
        updatedSources.emplace_back(std::move(old));
        ++oldIt;
        ++currentIt;
      }
    }

    while(currentIt != currentEnd) {
      updatedSources.emplace_back(std::move(*currentIt));
      ++currentIt;
    }
    while(oldIt != oldEnd) {
      sourcesToRemove.emplace_back(std::move(*oldIt));
      ++oldIt;
    }

    m_sources = std::move(updatedSources);

    /// We can immediately remove all sources that are not present
    for(const SourcePtr& s : sourcesToRemove) {
      removeSource(s);
    }

    for(SourcePtr& s : m_sources) {
      updateSource(s);
    }
  }

  void VideoCaptureMonitor::D::initExternalLibs()
  {
    easyrgb.loadDll();
  }

  SourcePtr VideoCaptureMonitor::D::createSource(const VideoInput& videoInput,
                                                 const AudioInput& audioInput)
  {
    if(videoInput.rgbIndex >= 0) {
      /// Check if this device is accessable with EasyRGB-device
      return easyrgb.createEasyRGBSource(videoInput, audioInput);
    }

    std::unique_ptr<Source> src;
    src.reset(new Source(videoInput, audioInput));
    return src;
  }

  SourcePtr VideoCaptureMonitor::D::createSource(const VideoInput& videoInput)
  {
    std::unique_ptr<Source> src;
    src.reset(new Source());
    src->video = videoInput;
    return src;
  }

  void VideoCaptureMonitor::D::addSource(const SourcePtr &s, Nimble::Size resolution)
  {
    m_host.eventSend("source-added", s->ffmpegName(), resolution.toVector());
  }

  void VideoCaptureMonitor::D::removeSource(const SourcePtr &s)
  {
    m_host.eventSend("source-removed", s->ffmpegName());
  }

  void VideoCaptureMonitor::D::resolutionChanged(const SourcePtr &s, Nimble::Size resolution)
  {
    m_host.eventSend("resolution-changed", s->ffmpegName(), resolution.toVector());
  }

  void VideoCaptureMonitor::D::updateSource(SourcePtr& src)
  {
    const SourceState& oldState = src->previousState;
    SourceState state = src->update();
    if (oldState.enabled != state.enabled) {
      if (state.enabled) {
        addSource(src, state.resolution);
      } else {
        removeSource(src);
      }
    } else if (state.enabled && oldState.resolution != state.resolution) {
      resolutionChanged(src, state.resolution);
    }
    src->previousState = state;
  }

  Source VideoCaptureMonitor::D::parseSource(const QString& device) const
  {
    static QRegularExpression re("^video=([^:]*)(:audio=(.*))?$",
                                 QRegularExpression::OptimizeOnFirstUsageOption);

    Source s;

    QRegularExpressionMatch m = re.match(device);
    if(m.hasMatch() && m.lastCapturedIndex() >= 3) {
      VideoInput vi;
      vi.friendlyName = m.captured(1);
      s.video = vi;

      AudioInput ai;
      ai.friendlyName = m.captured(3);
      s.audio = ai;
    }

    return s;
  }

  // -----------------------------------------------------------------

  VideoCaptureMonitor::VideoCaptureMonitor()
    : m_d(new D(*this))
  {
    eventAddOut("source-added");
    eventAddOut("source-removed");
    eventAddOut("resolution-changed");
  }

  VideoCaptureMonitor::~VideoCaptureMonitor()
  {
  }

  double VideoCaptureMonitor::pollInterval() const
  {
    return m_d->m_pollInterval;
  }

  void VideoCaptureMonitor::setPollInterval(double seconds)
  {
    m_d->m_pollInterval = seconds;
    if (secondsUntilScheduled() > 0) {
      scheduleFromNowSecs(m_d->m_pollInterval);
    }
  }

  void VideoCaptureMonitor::addHint(const QString& device)
  {
    Source s = m_d->parseSource(device);
    if(s.isValid()) {
      Radiant::Guard g(m_d->m_hintMutex);
      m_d->m_hintedSources.insert(s);
    }
  }

  void VideoCaptureMonitor::removeSource(const QString &source)
  {
    Source src = m_d->parseSource(source);
    if(src.isValid()) {
      Radiant::Guard g(m_d->m_removedSourcesMutex);
      m_d->m_removedSources.emplace_back(src);
    }
  }

  QList<VideoCaptureMonitor::VideoSource> VideoCaptureMonitor::sources() const
  {
    QList<VideoSource> ret;
    Radiant::Guard g(m_d->m_sourcesMutex);
    for (auto & ptr: m_d->m_sources) {
      Source & s = *ptr;
      if (s.previousState.enabled) {
        VideoSource vs;
        vs.device = s.ffmpegName().toUtf8();
        vs.resolution = s.previousState.resolution.toVector();
        ret << vs;
      }
    }
    return ret;
  }

  void VideoCaptureMonitor::doTask()
  {
    m_d->poll();
    scheduleFromNowSecs(m_d->m_pollInterval);
  }

  DEFINE_SINGLETON(VideoCaptureMonitor)
}

#include "MWCapture.hpp"

#include <Radiant/Thread.hpp>

#include <LibMWCapture/MWCapture.h>

namespace
{
  /// It's unclear if MW library is thread-safe, so we assume it isn't
  Radiant::Mutex s_mwMutex;
}

namespace VideoDisplay
{
  class MWCaptureSource : public Source
  {
  public:
    MWCaptureSource(std::weak_ptr<MWCapture> mwCapture, const VideoInput & videoInput,
                    const AudioInput & audioInput);

    virtual SourceState update() override;

  private:
    std::weak_ptr<MWCapture> m_mwCapture;
  };

  MWCaptureSource::MWCaptureSource(std::weak_ptr<MWCapture> mwCapture,
                                   const VideoInput & videoInput,
                                   const AudioInput & audioInput)
    : Source(videoInput, audioInput)
    , m_mwCapture(mwCapture)
  {
  }

  SourceState MWCaptureSource::update()
  {
    SourceState state;
    state.enabled = false;

    auto capture = m_mwCapture.lock();
    if (!capture)
      return state;

    Radiant::Guard g(s_mwMutex);
    HCHANNEL ch = MWOpenChannelByPath(video.magewellDevicePath.toStdWString().c_str());
    if (ch) {
      MWCAP_VIDEO_SIGNAL_STATUS status {};
      if (MWGetVideoSignalStatus(ch, &status) == MW_SUCCEEDED) {
        if (status.state == MWCAP_VIDEO_SIGNAL_LOCKED) {
          state.resolution.make(status.cx, status.cy);
          state.enabled = true;
        }
      }
      MWCloseChannel(ch);
    }
    return state;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  MWCapture::MWCapture()
  {
    Radiant::Guard g(s_mwMutex);
    MWCaptureInitInstance();
    MULTI_ONCE {
      int count = MWGetChannelCount();
      std::map<int, MWCAP_CHANNEL_INFO> boards;
      std::map<int, std::vector<int>> channels;

      for (int i = 0; i < count; ++i) {
        WCHAR path[512];
        path[0] = L'\0';
        if (MWGetDevicePath(i, path) != MW_SUCCEEDED)
          continue;

        HCHANNEL ch = MWOpenChannelByPath(path);
        if (!ch)
          continue;

        MWCAP_CHANNEL_INFO videoInfo {};
        if (MWGetChannelInfo(ch, &videoInfo) == MW_SUCCEEDED) {
          boards[videoInfo.byBoardIndex] = videoInfo;
          channels[videoInfo.byBoardIndex].push_back(videoInfo.byChannelIndex);
        }

        MWCloseChannel(ch);
      }

      for (auto & p: boards) {
        int channelCount = (int)channels[p.first].size();
        MWCAP_CHANNEL_INFO & i = p.second;
        Radiant::info("Detected Magewell %s with %s and %d channel%s, idx %d, sn %s, hw %d, fw %d, driver %d",
                      QByteArray(i.szProductName).trimmed().data(),
                      QByteArray(i.szFirmwareName).trimmed().data(),
                      channelCount, channelCount == 1 ? "": "s",
                      (int)i.byBoardIndex,
                      QByteArray(i.szBoardSerialNo).trimmed().data(),
                      (int)i.chHardwareVersion,
                      (int)i.dwFirmwareVersion,
                      (int)i.dwDriverVersion);
      }
    }
  }

  MWCapture::~MWCapture()
  {
    Radiant::Guard g(s_mwMutex);
    MWCaptureExitInstance();
  }

  void MWCapture::initInput(VideoInput & vi)
  {
    QString path = vi.devicePath;
    path.replace("@device:pnp:", "");

    Radiant::Guard g(s_mwMutex);
    HCHANNEL ch = MWOpenChannelByPath(path.toStdWString().c_str());
    if (ch) {
      MWCAP_CHANNEL_INFO videoInfo {};
      if (MWGetChannelInfo(ch, &videoInfo) == MW_SUCCEEDED)
        vi.magewellDevicePath = path;
      MWCloseChannel(ch);
    }
  }

  std::unique_ptr<Source> MWCapture::createSource(const VideoInput & videoInput, const AudioInput & audioInput)
  {
    return std::make_unique<MWCaptureSource>(weakInstance(), videoInput, audioInput);
  }

  DEFINE_SINGLETON(MWCapture)

} // namespace VideoDisplay

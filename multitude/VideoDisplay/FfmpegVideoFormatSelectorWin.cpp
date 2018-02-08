#include "FfmpegVideoFormatSelector.hpp"
#include "FfmpegDecoder.hpp"

extern "C" {
#include <libavdevice/avdevice.h>
}

namespace VideoDisplay
{
  static void setMapOptions(const QMap<QString, QString> & input, AVDictionary ** output)
  {
    for (auto it = input.begin(); it != input.end(); ++it)
      av_dict_set(output, it.key().toUtf8().data(), it.value().toUtf8().data(), 0);
  }

  /// dshow output can be something like this:
  /// vcodec=mjpeg  min s=640x480 fps=5 max s=640x480 fps=120
  /// In this case we will emit the following formats:
  /// mjpeg 640x480 @5
  /// mjpeg 640x480 @30
  /// mjpeg 640x480 @60
  /// mjpeg 640x480 @120
  /// If min and max resolutions are different, we just output both min and max sizes.
  static void emitFormats(std::vector<VideoInputFormat> & out, const VideoInputFormat & formatTpl,
                          Nimble::SizeI minSize, Nimble::SizeI maxSize, float minFps, float maxFps)
  {
    VideoInputFormat format = formatTpl;

    if (!format.vcodec.isEmpty()) {
      format.category = VideoInputFormat::FORMAT_COMPRESSED;
    } else if (format.pixelFormat.contains("rgb") || format.pixelFormat.contains("bgr")) {
      format.category = VideoInputFormat::FORMAT_RGB;
    } else if (format.pixelFormat.contains("yuv") || format.pixelFormat.contains("yuyv")) {
      format.category = VideoInputFormat::FORMAT_YUV;
    } else {
      /// For instance NV12 or NV21 are not natively supported atm and require an expensive conversion
      format.category = VideoInputFormat::FORMAT_UNKNOWN;
    }

    // It seems that when minSize is different from maxSize, this is a capture device
    // that chooses the native resolution based on the signal that it receives. We have
    // no way of knowing what is the correct resolution and aspect ratio to choose here,
    // so just let the card do its thing.
    if (minSize == maxSize)
      format.resolution = minSize;

    format.fps = minFps;
    out.push_back(format);

    for (int targetFps = 30; targetFps <= 120; targetFps *= 2) {
      if (format.fps < targetFps-10.f && maxFps >= targetFps+10.f) {
        format.fps = targetFps;
        out.push_back(format);
      }
    }

    if (format.fps < maxFps) {
      format.fps = maxFps;
      out.push_back(format);
    }
  }

  /// Ffmpeg doesn't have an API for fetching this information, instead it has
  /// an option "list_options" that can be set to true, which triggers the dshow
  /// component to print format information using ffmpeg logging functions.
  ///
  /// We are forced to temporarily capture ffmpeg output from this thread, open
  /// the stream using "list_options" flag and parse the output. Then based on
  /// that parsed output we process and generate the formats in emitFormats
  /// which gives as full list of available formats.
  ///
  /// Capture cards typically output dynamic resolutions that are rejected in
  /// emitFormats, which means that we let the driver / card to choose the
  /// resolution based on the incoming native resolution.
  std::vector<VideoInputFormat> scanInputFormats(
      const QString & input, AVInputFormat * inputFormat, QMap<QString, QString> options)
  {
    std::vector<VideoInputFormat> ret;

    QString videoTarget;
    for (auto str: input.split(":"))
      if (str.startsWith("video="))
        videoTarget = str;

    if (videoTarget.isEmpty())
      return ret;

    options["list_options"] = "true";

    AVDictionary * tmpAvoptions = nullptr;
    setMapOptions(options, &tmpAvoptions);

    AVFormatContext * formatContext = avformat_alloc_context();

    QRegExp pinR("Pin \"(.+)\"\\s.*");
    QRegExp pixFormatR("pixel_format=(.+)");
    QRegExp vcodecR("vcodec=(.+)");
    QRegExp errR(".*unknown compression type.*");
    QRegExp resR("min s=(\\d+)x(\\d+)\\s+fps=([\\d.]+)\\s+"
                 "max s=(\\d+)x(\\d+)\\s+fps=([\\d.]+)");
    QRegExp ignoreR(".*DirectShow video device options.*");

    VideoInputFormat format;

    std::function<bool(int, const char*)> logHandler = [&pinR, &pixFormatR, &resR, &vcodecR, &errR, &format, &ret, &ignoreR]
        (int level, const char * line) -> bool {
      if (level != AV_LOG_INFO)
        return false;

      while (*line == ' ')
        ++line;
      QString msg = line;

      if (pinR.exactMatch(msg)) {
        format = VideoInputFormat();
        format.pin = pinR.cap(1);
      } else if (pixFormatR.exactMatch(msg)) {
        format.pixelFormat = pixFormatR.cap(1);
        format.vcodec.clear();
      } else if (vcodecR.exactMatch(msg)) {
        format.vcodec = vcodecR.cap(1);
        format.pixelFormat.clear();
      } else if (errR.exactMatch(msg)) {
        format.pixelFormat.clear();
        format.vcodec.clear();
      } else if (resR.exactMatch(msg)) {
        Nimble::SizeI minSize(resR.cap(1).toInt(), resR.cap(2).toInt());
        Nimble::SizeI maxSize(resR.cap(4).toInt(), resR.cap(5).toInt());
        float minFps = resR.cap(3).toFloat();
        float maxFps = resR.cap(6).toFloat();
        if (!format.pin.isEmpty() && (format.vcodec.isEmpty() ^ format.pixelFormat.isEmpty())) {
          emitFormats(ret, format, minSize, maxSize, minFps, maxFps);
        }
      } else if (!ignoreR.exactMatch(msg)) {
        return false;
      }
      return true;
    };

    FfmpegDecoder::setTlsLogHandler(&logHandler);
    avformat_open_input(&formatContext, videoTarget.toUtf8().data(),
                        inputFormat, &tmpAvoptions);
    FfmpegDecoder::setTlsLogHandler(nullptr);

    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    av_dict_free(&tmpAvoptions);

    return ret;
  }

  const VideoInputFormat * chooseFormat(const std::vector<VideoInputFormat> & formats,
                                        const AVDecoder::Options & avOptions)
  {
    const QMap<QString, QString> & options = avOptions.demuxerOptions();

    QString pin = options.value("video_pin_name");
    QString pixelFormat = options.value("pixel_format");

    VideoInputFormat::FormatCategory category = VideoInputFormat::FORMAT_UNKNOWN;
    if (avOptions.pixelFormat() == VideoFrame::RGB || avOptions.pixelFormat() == VideoFrame::RGBA)
      category = VideoInputFormat::FORMAT_RGB;
    if (avOptions.pixelFormat() == VideoFrame::YUV || avOptions.pixelFormat() == VideoFrame::YUVA)
      category = VideoInputFormat::FORMAT_YUV;

    Nimble::SizeI resolution;
    if (options.contains("video_size")) {
      QRegExp sizeR("(\\d)+x(\\d+)");
      if (sizeR.exactMatch(options.value("video_size")))
        resolution.make(sizeR.cap(1).toInt(), sizeR.cap(2).toInt());
    }

    float fps = 0;
    if (options.contains("framerate"))
      fps = options.value("framerate").toFloat();

    const AVDecoder::VideoStreamHints & hints = avOptions.videoStreamHints();

    const VideoInputFormat * bestFormat = nullptr;
    uint64_t bestScore = 0;

    for (const VideoInputFormat & format: formats) {

      /// If user has specified any exact parameter values, filter the list based on those

      if (!pin.isNull() && format.pin != pin)
        continue;

      if (!pixelFormat.isNull() && format.pixelFormat != pixelFormat)
        continue;

      if (category != VideoInputFormat::FORMAT_UNKNOWN &&
          format.category != category)
        continue;

      if (resolution.isValid() && resolution != format.resolution)
        continue;

      if (fps > 0 && std::abs(fps - format.fps) > 0.001f)
        continue;

      /// Format is acceptable, now form a "score" for each format and choose
      /// the best one
      uint64_t score = 0;

      /// Use expensive or unknown video formats as a last choice
      if (format.category != VideoInputFormat::FORMAT_UNKNOWN)
        score += 10000000000000000000ull;

      if (format.fps >= hints.minFps && format.fps <= hints.maxFps)
        score += 1000000000000000000ull;

      if (format.resolution.width() >= hints.minResolution.width() &&
          format.resolution.height() >= hints.minResolution.height() &&
          format.resolution.width() <= hints.maxResolution.width() &&
          format.resolution.height() <= hints.maxResolution.height())
        score += 100000000000000000ull;

      /// If we prefer quality over resolution, we don't want to have compressed stream
      if (hints.preferUncompressedStream &&
          format.category != VideoInputFormat::FORMAT_COMPRESSED)
        score += 10000000000000000ull;

      /// If these lower digits of score are used at all, it means that we have
      /// multiple formats that all match all the options given in
      /// AVDecoder::Options, and also match the VideoStreamHints equally well.
      /// From these formats, use the best resolution and biggest fps.

      score += format.resolution.width() * format.resolution.height() * 10000ull;
      score += uint64_t(format.fps * 10);

      /// YUV is the best compared to RGB / compressed
      if (format.category == VideoInputFormat::FORMAT_YUV)
        score += 1ull;

      if (score > bestScore || !bestFormat) {
        bestFormat = &format;
        bestScore = score;
      }
    }

    return bestFormat;
  }

  void applyFormatOptions(const VideoInputFormat & format,
                          AVDecoder::Options & avOptions)
  {
    if (!format.pin.isEmpty())
      avOptions.setDemuxerOption("video_pin_name", format.pin);
    if (!format.pixelFormat.isEmpty())
      avOptions.setDemuxerOption("pixel_format", format.pixelFormat);
    if (format.resolution.isValid())
      avOptions.setDemuxerOption("video_size", QString("%1x%2").
                                 arg(format.resolution.width()).
                                 arg(format.resolution.height()));
    if (format.fps > 0)
      avOptions.setDemuxerOption("framerate", QString::number(format.fps));

    /// @todo vcodec
  }
}

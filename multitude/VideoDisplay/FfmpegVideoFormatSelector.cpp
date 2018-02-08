#include "FfmpegVideoFormatSelector.hpp"

namespace VideoDisplay
{
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

#ifdef RADIANT_LINUX
    if (!format.vcodec.isEmpty())
      avOptions.setDemuxerOption("input_format", format.vcodec);
#else
    /// @todo vcodec
#endif
  }
}

#include "FfmpegVideoFormatSelector.hpp"

namespace VideoDisplay
{
  static Radiant::Mutex s_warnMutex;
  static std::set<QString> s_warned;

  static bool isValidFps(float fps, const AVDecoder::VideoStreamHints & hints)
  {
    return fps >= hints.minFps && fps <= hints.maxFps;
  }

  static bool isValidResolution(Nimble::SizeI res, const AVDecoder::VideoStreamHints & hints)
  {
    return res.width() >= hints.minResolution.width() &&
        res.height() >= hints.minResolution.height() &&
        res.width() <= hints.maxResolution.width() &&
        res.height() <= hints.maxResolution.height();
  }

  // Warn exactly once per source
  static bool shouldWarn(const QString & src)
  {
    Radiant::Guard g(s_warnMutex);
    return s_warned.insert(src).second;
  }

  static void printWarning(const QString & src, const std::vector<VideoInputFormat> & formats)
  {
    QMap<QString, QStringList> grouped;
    for (const VideoInputFormat & format: formats) {
      QStringList & modes = grouped[format.vcodec + format.pixelFormat];
      QString res = QString("%1x%2@").arg(format.resolution.width()).arg(format.resolution.height());

      // The convention is to write framerate 60.0 as "60" and 24/1.001 as "23.976"
      if (std::abs(format.fps - std::round(format.fps)) < 0.001)
        res += QString::number(int(std::round(format.fps)));
      else
        res += QString::number(format.fps, 'f', 3);

      if (!modes.contains(res))
        modes << res;
    }

    Radiant::warning("Failed to find optimal video input format for video input %s, available formats:",
                     src.toUtf8().data());
    for (auto it = grouped.begin(); it != grouped.end(); ++it)
      Radiant::warning("  %s: %s", it.key().toUtf8().data(), it.value().join(", ").toUtf8().data());
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

      /// Format is acceptable, now use the hints to choose the best format

      /// Use expensive or unknown video formats as a last choice
      if (bestFormat && bestFormat->category != VideoInputFormat::FORMAT_UNKNOWN &&
          format.category == VideoInputFormat::FORMAT_UNKNOWN)
        continue;

      if (bestFormat && isValidFps(bestFormat->fps, hints) &&
          !isValidFps(format.fps, hints))
        continue;

      if (bestFormat && isValidResolution(bestFormat->resolution, hints) &&
          !isValidResolution(format.resolution, hints))
        continue;

      /// If we prefer quality over resolution, we don't want to have compressed stream
      if (hints.preferUncompressedStream && bestFormat &&
          bestFormat->category != VideoInputFormat::FORMAT_COMPRESSED &&
          format.category == VideoInputFormat::FORMAT_COMPRESSED)
        continue;

      /// Use the best resolution and biggest fps.

      if (bestFormat && bestFormat->resolution.width() * bestFormat->resolution.height() >
          format.resolution.width() * format.resolution.height())
        continue;

      if (bestFormat && bestFormat->fps > format.fps)
        continue;

      /// YUV is the best compared to RGB / compressed
      if (bestFormat && bestFormat->category == VideoInputFormat::FORMAT_YUV &&
          format.category != VideoInputFormat::FORMAT_YUV)
        continue;

      bestFormat = &format;
    }

    if (bestFormat) {
      bool perfectFormat = isValidFps(bestFormat->fps, hints) &&
          isValidResolution(bestFormat->resolution, hints) &&
          (!hints.preferUncompressedStream ||
           bestFormat->category != VideoInputFormat::FORMAT_COMPRESSED);
      if (!perfectFormat && shouldWarn(avOptions.source()))
        printWarning(avOptions.source(), formats);
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
    if (format.fps > 0) {
      if (avOptions.format() == "dshow") {
        /// dshow internally uses frame interval and not framerate values, see
        /// MinFrameInterval and MaxFrameInterval:
        /// https://msdn.microsoft.com/en-us/library/windows/desktop/dd407352(v=vs.85).aspx
        ///
        /// Magewell USB Capture dongles have frame interval 166667 units (59.99988 fps),
        /// and if we give "59.9999" here like what ffmpeg reports, that will be rounded to
        /// wrong direction in ffmpeg and the video opening fails. Instead we give the
        /// framerate as a fraction to work around these issues.
        /// There are some limitations on how large numbers you can have in fractions
        /// in ffmpeg, so we have a patch in place that increases that value to 1e7 so that
        /// we can give accurate values here.
        uint64_t frameInt = std::round(1e7 / format.fps);
        avOptions.setDemuxerOption("framerate", QString("10000000:%1").arg(frameInt));
      } else {
        avOptions.setDemuxerOption("framerate", QString::number(format.fps));
      }
    }

#ifdef RADIANT_LINUX
    if (!format.vcodec.isEmpty())
      avOptions.setDemuxerOption("input_format", format.vcodec);
#else
    /// @todo vcodec
#endif
  }
}

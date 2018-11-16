#include "FfmpegVideoFormatSelector.hpp"
#include "FfmpegDecoder.hpp"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavutil/pixdesc.h>
}

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/videodev2.h>

// This table is mostly from libavdevice v4l2-common.c
struct FmtMap {
  AVPixelFormat ffFmt;
  AVCodecID codecId;
  uint32_t v4l2Fmt;
  VideoDisplay::VideoInputFormat::FormatCategory category;
};

const FmtMap s_fmtConversionTable[] = {
  { AV_PIX_FMT_YUV420P, AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_YUV420,     VideoDisplay::VideoInputFormat::FORMAT_YUV },
  { AV_PIX_FMT_YUV420P, AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_YVU420,     VideoDisplay::VideoInputFormat::FORMAT_YUV },
  { AV_PIX_FMT_YUV422P, AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_YUV422P,    VideoDisplay::VideoInputFormat::FORMAT_YUV },
  { AV_PIX_FMT_YUYV422, AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_YUYV,       VideoDisplay::VideoInputFormat::FORMAT_YUV },
  { AV_PIX_FMT_UYVY422, AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_UYVY,       VideoDisplay::VideoInputFormat::FORMAT_YUV },
  { AV_PIX_FMT_YUV411P, AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_YUV411P,    VideoDisplay::VideoInputFormat::FORMAT_YUV },
  { AV_PIX_FMT_YUV410P, AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_YUV410,     VideoDisplay::VideoInputFormat::FORMAT_YUV },
  { AV_PIX_FMT_YUV410P, AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_YVU410,     VideoDisplay::VideoInputFormat::FORMAT_YUV },
  { AV_PIX_FMT_RGB555LE,AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_RGB555,     VideoDisplay::VideoInputFormat::FORMAT_RGB },
  { AV_PIX_FMT_RGB555BE,AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_RGB555X,    VideoDisplay::VideoInputFormat::FORMAT_RGB },
  { AV_PIX_FMT_RGB565LE,AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_RGB565,     VideoDisplay::VideoInputFormat::FORMAT_RGB },
  { AV_PIX_FMT_RGB565BE,AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_RGB565X,    VideoDisplay::VideoInputFormat::FORMAT_RGB },
  { AV_PIX_FMT_BGR24,   AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_BGR24,      VideoDisplay::VideoInputFormat::FORMAT_RGB },
  { AV_PIX_FMT_RGB24,   AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_RGB24,      VideoDisplay::VideoInputFormat::FORMAT_RGB },
  { AV_PIX_FMT_BGR0,    AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_BGR32,      VideoDisplay::VideoInputFormat::FORMAT_RGB },
  { AV_PIX_FMT_0RGB,    AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_RGB32,      VideoDisplay::VideoInputFormat::FORMAT_RGB },
  { AV_PIX_FMT_GRAY8,   AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_GREY,       VideoDisplay::VideoInputFormat::FORMAT_UNKNOWN },
  { AV_PIX_FMT_GRAY16LE,AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_Y16,        VideoDisplay::VideoInputFormat::FORMAT_UNKNOWN },
  { AV_PIX_FMT_NV12,    AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_NV12,       VideoDisplay::VideoInputFormat::FORMAT_UNKNOWN },
  { AV_PIX_FMT_NONE,    AV_CODEC_ID_MJPEG,    V4L2_PIX_FMT_MJPEG,      VideoDisplay::VideoInputFormat::FORMAT_COMPRESSED },
  { AV_PIX_FMT_NONE,    AV_CODEC_ID_MJPEG,    V4L2_PIX_FMT_JPEG,       VideoDisplay::VideoInputFormat::FORMAT_COMPRESSED },
  { AV_PIX_FMT_NONE,    AV_CODEC_ID_H264,     V4L2_PIX_FMT_H264,       VideoDisplay::VideoInputFormat::FORMAT_COMPRESSED },
  { AV_PIX_FMT_NONE,    AV_CODEC_ID_MPEG4,    V4L2_PIX_FMT_MPEG4,      VideoDisplay::VideoInputFormat::FORMAT_COMPRESSED },
  { AV_PIX_FMT_NONE,    AV_CODEC_ID_CPIA,     V4L2_PIX_FMT_CPIA1,      VideoDisplay::VideoInputFormat::FORMAT_COMPRESSED },
  { AV_PIX_FMT_BAYER_BGGR8, AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_SBGGR8, VideoDisplay::VideoInputFormat::FORMAT_UNKNOWN },
  { AV_PIX_FMT_BAYER_GBRG8, AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_SGBRG8, VideoDisplay::VideoInputFormat::FORMAT_UNKNOWN },
  { AV_PIX_FMT_BAYER_GRBG8, AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_SGRBG8, VideoDisplay::VideoInputFormat::FORMAT_UNKNOWN },
  { AV_PIX_FMT_BAYER_RGGB8, AV_CODEC_ID_RAWVIDEO, V4L2_PIX_FMT_SRGGB8, VideoDisplay::VideoInputFormat::FORMAT_UNKNOWN },
};

namespace VideoDisplay
{
  static void setPixelFormatAndCodec(VideoInputFormat & inputFormat, uint32_t pixelformat)
  {
    inputFormat.pixelFormat.clear();
    inputFormat.vcodec.clear();
    for (FmtMap f: s_fmtConversionTable) {
      if (f.v4l2Fmt == pixelformat) {
        if (f.ffFmt != AV_PIX_FMT_NONE) {
          if (const char * pixelFormatStr = av_get_pix_fmt_name(f.ffFmt))
            inputFormat.pixelFormat = pixelFormatStr;
        } else {
          if (const char * codecStr = avcodec_get_name(f.codecId))
            inputFormat.vcodec = codecStr;
        }
        inputFormat.category = f.category;
        break;
      }
    }
  }

  std::vector<VideoInputFormat> scanInputFormats(
      const QString & input, AVInputFormat *, QMap<QString, QString>)
  {
    std::vector<VideoInputFormat> ret;

    int fd = open(input.toUtf8().data(), O_RDWR);
    if (fd == -1) {
      const char * error = strerror(errno);
      Radiant::error("scanInputFormats # failed to open %s: %s", input.toUtf8().data(), error);
      return ret;
    }

    struct v4l2_fmtdesc fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    VideoInputFormat inputFormat;

    // Iterate all pixel formats
    for (fmt.index = 0; ioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0; ++fmt.index) {
      v4l2_frmsizeenum size;
      memset(&size, 0, sizeof(size));
      size.pixel_format = fmt.pixelformat;

      if (fmt.flags & V4L2_FMT_FLAG_COMPRESSED)
        inputFormat.category = VideoInputFormat::FORMAT_COMPRESSED;
      else
        inputFormat.category = VideoInputFormat::FORMAT_UNKNOWN;

      setPixelFormatAndCodec(inputFormat, fmt.pixelformat);

      // Iterate all resolutions for this pixel format
      for (size.index = 0; ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &size) == 0; ++size.index) {
        v4l2_frmivalenum frameInt;
        memset(&frameInt, 0, sizeof(frameInt));
        frameInt.pixel_format = fmt.pixelformat;

        if (size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
          frameInt.width = size.discrete.width;
          frameInt.height = size.discrete.height;
          inputFormat.resolution.make(frameInt.width, frameInt.height);
        } else {
          frameInt.width = 0;
          frameInt.height = 0;
          inputFormat.resolution = Nimble::SizeI();
        }

        // Iterate all fps values for this pixel format and resolution
        for (frameInt.index = 0; ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frameInt) == 0; ++frameInt.index) {
          if (frameInt.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
            // notice that we are converting the frame interval to frame rate here
            inputFormat.fps = float(frameInt.discrete.denominator) / frameInt.discrete.numerator;
            ret.push_back(inputFormat);
          } else if (frameInt.type == V4L2_FRMIVAL_TYPE_CONTINUOUS) {
            // max frame internal is the min fps
            float minfps = std::max(1.f, float(frameInt.stepwise.max.denominator) / frameInt.stepwise.max.numerator);
            float maxfps = float(frameInt.stepwise.min.denominator) / frameInt.stepwise.min.numerator;
            for (float fps = minfps; fps < maxfps+1; fps *= 2) {
              if (std::abs(fps - 15.f) < 2.f)
                fps = 15.f;
              inputFormat.fps = fps;
              ret.push_back(inputFormat);
            }
          }
        }
        if (frameInt.index == 0) {
          inputFormat.fps = 0;
          ret.push_back(inputFormat);
        }
      }
    }

    close(fd);

    return ret;
  }
}

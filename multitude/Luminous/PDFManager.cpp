#include "PDFManager.hpp"
#include "Image.hpp"

#include <Radiant/PlatformUtils.hpp>

#include <Punctual/TaskWrapper.hpp>

#include <boost/expected/expected.hpp>

#include <fpdfview.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace
{
  struct BatchConverter
  {
    QString pdfAbsoluteFilePath;
    QString path;
    int pageNumber = 0;
    int pageCount = -1;
    std::atomic<int> queuedTasks{0};
    std::vector<folly::Promise<QString>> promises;
  };
  typedef std::shared_ptr<BatchConverter> BatchConverterPtr;

  /// How many image save tasks to have in the bg thread queue at the same time.
  /// Too large number and the application will consume more memory if bg thread
  /// is busy or saving the output files take longer than converting them. Too
  /// small number and the conversion is not as efficient as it could be.
  static const int s_maxQueuedTasks = 4;

  boost::expected<size_t> queryPageCount(const QString& pdfAbsoluteFilePath)
  {
    FPDF_DOCUMENT doc = FPDF_LoadDocument(pdfAbsoluteFilePath.toUtf8().data(), NULL);
    if(!doc)
      return boost::make_unexpected(
            std::runtime_error(QString("Could not open document %1.").
                               arg(pdfAbsoluteFilePath).toStdString()));

    int pageCount = FPDF_GetPageCount(doc);
    FPDF_CloseDocument(doc);
    return pageCount;
  }

  boost::expected<QImage>
  renderPage(const QString& pdfAbsoluteFilePath, int pageNumber,
             const Nimble::SizeI& resolution, QRgb color)
  {
    FPDF_DOCUMENT doc = FPDF_LoadDocument(pdfAbsoluteFilePath.toUtf8().data(), NULL);
    if(!doc) {
      return boost::make_unexpected(
            std::runtime_error(QString("Could not open document %1.").
                               arg(pdfAbsoluteFilePath).toStdString()));
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageNumber);
    if(!page) {
      FPDF_CloseDocument(doc);
      return boost::make_unexpected(QString("Could not open requested page %1 from %2.").
                                    arg(QString::number(pageNumber), pdfAbsoluteFilePath).
                                    toStdString());
    }

    Nimble::SizeF targetResolution(FPDF_GetPageWidth(page), FPDF_GetPageHeight(page));
    targetResolution.fit(resolution.cast<float>(), Qt::KeepAspectRatio);

    Nimble::SizeI pixelSize = targetResolution.cast<int>();

    FPDF_BITMAP bitmap = FPDFBitmap_Create(pixelSize.width(), pixelSize.height(), 1);
    /// Will the bitmap first with the chosen color
    FPDFBitmap_FillRect(bitmap, 0, 0, pixelSize.width(), pixelSize.height(), color);
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, pixelSize.width(), pixelSize.height(), 0, 0);

    const uint8_t* buffer = static_cast<uint8_t*>(FPDFBitmap_GetBuffer(bitmap));
    const int bytesPerLine = pixelSize.width() * 4;
    QImage image(buffer, pixelSize.width(), pixelSize.height(), bytesPerLine, QImage::Format_ARGB32);

    QImage copy = image.copy();

    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(page);
    FPDF_CloseDocument(doc);

    return copy;
  }

  boost::expected<Nimble::SizeF>
  getPageSize(const QString& pdfAbsoluteFilePath, size_t pageNumber)
  {
    FPDF_DOCUMENT doc = FPDF_LoadDocument(pdfAbsoluteFilePath.toUtf8().data(), NULL);
    if(!doc) {
      return boost::make_unexpected(
            std::runtime_error(QString("Could not open document %1.").
                               arg(pdfAbsoluteFilePath).toStdString()));
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageNumber);
    if(!page) {
      FPDF_CloseDocument(doc);
      return boost::make_unexpected(QString("Could not open requested page %1 from %2.").
                                    arg(QString::number(pageNumber), pdfAbsoluteFilePath).
                                    toStdString());
    }

    Nimble::SizeF targetResolution(FPDF_GetPageWidth(page), FPDF_GetPageHeight(page));
    FPDF_ClosePage(page);
    FPDF_CloseDocument(doc);
    return targetResolution;
  }

  void batchConvert(BatchConverterPtr batchPtr, const Nimble::SizeI & resolution,
                    Radiant::Color bgColor, const QString & imageFormat)
  {
    BatchConverter & batch = *batchPtr;
    QRgb bg = bgColor.toQColor().rgba();

    /// Work max one second at a time
    const double maxWorkTime = 1.0;
    Radiant::Timer timer;

    FPDF_DOCUMENT doc = FPDF_LoadDocument(batch.pdfAbsoluteFilePath.toUtf8().data(), NULL);
    if (!doc) {
      /// This really shouldn't happen, unless someone deleted the file while
      /// we were processing it. Just break all remaining promises.
      std::runtime_error error(QString("Could not open document %1.").
                               arg(batch.pdfAbsoluteFilePath).toStdString());
      for (; batch.pageNumber < batch.pageCount; ++batch.pageNumber) {
        batch.promises[batch.pageNumber].setException(error);
      }
      return;
    }

    for (; batch.pageNumber < batch.pageCount; ++batch.pageNumber) {
      QString targetFile = QString("%2/%1.%3").arg(batch.pageNumber, 5, 10, QChar('0')).
          arg(batch.path, imageFormat);
      QFileInfo targetInfo(targetFile);
      if (targetInfo.exists() && targetInfo.size() > 0) {
        batch.promises[batch.pageNumber].setValue(std::move(targetFile));
        continue;
      }

      FPDF_PAGE page = FPDF_LoadPage(doc, batch.pageNumber);
      if (!page) {
        batch.promises[batch.pageNumber].setException(
              std::runtime_error(QString("Could not open page %2 from %1").
                                 arg(batch.pdfAbsoluteFilePath).arg(batch.pageNumber).
                                 toStdString()));

        if (timer.time() > maxWorkTime) {
          ++batch.pageNumber;
          break;
        }
        continue;
      }

      Nimble::SizeF targetResolution(FPDF_GetPageWidth(page), FPDF_GetPageHeight(page));
      targetResolution.fit(resolution.cast<float>(), Qt::KeepAspectRatio);
      Nimble::SizeI pixelSize = targetResolution.cast<int>();

      std::shared_ptr<Luminous::Image> image(new Luminous::Image());
      Luminous::PixelFormat pixelFormat = Luminous::PixelFormat::bgrUByte();
      int bitmapFormat = FPDFBitmap_BGR;

      /// Use BGR with non-alpha images, BGRA otherwise
      if (bgColor.alpha() < 0.999f) {
        pixelFormat = Luminous::PixelFormat::bgraUByte();
        bitmapFormat = FPDFBitmap_BGRA;
      }

      image->allocate(pixelSize.width(), pixelSize.height(), pixelFormat);
      /// Render directly to Luminous::Image buffer, no need to copy anything
      FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(pixelSize.width(), pixelSize.height(),
                                               bitmapFormat, image->data(), image->lineSize());

      /// Fill the bitmap first with the chosen color
      FPDFBitmap_FillRect(bitmap, 0, 0, pixelSize.width(), pixelSize.height(), bg);
      FPDF_RenderPageBitmap(bitmap, page, 0, 0, pixelSize.width(), pixelSize.height(), 0, 0);

      int pageNumber = batch.pageNumber;
      auto saveTask = std::make_shared<Radiant::SingleShotTask>([targetFile, image, pageNumber, batchPtr] () mutable {
        image->write(targetFile);
        batchPtr->promises[pageNumber].setValue(targetFile);
        image.reset();
        --batchPtr->queuedTasks;
      });
      saveTask->setPriority(Radiant::Task::PRIORITY_NORMAL - 1);
      ++batch.queuedTasks;
      Radiant::BGThread::instance()->addTask(std::move(saveTask));

      FPDFBitmap_Destroy(bitmap);
      FPDF_ClosePage(page);

      if (timer.time() > maxWorkTime || batch.queuedTasks.load() >= s_maxQueuedTasks) {
        ++batch.pageNumber;
        break;
      }
    }

    FPDF_CloseDocument(doc);
  }
}

namespace Luminous
{
  static std::mutex s_pdfiumMutex;

  class PDFManager::D
  {
  public:
    QString m_defaultCachePath;
  };

  PDFManager::PDFManager()
    : m_d(new D())
  {
    QString path = Radiant::PlatformUtils::localAppPath();
    if (path.isEmpty())
      path = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    path += "/MultiTaction/cornerstone/cache/pdfs";

    m_d->m_defaultCachePath = path;

    FPDF_InitLibrary();
  }

  PDFManager::~PDFManager()
  {
    FPDF_DestroyLibrary();
  }

  folly::Future<size_t> PDFManager::queryPageCount(const QString& pdfAbsoluteFilePath)
  {
    Punctual::WrappedTaskFunc<size_t> taskFunc = [pdfAbsoluteFilePath] ()
      -> Punctual::WrappedTaskReturnType<size_t>
    {
      if(!s_pdfiumMutex.try_lock()) {
        return Punctual::NotReadyYet();
      }
      auto count = ::queryPageCount(pdfAbsoluteFilePath);
      s_pdfiumMutex.unlock();
      return count;
    };
    return Punctual::createWrappedTask<size_t>(std::move(taskFunc));
  }

  folly::Future<QImage> PDFManager::renderPage(const QString& pdfAbsoluteFilePath,
                                               int pageNumber, const Nimble::SizeI& resolution,
                                               QRgb color)
  {
    std::function<Punctual::WrappedTaskReturnType<QImage>(void)> taskFunc =
      [pdfAbsoluteFilePath, pageNumber, resolution, color]()
        -> Punctual::WrappedTaskReturnType<QImage>
    {
      if(!s_pdfiumMutex.try_lock())
        return Punctual::NotReadyYet();
      auto image =::renderPage(pdfAbsoluteFilePath, pageNumber, resolution, color);
      s_pdfiumMutex.unlock();
      return image;
    };
    return Punctual::createWrappedTask<QImage>(std::move(taskFunc));
  }

  folly::Future<folly::Unit> PDFManager::renderPageToFile(const QString& pdfAbsoluteFilePath,
                                                          int pageNumber,
                                                          const QString& pageAbsoluteFilePath,
                                                          const Nimble::SizeI& resolution,
                                                          QRgb color)
  {
    auto writeImage = [pageAbsoluteFilePath, pdfAbsoluteFilePath, pageNumber] (QImage im) {
      bool ok = im.save(pageAbsoluteFilePath);
      if(!ok)
        throw std::runtime_error(QString("Could not save page %1 of %2 as %3").
                                 arg(QString::number(pageNumber), pdfAbsoluteFilePath,
                                     pageAbsoluteFilePath).toStdString());
    };
    return renderPage(pdfAbsoluteFilePath, pageNumber, resolution, color).then(writeImage);
  }


  folly::Future<Nimble::SizeF>
  PDFManager::getPageSize(const QString& pdfAbsoluteFilePath, size_t pageNumber)
  {
    Punctual::WrappedTaskFunc<Nimble::SizeF> taskFunc =
      [pdfAbsoluteFilePath, pageNumber]()
        -> Punctual::WrappedTaskReturnType<Nimble::SizeF>
    {
      if(!s_pdfiumMutex.try_lock())
        return Punctual::NotReadyYet();
      auto size = ::getPageSize(pdfAbsoluteFilePath, pageNumber);
      s_pdfiumMutex.unlock();
      return size;
    };
    return Punctual::createWrappedTask<Nimble::SizeF>(std::move(taskFunc));
  }

  folly::Future<PDFManager::CachedPDFDocument> PDFManager::renderDocumentToCacheDir(
      const QString & pdfFilename, const Nimble::SizeI & resolution,
      Radiant::Color bgColor, const QString & cachePath,
      const QString & imageFormat)
  {
    BatchConverterPtr batchConverter { new BatchConverter() };
    /// Make a copy of the default cache path now and not asynchronously when
    /// it could have been changed.
    QString cacheRoot = cachePath.isEmpty() ? defaultCachePath() : cachePath;
    Punctual::WrappedTaskFunc<CachedPDFDocument> taskFunc =
        [pdfFilename, resolution, bgColor, cacheRoot, imageFormat, batchConverter]()
        -> Punctual::WrappedTaskReturnType<CachedPDFDocument>
    {
      if (batchConverter->path.isNull()) {
        batchConverter->pdfAbsoluteFilePath = QFileInfo(pdfFilename).absoluteFilePath();

        // Sha1 is used because it's really fast
        QCryptographicHash hash(QCryptographicHash::Sha1);
        QFile file(pdfFilename);
        if (!file.open(QFile::ReadOnly))
          return boost::make_unexpected(QString("Could not open input file %1: %2").
                                        arg(file.fileName(), file.errorString()).toStdString());
        hash.addData(file.readAll());
        hash.addData((const char*)&bgColor, sizeof(bgColor));
        hash.addData((const char*)&resolution, sizeof(resolution));
        hash.addData(imageFormat.toUtf8());
        batchConverter->path = QString("%1/%2").arg(cacheRoot, hash.result().toHex().data());

        if (!QDir().mkpath(batchConverter->path))
          boost::make_unexpected(QString("Failed to create cache path %1").
                                 arg(batchConverter->path).toStdString());
      }

      if (!s_pdfiumMutex.try_lock())
        return Punctual::NotReadyYet();
      auto count = ::queryPageCount(batchConverter->pdfAbsoluteFilePath);
      s_pdfiumMutex.unlock();

      if (!count.valid())
        return boost::make_unexpected(count.error());

      batchConverter->pageCount = count.value();

      batchConverter->promises.resize(batchConverter->pageCount);

      CachedPDFDocument doc;
      doc.cachePath = batchConverter->path;
      doc.pages.reserve(batchConverter->pageCount);
      for (auto & p: batchConverter->promises)
        doc.pages.push_back(p.getFuture());

      Radiant::FunctionTask::executeInBGThread([batchConverter, resolution, bgColor, imageFormat] (Radiant::Task & task) {
        if (batchConverter->queuedTasks.load() >= s_maxQueuedTasks) {
          task.scheduleFromNowSecs(0.1);
          return;
        }

        if (!s_pdfiumMutex.try_lock()) {
          task.scheduleFromNowSecs(0.01);
          return;
        }

        batchConvert(batchConverter, resolution, bgColor, imageFormat);
        s_pdfiumMutex.unlock();
        if (batchConverter->pageNumber >= batchConverter->pageCount)
          task.setFinished();
        else if (batchConverter->queuedTasks.load() >= s_maxQueuedTasks)
          task.scheduleFromNowSecs(0.1);
      });

      return std::move(doc);
    };
    return Punctual::createWrappedTask<CachedPDFDocument>(std::move(taskFunc));
  }

  void PDFManager::setDefaultCachePath(const QString & cachePath)
  {
    m_d->m_defaultCachePath = cachePath;
  }

  const QString & PDFManager::defaultCachePath() const
  {
    return m_d->m_defaultCachePath;
  }

  DEFINE_SINGLETON(PDFManager)
}

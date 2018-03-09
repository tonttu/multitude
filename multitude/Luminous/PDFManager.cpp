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
    std::shared_ptr<std::vector<folly::Promise<QString>>> promises;
  };

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

  void batchConvert(BatchConverter & batch, const Nimble::SizeI & resolution,
                    Radiant::Color bgColor, const QString & imageFormat)
  {
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
        (*batch.promises)[batch.pageNumber].setException(error);
      }
      return;
    }

    auto promises = batch.promises;
    for (; batch.pageNumber < batch.pageCount; ++batch.pageNumber) {
      QString targetFile = QString("%2/%1.%3").arg(batch.pageNumber, 5, 10, QChar('0')).
          arg(batch.path, imageFormat);
      QFileInfo targetInfo(targetFile);
      if (targetInfo.exists() && targetInfo.size() > 0) {
        (*batch.promises)[batch.pageNumber].setValue(std::move(targetFile));
        continue;
      }

      FPDF_PAGE page = FPDF_LoadPage(doc, batch.pageNumber);
      if (!page) {
        (*batch.promises)[batch.pageNumber].setException(
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

      folly::Promise<QString> * promise = &(*batch.promises)[batch.pageNumber];
      /// Capture promises to the lambda to keep the promise alive
      auto saveTask = std::make_shared<Radiant::SingleShotTask>([targetFile, image, promise, promises] {
        image->write(targetFile);
        promise->setValue(targetFile);
      });
      saveTask->setPriority(Radiant::Task::PRIORITY_NORMAL - 1);
      Radiant::BGThread::instance()->addTask(std::move(saveTask));

      FPDFBitmap_Destroy(bitmap);
      FPDF_ClosePage(page);

      if (timer.time() > maxWorkTime) {
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
    BatchConverter self;
    /// Make a copy of the default cache path now and not asynchronously when
    /// it could have been changed.
    QString cacheRoot = cachePath.isEmpty() ? defaultCachePath() : cachePath;
    Punctual::WrappedTaskFunc<CachedPDFDocument> taskFunc =
        [pdfFilename, resolution, bgColor, cacheRoot, imageFormat, self]() mutable
        -> Punctual::WrappedTaskReturnType<CachedPDFDocument>
    {
      if (self.path.isNull()) {
        self.pdfAbsoluteFilePath = QFileInfo(pdfFilename).absoluteFilePath();

        // Sha1 is used because it's really fast
        QCryptographicHash hash(QCryptographicHash::Sha1);
        QFile file(pdfFilename);
        if (!file.open(QFile::ReadOnly))
          throw std::runtime_error(QString("Could not open input file %1: %2").
                                   arg(file.fileName(), file.errorString()).toStdString());
        hash.addData(file.readAll());
        hash.addData((const char*)&bgColor, sizeof(bgColor));
        hash.addData((const char*)&resolution, sizeof(resolution));
        hash.addData(imageFormat.toUtf8());
        self.path = QString("%1/%2").arg(cacheRoot, hash.result().toHex().data());

        if (!QDir().mkpath(self.path))
          throw std::runtime_error(QString("Failed to create cache path %1").
                                   arg(self.path).toStdString());
      }

      if (!s_pdfiumMutex.try_lock())
        return Punctual::NotReadyYet();
      auto count = ::queryPageCount(self.pdfAbsoluteFilePath);
      s_pdfiumMutex.unlock();

      // Rethrows the exception if it fails
      self.pageCount = count.value();

      self.promises.reset(new std::vector<folly::Promise<QString>>(self.pageCount));

      CachedPDFDocument doc;
      doc.cachePath = self.path;
      doc.pages.reserve(self.pageCount);
      for (auto & p: *self.promises)
        doc.pages.push_back(p.getFuture());

      Radiant::FunctionTask::executeInBGThread([self, resolution, bgColor, imageFormat] (Radiant::Task & task) mutable {
        if (!s_pdfiumMutex.try_lock()) {
          task.scheduleFromNowSecs(0.01);
          return;
        }

        batchConvert(self, resolution, bgColor, imageFormat);
        s_pdfiumMutex.unlock();
        if (self.pageNumber >= self.pageCount)
          task.setFinished();
      });

      return doc;
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

  const std::shared_ptr<PDFManager>& PDFManager::instance()
  {
    static auto ptr = std::make_shared<PDFManager>();
    return ptr;
  }

}

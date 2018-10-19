#include "PDFManager.hpp"
#include "Image.hpp"

#include <Radiant/PlatformUtils.hpp>

#include <Punctual/TaskWrapper.hpp>

#include <boost/expected/expected.hpp>

#include <folly/MoveWrapper.h>

#include <fpdfview.h>

#if !defined(__APPLE__)
  #include <fpdf_edit.h>
  #include <fpdf_save.h>
  #include <fpdf_annot.h>
#endif

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QBuffer>

namespace
{
  /// This should be increased every time we make a cache-breaking change to the renderer
  static const char * rendererVersion = "1";

  static std::mutex s_pdfiumMutex;

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
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, pixelSize.width(), pixelSize.height(), 0, FPDF_ANNOT);

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

  void batchConvert(BatchConverterPtr batchPtr, const Luminous::PDFManager::PDFCachingOptions & opts)
  {
    BatchConverter & batch = *batchPtr;
    QRgb bg = opts.bgColor.toQColor().rgba();

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
          arg(batch.path, opts.imageFormat);
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
      targetResolution.fit(opts.resolution.cast<float>(), Qt::KeepAspectRatio);
      Nimble::SizeI pixelSize = targetResolution.cast<int>();

      std::shared_ptr<Luminous::Image> image(new Luminous::Image());
      Luminous::PixelFormat pixelFormat = Luminous::PixelFormat::bgrUByte();
      int bitmapFormat = FPDFBitmap_BGR;

      /// Use BGR with non-alpha images, BGRA otherwise
      if (opts.bgColor.alpha() < 0.999f) {
        pixelFormat = Luminous::PixelFormat::bgraUByte();
        bitmapFormat = FPDFBitmap_BGRA;
      }

      image->allocate(pixelSize.width(), pixelSize.height(), pixelFormat);
      /// Render directly to Luminous::Image buffer, no need to copy anything
      FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(pixelSize.width(), pixelSize.height(),
                                               bitmapFormat, image->data(), image->lineSize());

      /// Fill the bitmap first with the chosen color
      FPDFBitmap_FillRect(bitmap, 0, 0, pixelSize.width(), pixelSize.height(), bg);
      FPDF_RenderPageBitmap(bitmap, page, 0, 0, pixelSize.width(), pixelSize.height(), 0, FPDF_ANNOT);

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

  /////////////////////////////////////////////////////////////////////////////

#if !defined(__APPLE__)

  class PDFPAnnotationImpl : public Luminous::PDFPAnnotation
  {
  public:
    PDFPAnnotationImpl(FPDF_ANNOTATION annotation)
      : m_annotation(annotation)
    {
      assert(m_annotation);
    }

    ~PDFPAnnotationImpl() override
    {
      assert(m_annotation);
      std::lock_guard<std::mutex> guard(s_pdfiumMutex);
      FPDFPage_CloseAnnot(m_annotation);
      if (m_path)
        FPDFPageObj_Destroy(m_path);
    }

    bool startDraw(const Nimble::Vector2f& start, const Radiant::Color& color, float strokeWidth) override
    {
      if (m_path)
        return false;
      std::lock_guard<std::mutex> guard(s_pdfiumMutex);

      m_path = FPDFPageObj_CreateNewPath(start.x, start.y);
      if (!m_path)
        return false;

      if (!FPDFPath_SetDrawMode(m_path, 0 /*NoFill*/, 1 /*DrawPathStroke*/))
        return false;

      const auto r = Nimble::Math::Clamp(static_cast<unsigned int>(color.red() * 255u), 0u, 255u);
      const auto g = Nimble::Math::Clamp(static_cast<unsigned int>(color.green() * 255u), 0u, 255u);
      const auto b = Nimble::Math::Clamp(static_cast<unsigned int>(color.blue() * 255u), 0u, 255u);
      const auto a = Nimble::Math::Clamp(static_cast<unsigned int>(color.alpha() * 255u), 0u, 255u);
      if (!FPDFPath_SetStrokeColor(m_path, r, g, b, a))
        return false;

      if (!FPDFPath_SetStrokeWidth(m_path, strokeWidth))
        return false;

      return true;
    }

    bool lineTo(const Nimble::Vector2f& point) override
    {
      if (!m_path)
        return false;
      std::lock_guard<std::mutex> guard(s_pdfiumMutex);
      return FPDFPath_LineTo(m_path, point.x, point.y);
    }

    bool endDraw() override
    {
      if (!m_path)
        return false;
      assert(m_annotation);
      std::lock_guard<std::mutex> guard(s_pdfiumMutex);
      FPDF_BOOL res = FPDFAnnot_AppendObject(m_annotation, m_path);
      m_path = nullptr;
      return res;
    }
  private:
    FPDF_PAGEOBJECT m_path = nullptr;
    FPDF_ANNOTATION m_annotation = nullptr;
  };

  /////////////////////////////////////////////////////////////////////////////

  class PDFPageImpl : public Luminous::PDFPage
  {
  public:
    PDFPageImpl(FPDF_PAGE page)
      : m_page(page)
    {
      assert(m_page);
    }

    ~PDFPageImpl() override
    {
      assert(m_page);
      std::lock_guard<std::mutex> guard(s_pdfiumMutex);
      FPDF_ClosePage(m_page);
    }

    Nimble::SizeF size() const override
    {
      assert(m_page);
      std::lock_guard<std::mutex> guard(s_pdfiumMutex);
      const auto width = FPDF_GetPageWidth(m_page);
      const auto height = FPDF_GetPageHeight(m_page);
      return {static_cast<float>(width), static_cast<float>(height)};
    }

    Luminous::PDFPAnnotationPtr createAnnotation() override
    {
      assert(m_page);
      std::lock_guard<std::mutex> guard(s_pdfiumMutex);
      assert(FPDFAnnot_IsSupportedSubtype(FPDF_ANNOT_STAMP));

      FPDF_ANNOTATION annotation = FPDFPage_CreateAnnot(m_page, FPDF_ANNOT_STAMP);
      if (!annotation)
        return nullptr;

      auto result = std::make_shared<PDFPAnnotationImpl>(annotation);

      FS_RECTF rect;
      rect.left = 0;
      rect.bottom = 0;
      rect.right = FPDF_GetPageWidth(m_page);
      rect.top = FPDF_GetPageHeight(m_page);

      if (!FPDFAnnot_SetRect(annotation, &rect))
        return nullptr;

      return result;
    }

    bool generateContent() override
    {
      assert(m_page);
      std::lock_guard<std::mutex> guard(s_pdfiumMutex);
      return FPDFPage_GenerateContent(m_page);
    }
  private:
    FPDF_PAGE m_page = nullptr;
  };

  /////////////////////////////////////////////////////////////////////////////

  class QBufferWriter : public FPDF_FILEWRITE
  {
  public:
    QBufferWriter()
    {
      version = 1;
      WriteBlock = &QBufferWriter::writeBlockThunk;

      m_buffer.reset(new QBuffer());
      m_buffer->open(QBuffer::ReadWrite);
    }

    std::unique_ptr<QIODevice> takeBuffer()
    {
      assert(m_buffer);
      return std::move(m_buffer);
    }
  private:
    static int writeBlockThunk(struct FPDF_FILEWRITE_* pThis, const void* pData, unsigned long size)
    {
      return static_cast<QBufferWriter*>(pThis)->writeBlock(pData, size);
    }

    int writeBlock(const void* pData, unsigned long size)
    {
      assert(m_buffer);
      assert(m_buffer->isOpen());
      return m_buffer->write(static_cast<const char*>(pData), size);
    }

    std::unique_ptr<QIODevice> m_buffer;
  };

  /////////////////////////////////////////////////////////////////////////////

  class PDFDocumentImpl : public Luminous::PDFDocument
  {
  public:
    PDFDocumentImpl(FPDF_DOCUMENT doc)
      : m_doc(doc)
    {
      assert(m_doc);
    }

    ~PDFDocumentImpl() override
    {
      assert(m_doc);
      std::lock_guard<std::mutex> guard(s_pdfiumMutex);
      FPDF_CloseDocument(m_doc);
    }

    int pageCount() const override
    {
      assert(m_doc);
      std::lock_guard<std::mutex> guard(s_pdfiumMutex);
      return FPDF_GetPageCount(m_doc);
    }

    Luminous::PDFPagePtr openPage(int index) override
    {
      assert(m_doc);
      std::lock_guard<std::mutex> guard(s_pdfiumMutex);
      FPDF_PAGE page = FPDF_LoadPage(m_doc, index);
      if (!page)
        return nullptr;

      return std::make_shared<PDFPageImpl>(page);
    }

    virtual std::unique_ptr<QIODevice> save() override
    {
      assert(m_doc);
      std::lock_guard<std::mutex> guard(s_pdfiumMutex);
      QBufferWriter writer;
      if (!FPDF_SaveAsCopy(m_doc, &writer, 0))
        return nullptr;

      return writer.takeBuffer();
    }
  private:
    FPDF_DOCUMENT m_doc = nullptr;
  };

#endif // #if !defined(__APPLE__)

} // anonymous namespace

namespace Luminous
{

#if !defined(__APPLE__)
  PDFPAnnotation::~PDFPAnnotation()
  {
  }

  PDFPage::~PDFPage()
  {
  }

  PDFDocument::~PDFDocument()
  {
  }
#endif

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
      const QString & pdfFilename, const PDFCachingOptions & opts, int maxPageCount)
  {
    BatchConverterPtr batchConverter { new BatchConverter() };
    /// Make a copy of the default cache path now and not asynchronously when
    /// it could have been changed.
    QString cacheRoot = opts.cachePath.isEmpty() ? defaultCachePath() : opts.cachePath;
    Punctual::WrappedTaskFunc<CachedPDFDocument> taskFunc =
        [pdfFilename, opts, cacheRoot, maxPageCount, batchConverter]()
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
        hash.addData((const char*)&opts.bgColor, sizeof(opts.bgColor));
        hash.addData((const char*)&opts.resolution, sizeof(opts.resolution));
        hash.addData(opts.imageFormat.toUtf8());
        hash.addData(rendererVersion);
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

      batchConverter->pageCount = std::min<int>(maxPageCount, count.value());

      batchConverter->promises.resize(batchConverter->pageCount);

      CachedPDFDocument doc;
      doc.cachePath = batchConverter->path;
      // Use the real value here instead of the value limited by maxPageCount
      doc.pageCount = count.value();
      doc.pages.reserve(batchConverter->promises.size());
      for (auto & p: batchConverter->promises)
        doc.pages.push_back(p.getFuture());

      Radiant::FunctionTask::executeInBGThread([batchConverter, opts] (Radiant::Task & task) {
        if (batchConverter->queuedTasks.load() >= s_maxQueuedTasks) {
          task.scheduleFromNowSecs(0.1);
          return;
        }

        if (!s_pdfiumMutex.try_lock()) {
          task.scheduleFromNowSecs(0.01);
          return;
        }

        batchConvert(batchConverter, opts);
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

#if !defined(__APPLE__)
  PDFDocumentPtr PDFManager::editDocument(const QString& pdfAbsoluteFilePath)
  {
    std::lock_guard<std::mutex> guard(s_pdfiumMutex);

    FPDF_DOCUMENT doc = FPDF_LoadDocument(pdfAbsoluteFilePath.toUtf8().data(), 0);
    if (!doc)
      return nullptr;
    return std::make_shared<PDFDocumentImpl>(doc);
  }
#endif // #if !defined(__APPLE__)

  DEFINE_SINGLETON(PDFManager)
}

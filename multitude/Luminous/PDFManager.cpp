#include "PDFManager.hpp"

#include <Punctual/TaskWrapper.hpp>

#include <boost/expected/expected.hpp>

#include <fpdfview.h>

#include <Radiant/Sleep.hpp>

namespace
{

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

}

namespace Luminous
{

  static std::mutex s_pdfiumMutex;

  PDFManager::PDFManager()
  {
    FPDF_InitLibrary();
  }

  PDFManager::~PDFManager()
  {
    FPDF_DestroyLibrary();
  }

  folly::Future<size_t> PDFManager::queryPageCount(const QString& pdfAbsoluteFilePath)
  {
    auto taskFunc = [pdfAbsoluteFilePath] () -> Punctual::WrappedTaskReturnType<size_t>  {
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
    auto taskFunc = [pdfAbsoluteFilePath, pageNumber, resolution, color]()
        -> Punctual::WrappedTaskReturnType<QImage> {
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
    auto taskFunc = [pdfAbsoluteFilePath, pageNumber]()
        -> Punctual::WrappedTaskReturnType<Nimble::SizeF> {
      if(!s_pdfiumMutex.try_lock())
        return Punctual::NotReadyYet();
      auto size = ::getPageSize(pdfAbsoluteFilePath, pageNumber);
      s_pdfiumMutex.unlock();
      return size;
    };
    return Punctual::createWrappedTask<Nimble::SizeF>(std::move(taskFunc));
  }

  const std::shared_ptr<PDFManager>& PDFManager::instance()
  {
    static auto ptr = std::make_shared<PDFManager>();
    return ptr;
  }

}

#pragma once

#include "Export.hpp"

#include <Nimble/Size.hpp>
#include <folly/futures/Future.h>

#include <memory>
#include <QImage>

namespace Luminous
{

  class LUMINOUS_API PDFManager
  {
  public:
    static const std::shared_ptr<PDFManager>& instance();

    PDFManager();
    ~PDFManager();

    /// @param pdfAbsoluteFilePath absolute file path of the pdf file
    /// @return number of pages. If operation failed contains
    ///         std::runtime_exception with error message
    folly::Future<size_t> queryPageCount(const QString& pdfAbsoluteFilePath);

    /// @param pdfAbsoluteFilePath absolute file path of the pdf file
    /// @param pageNumber page to render
    /// @param resolution Target resolution of the rendered result. Actual result
    ///                   can be smaller as the aspect ratio is preserved
    /// @return QImage containing rendered page. If operation failed contains
    ///         std::runtime_exception with error message
    folly::Future<QImage> renderPage(const QString& pdfAbsoluteFilePath, int pageNumber,
                                     const Nimble::SizeI& resolution, QRgb color = 0x00FFFFFF);

    /// @param pdfAbsoluteFilePath absolute file path of the pdf file
    /// @param pageNumber page to render
    /// @param pageAbsoluteFilePath file path for the rendered result
    /// @param resolution Target resolution of the rendered result. Actual result
    ///                   can be smaller as the aspect ratio is preserved
    /// @return is fulfilled when operation is done. If operation failed contains
    ///         std::runtime_exception with error message
    folly::Future<folly::Unit> renderPageToFile(const QString& pdfAbsoluteFilePath,
                                                int pageNumber,
                                                const QString& pageAbsoluteFilePath,
                                                const Nimble::SizeI& resolution,
                                                QRgb color = 0x00FFFFFF);

    /// @param pdfAbsoluteFilePath absolute file path of the pdf file
    /// @param pageNumber number of pdf-page
    /// @return page size in points where single point is ~0.3528mm. If operation
    ///         failed contains std::runtime_exception with error message
    folly::Future<Nimble::SizeF> getPageSize(const QString& pdfAbsoluteFilePath, size_t pageNumber);
  };

}

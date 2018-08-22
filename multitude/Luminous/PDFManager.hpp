#pragma once

#include "Export.hpp"

#include <Radiant/Color.hpp>
#include <Radiant/Singleton.hpp>

#include <Nimble/Size.hpp>
#include <folly/futures/Future.h>

#include <memory>
#include <QImage>

namespace Luminous
{

  class LUMINOUS_API PDFManager
  {
    DECLARE_SINGLETON(PDFManager);

  public:
    /// One PDF document written to a cache. There will be exactly one image
    /// file for each page in the PDF document.
    struct CachedPDFDocument
    {
      /// Root path for the images, it's based on the checksum of the document
      /// and render parameters, so it's unique and doesn't contain other files.
      QString cachePath;

      /// Number of pages in the document. This might not be the same as the
      /// number of items in the "pages" vector, if you have set the
      /// maxPageCount parameter when calling renderDocumentToCacheDir.
      int pageCount = 0;

      /// Filenames of the image files for all requested pages
      ///
      /// To wait for the whole document to finish, you can write something like:
      /// folly::collectAll(pages).then([](const std::vector<folly::Try<QString>> & files) {});
      ///
      /// To get notifications once each page is finished, do this:
      /// folly::unorderedReduce(pages, true, [](bool, const QString & filename) {
      ///   // do something with filename
      ///   return true;
      /// });
      ///
      /// Or you can use folly::reduce in the last example to get notifications
      /// in page order.
      std::vector<folly::Future<QString>> pages;
    };

    struct PDFCachingOptions
    {
      /// Target resolution of the rendered result. Actual result can be smaller
      /// as the aspect ratio is preserved
      Nimble::SizeI resolution;
      /// Background color to be used with the generated images. If the color
      /// is translucent, cache will contain images with an alpha channel.
      Radiant::Color bgColor;
      /// Cache root to use. If empty, defaultCachePath() will be used instead.
      /// Actual files will be written to a subdirectory, for example the first
      /// page will look like: <cachePath>/<sha1(file, params)>/00000.csimg
      QString cachePath;
      /// Image format (file extension) for the cached files. Unless you have a
      /// great reason to change it, use the default "csimg" which is by far
      /// the fastest image format to encode and decode.
      QString imageFormat = "csimg";
    };

  public:
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

    /// Set the default cache path used with renderDocumentToCacheDir
    void setDefaultCachePath(const QString & cachePath);
    /// The default cache path used with renderDocumentToCacheDir
    /// On Windows this is initially %LOCALAPPDATA%/MultiTaction/cornerstone/cache/pdfs
    /// and on other platforms $HOME/MultiTaction/cornerstone/cache/pdfs
    const QString & defaultCachePath() const;

    /// @param pdfFilename pdf filename
    /// @param opts caching options, see PDFCachingOptions
    /// @param maxPageCount The number of pages to cache at max
    /// @return cached pdf document object with the unique document cache
    ///         directory and futures for individual pages. If the operation
    ///         fails, the returned future contains std::runtime_error
    folly::Future<CachedPDFDocument> renderDocumentToCacheDir(const QString & pdfFilename,
                                                              const PDFCachingOptions & opts,
                                                              int maxPageCount = std::numeric_limits<int>::max());

  private:
    class D;
    std::unique_ptr<D> m_d;
  };

}

#pragma once

#include "Export.hpp"

#include <Radiant/Color.hpp>
#include <Radiant/Singleton.hpp>

#include <Nimble/Size.hpp>
#include <folly/futures/Future.h>

#include <memory>
#include <QImage>
#include <QIODevice>

namespace Luminous
{
#if !defined(__APPLE__)
  ///
  /// @brief Represents stamp annotation
  /// This API is experimental one and are subject to change in future
  class LUMINOUS_API PDFPAnnotation
  {
  public:
    virtual ~PDFPAnnotation();

    ///
    /// @brief Starts drawing new path
    /// @param start Starting point in the page coordinate system
    /// @param color Stroke color
    /// @param strokeWidth Stroke width
    /// @return true on success
    ///
    virtual bool startDraw(const Nimble::Vector2f& start, const Radiant::Color& color, float strokeWidth) = 0;
    ///
    /// @brief lineTo Draw a line to the point
    /// @param pt point in the page coordinate system
    /// @return true on success
    ///
    virtual bool lineTo(const Nimble::Vector2f& pt) = 0;
    ///
    /// @brief endDraw Ends drawing and attach created path to the annotation
    /// @return true on success
    ///
    virtual bool endDraw() = 0;
  };

  using PDFPAnnotationPtr = std::shared_ptr<PDFPAnnotation>;

  ///
  /// @brief Represents PDF document page
  /// This API is experimental one and are subject to change in future
  class LUMINOUS_API PDFPage
  {
  public:
    virtual ~PDFPage();

    ///
    /// @brief size
    /// @return size of the page in points
    ///
    virtual Nimble::SizeF size() const = 0;
    ///
    /// @brief Creates a new stamp annotation covering the whole page
    /// @return annotation handle
    ///
    virtual PDFPAnnotationPtr createAnnotation() = 0;
    ///
    /// @brief Updates page content
    ///  This need to be called after all page edits was made
    /// @return true on success
    ///
    virtual bool generateContent() = 0;
  };

  using PDFPagePtr = std::shared_ptr<PDFPage>;

  ///
  /// @brief Represent PDF document that is possible to edit
  /// This API is experimental one and are subject to change in future
  class LUMINOUS_API PDFDocument
  {
  public:
    virtual ~PDFDocument();

    ///
    /// @brief pageCount
    /// @return page count in the document
    ///
    virtual int pageCount() const = 0;
    ///
    /// @brief Opens page for edit
    /// @param index Zero-based index of the page
    /// @return page handle on success or nullptr if failed
    ///
    virtual PDFPagePtr openPage(int index) = 0;
    ///
    /// @brief Saves document to a memory buffer
    /// @return stored document
    ///
    virtual std::unique_ptr<QIODevice> save() = 0;
  };

  using PDFDocumentPtr = std::shared_ptr<PDFDocument>;
#endif // #if !defined(__APPLE__)

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
#if !defined(__APPLE__)
    /// @brief Opens PDF file for edit
    /// @param pdfAbsoluteFilePath absolute file path of the pdf file
    /// @return handle to the opened pdf document on success or nullptr if failed
    PDFDocumentPtr editDocument(const QString& pdfAbsoluteFilePath);
#endif
  private:
    class D;
    std::unique_ptr<D> m_d;
  };

}

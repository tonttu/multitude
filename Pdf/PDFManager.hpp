/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#pragma once

#include <folly/futures/Future.h>

#include <boost/expected/expected.hpp>

#include "Export.hpp"

#include <Radiant/Color.hpp>
#include <Radiant/Singleton.hpp>

#include <Nimble/Size.hpp>

#include <memory>
#include <QImage>
#include <QIODevice>

namespace Pdf
{
#if !defined(__APPLE__)
  ///
  /// @brief Represents stamp annotation
  /// This API is experimental one and are subject to change in future
  class PDF_API PDFPAnnotation
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
    /// Draws a cubic Bezier curve from the current point
    /// @param c1 first control point
    /// @param c2 second control point
    /// @param p ending point
    /// @return true on success
    ///
    virtual bool bezierTo(const Nimble::Vector2f & c1, const Nimble::Vector2f & c2, const Nimble::Vector2f & p) = 0;
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
  class PDF_API PDFPage
  {
  public:
    virtual ~PDFPage();

    ///
    /// @brief size
    /// @return size of the page in points
    ///
    virtual Nimble::SizeF size() const = 0;

    enum class Rotation
    {
      NO_ROTATION,
      CLOCKWISE_90,
      CLOCKWISE_180,
      CLOCKWISE_270,
      UNKNOWN
    };

    ///
    /// \brief rotation
    /// \return return rotation of the page
    ///
    virtual Rotation rotation() const = 0;

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
  class PDF_API PDFDocument
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

  class PDF_API PDFManager
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
      /// Image format (file extension) for the cached files.
      /// The default is "csimg" if ENABLE_LUMINOUS is defined. This is by far
      /// the fastest image format to encode and decode.
      /// If ENABLE_LUMINOUS is not defined, the default is then "webp", which
      /// is great for optimizing disk space / bandwidth, but is lossy and
      /// takes more resources to encode and decode.
      QString imageFormat;
    };

  public:
    ~PDFManager();

    /// @param pdfAbsoluteFilePath absolute file path of the pdf file
    /// @return number of pages. If operation failed contains
    ///         std::exception with error message
    folly::Future<int> queryPageCount(const QString& pdfAbsoluteFilePath);

    /// @param pdfAbsoluteFilePath absolute file path of the pdf file
    /// @param pageNumber page to render. Page indexing starts from zero.
    /// @param resolution Target resolution of the rendered result. Actual result
    ///                   can be smaller as the aspect ratio is preserved
    /// @return QImage containing rendered page. If operation failed contains
    ///         std::exception with error message
    folly::Future<QImage> renderPage(const QString& pdfAbsoluteFilePath, int pageNumber,
                                     const Nimble::SizeI& resolution, QRgb color = 0x00FFFFFF);

    /// Like renderPage, but works synchronously
    boost::expected<QImage, QString> renderPageSync(const QString & pdfAbsoluteFilePath, int pageNumber,
                                                    const Nimble::SizeI & resolution, QRgb color = 0x00FFFFFF);

    /// @param pdfAbsoluteFilePath absolute file path of the pdf file
    /// @param pageNumber page to render. Page indexing starts from zero.
    /// @param pageAbsoluteFilePath file path for the rendered result
    /// @param resolution Target resolution of the rendered result. Actual result
    ///                   can be smaller as the aspect ratio is preserved
    /// @return is fulfilled when operation is done. If operation failed contains
    ///         std::exception with error message
    folly::Future<folly::Unit> renderPageToFile(const QString& pdfAbsoluteFilePath,
                                                int pageNumber,
                                                const QString& pageAbsoluteFilePath,
                                                const Nimble::SizeI& resolution,
                                                QRgb color = 0x00FFFFFF);

    /// @param pdfAbsoluteFilePath absolute file path of the pdf file
    /// @param pageNumber number of pdf-page. Page indexing starts from zero.
    /// @return page size in points where single point is ~0.3528mm. If operation
    ///         failed contains std::exception with error message
    folly::Future<Nimble::SizeF> getPageSize(const QString& pdfAbsoluteFilePath, int pageNumber);

    /// Like getPageSize, but works synchronously
    /// @returns page size and the number of pages
    boost::expected<std::pair<Nimble::SizeF, int>, QString> pageSizeSync(
        const QString & pdfAbsoluteFilePath, int pageNumber);

    /// Set the default cache path used with renderDocumentToCacheDir
    void setDefaultCachePath(const QString & cachePath);
    /// The default cache path used with renderDocumentToCacheDir
    /// On Windows this is initially %LOCALAPPDATA%/MultiTaction/cache/pdfs
    /// and on other platforms $HOME/MultiTaction/cache/pdfs
    const QString & defaultCachePath() const;

    /// @param pdfFilename pdf filename
    /// @param opts caching options, see PDFCachingOptions
    /// @param maxPageCount The number of pages to cache at max
    /// @return cached pdf document object with the unique document cache
    ///         directory and futures for individual pages. If the operation
    ///         fails, the returned future contains std::runtime_error
    folly::Future<CachedPDFDocument> renderDocumentToCacheDir(const QString & pdfFilename,
                                                              PDFCachingOptions opts,
                                                              int maxPageCount = std::numeric_limits<int>::max());
#if !defined(__APPLE__)
    /// @brief Opens PDF file for edit
    /// @param pdfAbsoluteFilePath absolute file path of the pdf file
    /// @return handle to the opened pdf document on success or nullptr if failed
    PDFDocumentPtr editDocument(const QString& pdfAbsoluteFilePath);
#endif
  private:
    PDFManager();

    class D;
    std::unique_ptr<D> m_d;
  };
  typedef std::shared_ptr<PDFManager> PDFManagerPtr;
}

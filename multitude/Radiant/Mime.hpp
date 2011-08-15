#ifndef RADIANT_MIME_HPP
#define RADIANT_MIME_HPP

#include "Export.hpp"

#include <QString>

#include <map>

namespace Radiant
{
  /// see RFC 2046
  class RADIANT_API MimeType
  {
  public:
    /// Get mime type from string
    /// @param mime string of type "toplevel/subtype",
    /// @sa topLevel subType
    MimeType(const QString & mime);

    /// constructs a type from top-level and sub-type
    /// @param toplevel top-level mime type
    /// @param subtype sub-level mime type
    /// @sa topLevel
    MimeType(const QString & toplevel, const QString & subtype);

    /// Get the top-level
    /// @return toplevel of the mime type, eg. text from text/plain
    const QString & topLevel() const
    {
      return m_toplevel;
    }

    /// Get the sub-type
    /// @return subtype of the mime type, eg. plain from text/plain
    const QString & subType() const
    {
      return m_subtype;
    }

    /// Get the
    /// @return whole type, eg. text/plain
    const QString typeString() const
    {
      return m_toplevel + "/" + m_subtype;
    }

  private:
    // text/plain =>
    const QString m_toplevel; // text
    const QString m_subtype; // plain
  };

  class RADIANT_API MimeManager
  {
  protected:
    typedef std::map<QString, MimeType> ExtensionMap;

    /// shared extension map
    static ExtensionMap s_sharedExtensions;
    ExtensionMap m_extensions;

    static void initialize();

  public:
    MimeManager();
    virtual ~MimeManager();

    /// add or replace a shared mapping from file extension to mime type
    static void insertSharedExtension(const QString & extension, const MimeType & type);

    /// Add or replace a shared mapping from file extension to mime type
    /// @param extension filename extension
    /// @param type mime type
    void insertExtension(const QString & extension, const MimeType & type);

    /// Get the mime type by filename extension
    /// @param ext filename extension
    /// @return The matching MimeType, or NULL if not found
    const MimeType * mimeTypeByExtension(const QString & ext) const;
  };
}
#endif // MIME_HPP

/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_MIME_HPP
#define RADIANT_MIME_HPP

#include "Export.hpp"

#include <QString>

#include <map>

namespace Radiant
{
  /// See RFC 2046
  class RADIANT_API MimeType
  {
  public:
    /// Get mime type from string
    /// @param mime string of type "toplevel/subtype",
    /// @sa topLevel subType
    explicit MimeType(const QString & mime);

    /// constructs a type from top-level and sub-type
    /// @param toplevel top-level mime type
    /// @param subtype sub-level mime type
    /// @sa topLevel
    MimeType(const QString & toplevel, const QString & subtype);

    MimeType(const MimeType & t);
    MimeType & operator=(const MimeType & t);

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
    QString m_toplevel; // text
    QString m_subtype; // plain
  };

  /// This class keeps track of matching file extensions to mime types.
  class RADIANT_API MimeManager
  {
  protected:
    /// Map from file extensions to mime types
    typedef std::map<QString, MimeType> ExtensionMap;

    /// Global extension map shared by all instances
    static ExtensionMap s_sharedExtensions;
    /// Extension map specific for this instance
    ExtensionMap m_extensions;

    /// @cond

    static void initialize();

    /// @endcond

  public:
    MimeManager();
    virtual ~MimeManager();

    /// Add or replace a shared mapping from file extension to mime type
    /// @param extension file extension
    /// @param type mime type
    static void insertSharedExtension(const QString & extension, const MimeType & type);

    /// Add or replace a shared mapping from file extension to mime type
    /// @param extension filename extension
    /// @param type mime type
    void insertExtension(const QString & extension, const MimeType & type);

    /// Get the mime type by filename extension
    /// @param ext filename extension
    /// @return The matching MimeType, or NULL if not found
    const MimeType * mimeTypeByExtension(const QString & ext) const;

    /// Get a list of file extensions that match the given mime type
    /// @param mime mime type to query
    /// @return list of file extensions
    QStringList extensionsByMimeRegexp(const QString & mime) const;
  };
}
#endif // MIME_HPP

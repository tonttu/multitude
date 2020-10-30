#pragma once

#include "Export.hpp"

#include <QString>
#include <QVariant>

#include <memory>
#include <optional>

namespace Radiant
{
  /// Ini file parser and writer.
  class RADIANT_API IniFile
  {
  public:
    IniFile();
    ~IniFile();

    IniFile(const IniFile & other);
    IniFile & operator=(const IniFile & other);

    IniFile(IniFile && other);
    IniFile & operator=(IniFile && other);

    /// Reads and parses a file. Can merge keys from duplicate sections.
    /// Triggers a warning if you define the same key in multiple places,
    /// but doesn't fail parsing. Returns false on parse error.
    bool parseFile(const QString & filename);
    bool parseData(const QByteArray & data, const QString & sourceName = QString());

    /// Writes the ini file to a file or memory buffer. Keeps all old comments
    /// and formatting. If you haven't made any changes to the file, this will
    /// write exactly the same data that was originally parsed, except that
    /// this always has a trailing newline.
    ///
    /// Uses the original newline style (\r\n or \n) if the IniFile was parsed
    /// from existing file. Otherwise uses platform default.
    bool writeToFile(const QString & filename);
    QByteArray writeData() const;

    /// Returns the names of sections that start with the given prefix.
    /// For instance you might have sections [foo:a], [foo:b], so
    /// sections("foo:") would return "foo:a", "foo:b".
    ///
    /// If prefix is empty, returns all ini sections in the file.
    QStringList sections(const QString & prefix = QString()) const;

    /// Returns the full names of all keys in the given section.
    /// The name includes the section name, for example keys("foo")
    /// could return ["foo/bar"].
    QStringList keys(const QString & sectionName) const;

    /// Returns a single value given the full key (section/name). Returns
    /// an invalid QVariant if the value was not found.
    QVariant value(const QString & key) const;
    QVariant value(const QString & key, const QVariant & defaultValue) const;

    /// Sets key value. Key is the full name of the key, including the section
    /// name (section/name). If existing key is found, that is reused. If
    /// existing key is found, but is currently commented out with ';', the
    /// comment is removed and the value is reused. Otherwise a new key is
    /// created at the end of the section. The section is also created if
    /// not found.
    void setValue(const QString & key, const QVariant & value);

    /// Removes a key value
    void clearValue(const QString & key);

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
} // namespace Radiant

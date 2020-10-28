#include "IniFile.hpp"
#include "Trace.hpp"
#include "QByteArrayHash.hpp"

#include <Platform.hpp>

#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>

namespace Radiant
{
  namespace
  {
    struct Line
    {
      int origLineNumber = 0;
      QByteArray raw;
    };

    struct LineRange
    {
      std::list<Line>::iterator begin;
      std::list<Line>::iterator end;
    };

    struct Value
    {
      QString key;
      QString value;
      bool commented = false;

      std::list<Line>::iterator line;
    };

    struct Section
    {
      QString name;
      QByteArray nameLower;

      std::vector<LineRange> lines;

      std::unordered_map<QByteArray, std::vector<Value>> values;
    };

    std::pair<QString, QString> splitKey(const QString & fullKey)
    {
      int idx = fullKey.indexOf('/');
      if (idx < 0)
        return {QString(), fullKey};
      else
        return {fullKey.left(idx), fullKey.mid(idx + 1)};
    }

    QString unescapeKey(const QString & key)
    {
      /// QSettings uses percentage escaping (%xx) and unicode escaping (%Uxxxx)
      /// in keys and section names, but we don't really need that.
      return key.trimmed();
    }

    QString unescapeValue(const QString & value)
    {
      QString unescaped;
      bool quoted = false;
      for (int i = 0, s = value.size(); i < s; ++i) {
        QChar chr = value[i];
        if (i == 0 && chr == '"') {
          quoted = true;
          continue;
        }
        if (quoted && i == s - 1 && chr == '"')
          return unescaped;

        if (chr == '\\') {
          if (i == s - 1)
            // Newlines can't be escaped
            throw std::runtime_error("Unexpected escape character (\\) at the end of the line");

          QChar next = value[++i];
          switch (next.unicode()) {
          case '\\':
          case '\'':
          case '"':
          case ';':
          case '#':
          case '=':
          case ':':
            unescaped.append(next);
            break;
          case 'a':
            unescaped.append('\a');
            break;
          case 'b':
            unescaped.append('\b');
            break;
          case 't':
            unescaped.append('\t');
            break;
          case 'r':
            unescaped.append('\r');
            break;
          case 'n':
            unescaped.append('\n');
            break;
          case 'x':
            if (s - i >= 5) {
              bool ok = false;
              uint unicode = value.mid(i + 1, 4).toUInt(&ok, 16);
              if (!ok)
                throw std::runtime_error("Failed to parse unicode escape sequence, expected \\x???? with four hex characters");
              unescaped += QChar(unicode);
              i += 4;
            }
            break;
          default:
            throw std::runtime_error("Unknown escape character. Notice that backslash (\\) needs to be escaped (\\\\) in ini files");
          }
        } else {
          unescaped += chr;
        }
      }
      if (quoted)
        throw std::runtime_error("Missing double quote character (\") at the end of the line");

      return unescaped;
    }

    QByteArray escapeKeyAndValue(const QString & key, const QString & value)
    {
      QString escapedValue = value;
      escapedValue.replace("\\", "\\\\");
      escapedValue.replace("\a", "\\a");
      escapedValue.replace("\b", "\\b");
      escapedValue.replace("\t", "\\t");
      escapedValue.replace("\r", "\\r");
      escapedValue.replace("\n", "\\n");
      if (escapedValue.startsWith('"'))
        escapedValue.prepend('\\');
      return QString("%1=%2").arg(key, escapedValue).toUtf8();
    }

    QByteArray escapeSection(const QString & sectionName)
    {
      return QString("[%1]").arg(sectionName).toUtf8();
    }
  }

  class IniFile::D
  {
  public:
    void clear();
    Section * findSection(const QString & sectionName);
    Section & createGlobalSection();
    Section & createSection(const QString & sectionName);
    void clearValue(Section & section, const QString & key);
    void setValue(Section & section, const QString & key, const QString & value);
    void parseLine(const std::list<Line>::iterator & line, const QString & sourceName, Section *& currentSection);

    QByteArray m_newline
#ifdef RADIANT_WINDOWS
    {"\r\n"};
#else
    {"\n"};
#endif

    QRegularExpression m_sectionRegex{"^\\s*\\[(.+)\\]\\s*$"};
    QRegularExpression m_valueRegex{"^\\s*(;?)\\s*([^=]+)=(.*)$"};
    std::list<Line> m_lines;
    std::vector<Section> m_sections;
  };

  void IniFile::D::clear()
  {
    m_lines.clear();
    m_sections.clear();
  }

  Section * IniFile::D::findSection(const QString & sectionName)
  {
    QByteArray sectionLow = sectionName.toLower().toUtf8();
    for (Section & section: m_sections)
      if (section.nameLower == sectionLow)
        return &section;
    return nullptr;
  }

  Section & IniFile::D::createGlobalSection()
  {
    m_lines.emplace_front();
    m_lines.emplace_front();
    m_sections.emplace_back();
    Section & global = m_sections.back();
    global.lines.emplace_back();
    global.lines.back().begin = m_lines.begin();
    global.lines.back().end = ++m_lines.begin();
    return global;
  }

  Section & IniFile::D::createSection(const QString & sectionName)
  {
    QByteArray sectionLow = sectionName.toLower().toUtf8();
    for (Section & section: m_sections)
      if (section.nameLower == sectionLow)
        return section;

    if (sectionName.isEmpty())
      return createGlobalSection();

    m_lines.emplace_back();
    m_lines.back().raw = m_newline + escapeSection(sectionName) + m_newline;
    auto it = --m_lines.end();
    for (auto & s: m_sections)
      if (s.lines.back().end == m_lines.end())
        s.lines.back().end = it;

    m_sections.emplace_back();
    Section & section = m_sections.back();
    section.name = sectionName;
    section.nameLower = sectionLow;
    section.lines.emplace_back();
    section.lines.back().begin = it;
    section.lines.back().end = m_lines.end();
    return section;
  }

  void IniFile::D::clearValue(Section & section, const QString & key)
  {
    QByteArray keyLow = key.toLower().toUtf8();
    auto it = section.values.find(keyLow);
    if (it == section.values.end())
      return;

    std::vector<Value> & values = it->second;
    for (auto it = values.begin(); it != values.end();) {
      if (it->commented) {
        ++it;
      } else {
        m_lines.erase(it->line);
        it = values.erase(it);
      }
    }
    if (values.empty())
      section.values.erase(it);
  }

  void IniFile::D::setValue(Section & section, const QString & key, const QString & value)
  {
    QByteArray keyLow = key.toLower().toUtf8();
    std::vector<Value> & values = section.values[keyLow];

    for (auto it = values.rbegin(), end = values.rend(); it != end; ++it) {
      Value & v = *it;
      if (!v.commented) {
        v.key = key;
        v.value = value;
        v.line->raw = escapeKeyAndValue(key, value) + m_newline;
        return;
      }
    }

    for (auto it = values.rbegin(), end = values.rend(); it != end;) {
      Value & v = *it;
      v.key = key;
      v.value = value;
      v.commented = false;
      v.line->raw = escapeKeyAndValue(key, value) + m_newline;
      return;
    }

    values.emplace_back();
    Value & v = values.back();
    v.key = key;
    v.value = value;

    Line line;
    line.raw = escapeKeyAndValue(key, value) + m_newline;
    v.line = m_lines.insert(section.lines.back().end, line);
  }

  void IniFile::D::parseLine(const std::list<Line>::iterator & line, const QString & sourceName, Section *& currentSection)
  {
    QString txt = QString::fromUtf8(line->raw);
    QRegularExpressionMatch m = m_sectionRegex.match(txt);
    if (m.hasMatch()) {
      QString name = unescapeKey(m.captured(1));
      if (currentSection)
        currentSection->lines.back().end = line;

      currentSection = findSection(name);
      if (!currentSection) {
        m_sections.emplace_back();
        currentSection = &m_sections.back();
        currentSection->name = name;
        currentSection->nameLower = name.toLower().toUtf8();
      }
      currentSection->lines.emplace_back();
      currentSection->lines.back().begin = line;
      return;
    }

    m = m_valueRegex.match(txt);
    if (m.hasMatch()) {
      QString key = unescapeKey(m.captured(2));
      QString value = unescapeValue(m.captured(3).trimmed());

      if (!currentSection)
        currentSection = &createGlobalSection();

      QByteArray keyLower = key.toLower().toUtf8();
      auto it = currentSection->values.find(keyLower);
      if (it == currentSection->values.end())
        it = currentSection->values.insert(std::pair(keyLower, std::vector<Value>())).first;

      std::vector<Value> & values = it->second;

      bool commented = m.captured(1) == ";";
      if (!commented) {
        for (const Value & v: values) {
          if (v.commented)
            continue;

          Radiant::warning("Ini parser: %s:%d overrides value '%s' already defined on line %d",
                           sourceName.toUtf8().data(), line->origLineNumber,
                           key.toUtf8().data(), v.line->origLineNumber);
          break;
        }
      }

      values.emplace_back();
      Value & v = values.back();
      v.commented = commented;
      v.key = key;
      v.value = value;
      v.line = line;
      return;
    }
  }

  IniFile::IniFile()
    : m_d(new D())
  {
  }

  IniFile::~IniFile()
  {}

  bool IniFile::parseFile(const QString & filename)
  {
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
      Radiant::error("IniFile # Failed to open %s for reading: %s", filename.toUtf8().data(),
                     file.errorString().toUtf8().data());
      return false;
    }

    return parseData(file.readAll(), filename);
  }

  bool IniFile::parseData(const QByteArray & data, const QString & sourceName)
  {
    bool ok = true;
    m_d->clear();

    Section * currentSection = nullptr;

    int lineStartOffset = 0;
    int lineNumber = 0;
    for (;;) {
      int idx = data.indexOf('\n', lineStartOffset);
      QByteArray raw = data.mid(lineStartOffset, idx < 0 ? -1 : idx - lineStartOffset + 1);
      if (!raw.isEmpty()) {
        if (m_d->m_lines.empty()) {
          if (raw.endsWith("\r\n"))
            m_d->m_newline = "\r\n";
          else
            m_d->m_newline = "\n";
        }

        m_d->m_lines.emplace_back();
        m_d->m_lines.back().origLineNumber = ++lineNumber;
        m_d->m_lines.back().raw = std::move(raw);
        std::list<Line>::iterator line = --m_d->m_lines.end();

        try {
          m_d->parseLine(line, sourceName, currentSection);
        } catch (const std::exception & error) {
          Radiant::error("IniFile # %s:%d: %s", sourceName.toUtf8().data(), lineNumber,
                         error.what());
          ok = false;
        }
      }
      if (idx < 0)
        break;
      lineStartOffset = idx + 1;
    }
    if (!m_d->m_lines.empty() && !m_d->m_lines.back().raw.isEmpty() &&
        !m_d->m_lines.back().raw.endsWith("\n"))
      m_d->m_lines.back().raw += m_d->m_newline;

    if (currentSection)
      currentSection->lines.back().end = m_d->m_lines.end();

    return ok;
  }

  bool IniFile::writeToFile(const QString & filename)
  {
    QSaveFile file(filename);
    if (!file.open(QSaveFile::WriteOnly)) {
      Radiant::error("IniFile # Failed to open %s for writing: %s", filename.toUtf8().data(),
                     file.errorString().toUtf8().data());
      return false;
    }

    file.write(writeData());
    if (!file.commit()) {
      Radiant::error("IniFile # Failed to save to %s: %s", filename.toUtf8().data(),
                     file.errorString().toUtf8().data());
      return false;
    }
    return true;
  }

  QByteArray IniFile::writeData() const
  {
    QByteArray data;
    for (const Line & line: m_d->m_lines)
      data += line.raw;
    if (data.isEmpty())
      data += m_d->m_newline;
    return data;
  }

  QStringList IniFile::sections(const QString & prefix) const
  {
    const QByteArray & prefixLower = prefix.toLower().toUtf8();

    QStringList ret;
    for (const Section & s: m_d->m_sections)
      if (s.nameLower.startsWith(prefixLower))
        ret << s.name;
    return ret;
  }

  QStringList IniFile::keys(const QString & sectionName) const
  {
    QStringList keyList;

    Section * section = m_d->findSection(sectionName);
    if (!section)
      return keyList;

    for (const auto & p: section->values) {
      const std::vector<Value> & values = p.second;
      for (auto it = values.rbegin(), end = values.rend(); it != end; ++it) {
        const Value & v = *it;
        if (!v.commented) {
          keyList << QString("%1/%2").arg(section->name, v.key);
          break;
        }
      }
    }

    return keyList;
  }

  QVariant IniFile::value(const QString & fullKey) const
  {
    auto [sectionName, key] = splitKey(fullKey);
    Section * section = m_d->findSection(sectionName);
    if (!section)
      return QVariant();

    auto it = section->values.find(key.toLower().toUtf8());
    if (it == section->values.end())
      return QVariant();

    for (auto it2 = it->second.rbegin(), end2 = it->second.rend(); it2 != end2; ++it2) {
      const Value & v = *it2;
      if (!v.commented)
        return v.value;
    }

    return QVariant();
  }

  QVariant IniFile::value(const QString & fullKey, const QVariant & defaultValue) const
  {
    QVariant maybe = value(fullKey);
    return maybe.isValid() ? maybe : defaultValue;
  }

  void IniFile::setValue(const QString & fullKey, const QVariant & value)
  {
    auto [sectionName, key] = splitKey(fullKey);
    Section & section = m_d->createSection(sectionName);
    m_d->setValue(section, key, value.toString());
  }

  void IniFile::clearValue(const QString & fullKey)
  {
    auto [sectionName, key] = splitKey(fullKey);
    Section * section = m_d->findSection(sectionName);
    if (!section)
      return;

    m_d->clearValue(*section, key);
  }
} // namespace Radiant

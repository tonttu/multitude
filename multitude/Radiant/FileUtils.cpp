/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "FileUtils.hpp"
#include "Mutex.hpp"
#include "Platform.hpp"
#include "PlatformUtils.hpp"
#include "StringUtils.hpp"
#include "Directory.hpp"
#include "Radiant.hpp"
#include "Trace.hpp"

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <QString>

#include <sys/stat.h>

#include <QFileInfo>
#include <QDir>
#include <QDateTime>

#ifdef RADIANT_WINDOWS
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
// The POSIX name for this item is deprecated..
#pragma warning(disable: 4996)
#else
#include <unistd.h>
#endif // PLATFORM_WINDOWS

namespace {
  Radiant::Mutex s_fileWriterMutex;
  //std::function<void ()> s_fileWriterInit;
  //std::function<void ()> s_fileWriterDeinit;
  void (*s_fileWriterInit)() = 0;
  void (*s_fileWriterDeinit)() = 0;
  volatile int s_fileWriterCount = 0;
}

namespace Radiant
{
  FileWriter::FileWriter()
  {
    if(!s_fileWriterInit) return;
    Guard g(s_fileWriterMutex);
    if(s_fileWriterCount++ == 0)
      s_fileWriterInit();
  }

  FileWriter::~FileWriter()
  {
    if(!s_fileWriterDeinit) return;
    Guard g(s_fileWriterMutex);
    if(--s_fileWriterCount == 0)
      s_fileWriterDeinit();
  }

  void FileWriter::setInitFunction(void (*f)())
  {
    s_fileWriterInit = f;
  }

  void FileWriter::setDeinitFunction(void (*f)())
  {
    s_fileWriterDeinit = f;
  }

  /////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////

  using namespace StringUtils;

  unsigned long FileUtils::getFileLen(std::ifstream& file)
  {
    if(!file.good()) return 0;

    unsigned long pos = file.tellg();
    file.seekg(0, std::ios::end);
    unsigned long len = file.tellg();
    file.seekg(pos, std::ios::beg);

    return len;
  }

  unsigned long FileUtils::getFileLen(const QString & filename)
  {
    std::ifstream file(filename.toUtf8().data());

    return getFileLen(file);
  }

  bool FileUtils::fileReadable(const char* filename)
  {
    return PlatformUtils::fileReadable(filename);
  }

  bool FileUtils::fileReadable(const QString & filename)
  {
    return PlatformUtils::fileReadable(filename.toUtf8().data());
  }

  bool FileUtils::fileAppendable(const QString & filename)
  {
    if(!fileReadable(filename))
      return false;

    FILE * f = fopen(filename.toUtf8().data(), "r+");
    if(!f)
      return false;
    fclose(f);
    return true;
  }

  bool FileUtils::renameFile(const char * from, const char * to)
  {
    FileWriter writer;
    int ok = rename(from, to);
    return (ok == 0);
  }

  bool FileUtils::removeFile(const char * filename)
  {
    FileWriter writer;
    return remove(filename) == 0;
  }

  QByteArray FileUtils::loadTextFile(const QString & filename)
  {
    QFile file(filename);
    if(file.open(QIODevice::ReadOnly))
      return file.readAll();
    return QByteArray(); // null bytearray
  }

  bool FileUtils::writeTextFile(const char * filename, const char * contents)
  {
    FileWriter writer;
    uint32_t len = uint32_t(strlen(contents));

    QString tmpname = QString(filename)+".cornerstone_tmp";
    QFile tmp(tmpname), file(filename);
    if(tmp.open(QIODevice::WriteOnly)) {
      if(tmp.write(contents, len) != len) {
        /// @todo does this check actually fail on windows, when doing some line change conversions?
        //return false;
      }
      tmp.close();
      if(QFile::exists(filename))
        QFile::remove(filename);
      return QFile::rename(tmpname, filename);
    } else {
      Radiant::warning("FileUtils::writeTextFile # Failed to write to %s: %s",
                       tmpname.toUtf8().data(), tmp.errorString().toUtf8().data());
      if(file.open(QIODevice::WriteOnly)) {
        if(file.write(contents, len) != len) {
          /// @todo does this check actually fail on windows, when doing some line change conversions?
          //Radiant::error("FileUtils::writeTextFile # Failed to write to %s", filename);
          //return false;
        }
        return true;
      } else {
        Radiant::error("FileUtils::writeTextFile # Failed to write to %s: %s",
                       filename, file.errorString().toUtf8().data());
        return false;
      }
    }
  }

  QString FileUtils::path(const QString & filepath)
  {
    QFileInfo fi(filepath);
    return fi.path();
  }

  QString FileUtils::filename(const QString & filepath)
  {
    QFileInfo fi(filepath);
    return fi.fileName();
  }

  QString FileUtils::baseFilename(const QString & filepath)
  {
    QFileInfo fi(filepath);
    return fi.baseName();
  }


  QString FileUtils::baseFilenameWithPath(const QString & filepath)
  {
    return path(filepath) + '/' + baseFilename(filepath);
  }

  QString FileUtils::suffix(const QString & filepath)
  {
    QFileInfo fi(filepath);
    return fi.suffix();
  }

  QString FileUtils::suffixLowerCase(const QString & filepath)
  {
    return suffix(filepath).toLower();
  }

  bool FileUtils::suffixMatch(const QString & filename,
                              const QString & suf)
  {
    return suffixLowerCase(filename) == suf.toLower();
  }

  QString FileUtils::findFile(const QString & filename, const QString & paths)
  {
    QStringList list = paths.split(";", QString::SkipEmptyParts);
    /*needed to avoid bug(?) in foreach when there are duplicate values in list*/
    list.removeDuplicates();
    foreach(QString str, list) {
      QString fullPath = str + "/" + filename;
      if(fileReadable(fullPath))
        return fullPath;
    }

    return "";
  }

  QString FileUtils::findOverWritable(const QString & filename, const QString & paths)
  {
    foreach(QString str, paths.split(pathSeparator(), QString::SkipEmptyParts)) {
      QString fullPath = str + "/" + filename;

      if(fileAppendable(fullPath))
        return fullPath;
    }
    return filename;
  }

  FILE * FileUtils::createFilePath(const QString & filePath)
  {
    if(filePath.isEmpty()) return 0;

    QFileInfo fi(filePath);
    QDir().mkpath(fi.path());

    /// @todo change to something else than FILE/fopen
    return fopen(filePath.toUtf8().data(), "w");
  }

  bool FileUtils::looksLikeImage(const QString & filePath)
  {
    return suffixMatch(filePath, "png") ||
        suffixMatch(filePath, "jpg") ||
        suffixMatch(filePath, "jpeg") ||
        suffixMatch(filePath, "dds");
  }

  bool FileUtils::looksLikeVideo(const QString & filePath)
  {
    return suffixMatch(filePath, "avi") ||
        suffixMatch(filePath, "qt") ||
        suffixMatch(filePath, "mov") ||
        suffixMatch(filePath, "mp4");
  }

  Radiant::TimeStamp FileUtils::lastModified(const QString & filePath)
  {
    QFileInfo fi(filePath);

    if(!fi.exists()) {
      Radiant::error("FileUtils::lastModified # file (%s) does not exist", filePath.toUtf8().data());
      return Radiant::TimeStamp(0);
    }

    QDateTime newer = std::max(fi.created(), fi.lastModified());

    return TimeStamp(newer.toTime_t());
  }

  void FileUtils::indent(FILE * f, int levels)
  {
    assert(f != 0);
    for(int i = 0; i < levels; i++) {
      fprintf(f, ". ");
    }
  }

  QString FileUtils::pathSeparator()
  {
    return QString(";");
  }

	QString FileUtils::directorySeparator()
	{
#if defined RADIANT_WINDOWS
		return QString("\\");
#else
		return QString("/");
#endif
	}
}

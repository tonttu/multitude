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
#include "PlatformUtils.hpp"
#include "StringUtils.hpp"
#include "Directory.hpp"

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <QString>

#include <sys/stat.h>

#include <QFileInfo>
#include <QDir>

#ifdef WIN32
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
// The POSIX name for this item is deprecated..
#pragma warning(disable: 4996)
#endif


using namespace std;

namespace Radiant
{

  using namespace StringUtils;

  unsigned long FileUtils::getFileLen(ifstream& file)
  {
    if(!file.good()) return 0;

    unsigned long pos = file.tellg();
    file.seekg(0, ios::end);
    unsigned long len = file.tellg();
    file.seekg(pos, ios::beg);

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
    int ok = rename(from, to);
    return (ok == 0);
  }

  bool FileUtils::removeFile(const char * filename)
  {
    return remove(filename) == 0;
  }

  char* FileUtils::loadTextFile(const char* filename)
  {
    ifstream file;

    file.open(filename, ios::in | ios::binary);
    if(!file.good()) {
      cerr << "loadTextFile # could not open '" << filename <<
          "' for reading" << endl;
      return 0;
    }

    unsigned long len = getFileLen(file);

    if(len == 0) {
      cerr << "loadTextFile # file '" << filename << "' is empty" << endl;
      return 0;
    }

    char* contents = new char [len + 1];
    file.read(contents, len);
    contents[len] = 0;

    file.close();

    return contents;
  }

  QString FileUtils::readTextFile(const QString & filename)
  {
    QFile file(filename);
    if(file.open(QIODevice::ReadOnly))
      return file.readAll();
    return "";
  }

  bool FileUtils::writeTextFile(const char * filename, const char * contents)
  {
#ifdef WIN32
    int fd = _creat(filename, _S_IWRITE);
#else
    int fd = creat(filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
    if(fd <= 0)
      return false;

    uint32_t len = uint32_t(strlen(contents));

    bool ok = write(fd, contents, len) == len;

    close(fd);

    return ok;
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
    foreach(QString str, paths.split(";")) {
      QString fullPath = str + "/" + filename;

      if(fileReadable(fullPath))
        return fullPath;
    }

    return QString();
  }

  QString FileUtils::findOverWritable(const QString & filename, const QString & paths)
  {
    foreach(QString str, paths.split(";")) {
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
        suffixMatch(filePath, "jpeg");
  }

  bool FileUtils::looksLikeVideo(const QString & filePath)
  {
    return suffixMatch(filePath, "avi") ||
        suffixMatch(filePath, "qt") ||
        suffixMatch(filePath, "mov");
  }

  unsigned long int FileUtils::lastModified(const QString & filePath)
  {
    struct stat file;
    if(stat(filePath.toUtf8().data(), &file) == -1) {
      return 0;
    }
    return file.st_mtime;
  }

  void FileUtils::indent(FILE * f, int levels)
  {
    assert(f != 0);
    for(int i = 0; i < levels; i++) {
      fprintf(f, ". ");
    }
  }


}

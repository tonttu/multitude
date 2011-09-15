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
#include "Platform.hpp"
#include "PlatformUtils.hpp"
#include "StringUtils.hpp"
#include "Directory.hpp"
#include "Radiant.hpp"

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string.h>

#include <sys/stat.h>

#include <QFileInfo>

#ifdef RADIANT_WINDOWS
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
// The POSIX name for this item is deprecated..
#pragma warning(disable: 4996)
#endif // PLATFORM_WINDOWS

namespace Radiant
{

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

  unsigned long FileUtils::getFileLen(const std::string & filename)
  {
    std::ifstream file(filename.c_str());

    return getFileLen(file);
  }

  bool FileUtils::fileReadable(const char* filename)
  {
    return PlatformUtils::fileReadable(filename);
  }

  bool FileUtils::fileReadable(const std::string & filename)
  {
    return PlatformUtils::fileReadable(filename.c_str());
  }

  bool FileUtils::fileAppendable(const char* filename)
  {
    if(!fileReadable(filename))
      return false;

    FILE * f = fopen(filename, "r+");
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
    std::ifstream file;

    file.open(filename, std::ios::in | std::ios::binary);
    if(!file.good()) {
      error("loadTextFile # could not open '%s' for reading", filename);
      return 0;
    }

    unsigned long len = getFileLen(file);

    if(len == 0) {
      error("loadTextFile # file '%s' is empty", filename);
      return 0;
    }

    char* contents = new char [len + 1];
    file.read(contents, len);
    contents[len] = 0;

    file.close();

    return contents;
  }

  std::wstring FileUtils::readTextFile(const std::string & filename)
  {
    std::wstring res;

    std::ifstream file(filename.c_str());

    if(file.is_open()) {

      std::string line;

      while(getline(file, line))
        res += StringUtils::utf8AsStdWstring(line) + wchar_t(0x200B); // W_NEWLINE

      file.close();
    }

    return res;
  }

  bool FileUtils::writeTextFile(const char * filename, const char * contents)
  {
#ifdef RADIANT_WINDOWS
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

  std::string FileUtils::path(const std::string & filepath)
  {
    size_t cut = filepath.rfind(directorySeparator()) + 1;
    return filepath.substr(0, cut);
  }

  std::string FileUtils::filename(const std::string & filepath)
  {
    size_t cut = filepath.rfind(directorySeparator()) + 1;
    return filepath.substr(cut);
  }

  std::string FileUtils::baseFilename(const std::string & filepath)
  {
    size_t cut1 = filepath.rfind(directorySeparator()) + 1;
    size_t cut2 = filepath.rfind(".");

    // info("baseFilename %s %d %d", filepath.c_str(), cut1, cut2);
    return (cut1 > 0) ?
        filepath.substr(cut1, cut2 - cut1) : filepath.substr(0, cut2);
  }

  std::string FileUtils::baseFilenameWithPath(const std::string & filepath)
  {
    size_t cut2 = filepath.rfind(".");
    return filepath.substr(0, cut2);
  }

  std::string FileUtils::withoutSuffix(const std::string & filepath)
  {
    size_t cut = filepath.rfind(".");
    if(cut > 0)
      return filepath.substr(0, cut);

    return filepath;
  }


  std::string FileUtils::suffix(const std::string & filepath)
  {
    QFileInfo fi(filepath.c_str());

    return fi.suffix().toStdString();
  }

  std::string FileUtils::suffixLowerCase(const std::string & filepath)
  {
    size_t cut = filepath.rfind(".") + 1;
    return StringUtils::lowerCase(filepath.substr(cut));
  }

  bool FileUtils::suffixMatch(const std::string & filename,
                              const std::string & suf)
  {
    std::string s = suffix(filename);
    return StringUtils::lowerCase(s) == StringUtils::lowerCase(suf);
  }

  std::string FileUtils::findFile(const std::string & filename, const std::string & paths)
  {
    StringList pathList;
    split(paths, pathSeparator().c_str(), pathList, true);

    for(StringList::iterator it = pathList.begin();
    it != pathList.end(); it++) {
      std::string fullPath = (*it) + directorySeparator() + filename;

      debugRadiant("Radiant::findFile # Testing %s for %s", (*it).c_str(), filename.c_str());

      if(fileReadable(fullPath.c_str())) {
        debugRadiant("Radiant::findFile # FOUND %s", fullPath.c_str());
        return fullPath;
      }
    }

    return std::string();
  }

  std::string FileUtils::findOverWritable(const std::string & filename, const std::string & paths)
  {
    StringList pathList;
    split(paths, pathSeparator().c_str(), pathList, true);

    for(StringList::iterator it = pathList.begin();
    it != pathList.end(); it++) {
      std::string fullPath = (*it) + directorySeparator() + filename;

      if(fileAppendable(fullPath.c_str()))
        return fullPath;
    }

    return filename;
  }

  FILE * FileUtils::createFilePath(const std::string & filePath)
  {
    if(filePath.empty()) return 0;

    StringList pieces;
    split(filePath, directorySeparator().c_str(), pieces, true);

    const std::string file(pieces.back());
    pieces.pop_back();

    std::string soFar("");

    for(StringList::iterator it = pieces.begin(); it != pieces.end(); it++) {
      soFar += directorySeparator() + *it;

      if(!Directory::exists(soFar)) {
        Directory::mkdir(soFar);
      }
    }

    soFar += directorySeparator() + file;

    return fopen(soFar.c_str(), "w");
  }

  bool FileUtils::looksLikeImage(const std::string & filePath)
  {
    return suffixMatch(filePath, "png") ||
        suffixMatch(filePath, "jpg") ||
        suffixMatch(filePath, "jpeg") ||
        suffixMatch(filePath, "dds");
  }

  bool FileUtils::looksLikeVideo(const std::string & filePath)
  {
    return suffixMatch(filePath, "avi") ||
        suffixMatch(filePath, "qt") ||
        suffixMatch(filePath, "mov") ||
        suffixMatch(filePath, "mp4");
  }

  unsigned long int FileUtils::lastModified(const std::string & filePath)
  {
    struct stat file;
    if(stat(filePath.c_str(), &file) == -1) {
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

	std::string FileUtils::pathSeparator()
	{
#if defined RADIANT_WINDOWS
		return std::string(";");
#else
		return std::string(":");
#endif
	}

	std::string FileUtils::directorySeparator()
	{
#if defined RADIANT_WINDOWS
		return std::string("\\");
#else
		return std::string("/");
#endif
	}
}

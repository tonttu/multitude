/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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
#include "Timer.hpp"
#include "ProcessRunner.hpp"

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <QString>

#include <sys/stat.h>

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QThread>
#include <QTemporaryFile>

#ifdef RADIANT_LINUX
#include <errno.h>
#include <sys/file.h>
#endif

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
  bool s_fileWriterMountedRW = false;
  bool s_fileWriterEnabled = false;
  int s_fileWriterCount = 0;
  std::function<void (Radiant::FileWriter::FileWriterMode mode)> s_callback = nullptr;

#if defined(RADIANT_LINUX) && !defined(RADIANT_MOBILE)

  bool do_flock(QFile & file, int flags = LOCK_EX) {
    while(flock(file.handle(), flags) == -1) {
      if(errno != EINTR) return false;
    }
    return true;
  }

  bool try_flock(QFile & file, bool exclusive = true) {
    while(flock(file.handle(), (exclusive ? LOCK_EX : LOCK_SH) | LOCK_NB) == -1) {
      if(errno == EWOULDBLOCK) return false;
      if(errno != EINTR) return false;
    }
    return true;
  }

  QFile s_globalLockfile;
  QFile s_usersLockfile;

  bool s_initLocks()
  {
    if(!s_globalLockfile.isOpen()) {
      s_globalLockfile.setFileName("/tmp/mount-rw-lock");
      if(!s_globalLockfile.open(QFile::WriteOnly)) {
        Radiant::error("Failed to open /tmp/mount-rw-lock: %s", s_globalLockfile.errorString().toUtf8().data());
        return false;
      }
    }

    if(!s_usersLockfile.isOpen()) {
      s_usersLockfile.setFileName("/tmp/mount-rw-users");
      if(!s_usersLockfile.open(QFile::WriteOnly)) {
        Radiant::error("Failed to open /tmp/mount-rw-users: %s", s_usersLockfile.errorString().toUtf8().data());
        return false;
      }
    }

    return true;
  }


  void s_mountRW(const QString & name)
  {
    if(!s_initLocks()) return;

    if(!do_flock(s_globalLockfile)) {
      Radiant::error("Failed to acquire the global lock: %s", strerror(errno));
      s_globalLockfile.close();
      s_usersLockfile.close();
      return;
    }

    if(try_flock(s_usersLockfile)) {
      Radiant::info("Remounting root filesystem to read-write -mode (reason: %s)", name.toUtf8().data());
      Radiant::FileUtils::runAsRoot("mount", QStringList() << "-o" << "remount,rw" << "/");

      if(!do_flock(s_usersLockfile, LOCK_UN)) {
        Radiant::error("Failed to release the users lock: %s", strerror(errno));
        s_globalLockfile.close();
        s_usersLockfile.close();
        return;
      }
    }

    if(!do_flock(s_usersLockfile, LOCK_SH)) {
      Radiant::error("Failed to increase the use count");
      s_globalLockfile.close();
      s_usersLockfile.close();
      return;
    }

    if(!do_flock(s_globalLockfile, LOCK_UN)) {
      Radiant::error("Failed to release the global lock");
      s_globalLockfile.close();
      s_usersLockfile.close();
      return;
    }
  }

  void s_mountRO()
  {
    if(!s_initLocks()) return;

    if(!do_flock(s_globalLockfile)) {
      Radiant::error("Failed to acquire the global lock: %s", strerror(errno));
      s_globalLockfile.close();
      s_usersLockfile.close();
      return;
    }

    if(!do_flock(s_usersLockfile, LOCK_UN)) {
      Radiant::error("Failed to decrease the use count");
      s_globalLockfile.close();
      s_usersLockfile.close();
      return;
    }

    if(try_flock(s_usersLockfile)) {
      Radiant::info("Remounting root filesystem to read-only -mode");
      /// @todo should this happen asynchronously?
      Radiant::FileUtils::run("sync");
      Radiant::FileUtils::runAsRoot("mount", QStringList() << "-o" << "remount,ro" << "/");

      if(!do_flock(s_usersLockfile, LOCK_UN)) {
        Radiant::error("Failed to release the users lock: %s", strerror(errno));
        s_globalLockfile.close();
        s_usersLockfile.close();
        return;
      }
    }

    if(!do_flock(s_globalLockfile, LOCK_UN)) {
      Radiant::error("Failed to release the global lock");
      s_globalLockfile.close();
      s_usersLockfile.close();
      return;
    }
  }

  void fileWriterInit()
  {
    s_fileWriterEnabled = Radiant::FileWriter::wantRootFileSystemReadOnly();
    if(s_fileWriterEnabled)
      Radiant::info("Root filesystem is mounted in read-only mode, using rw-remounting when necessary.");
  }
#endif
}

namespace Radiant
{
  FileWriter::FileWriter(const QString & name)
  {
    (void)name;
#if defined(RADIANT_LINUX) && !defined(RADIANT_MOBILE)
    MULTI_ONCE{ fileWriterInit(); }
#endif
    if (!s_fileWriterEnabled) return;
    Guard g(s_fileWriterMutex);
    s_fileWriterCount++;
    if (!s_fileWriterMountedRW) {
#if defined(RADIANT_LINUX) && !defined(RADIANT_MOBILE)
      s_mountRW(name);
#endif
      s_fileWriterMountedRW = true;
      if(s_callback)
        s_callback(READ_WRITE);
    }
  }

  FileWriter::~FileWriter()
  {
    if (!s_fileWriterEnabled) return;
    Guard g(s_fileWriterMutex);
    if (--s_fileWriterCount == 0 && s_fileWriterMountedRW) {
#if defined(RADIANT_LINUX) && !defined(RADIANT_MOBILE)
      s_mountRO();
#endif
      s_fileWriterMountedRW = false;
      if(s_callback)
        s_callback(READ_ONLY);
    }
  }

  bool FileWriter::wantRootFileSystemReadOnly()
  {
#ifdef RADIANT_LINUX
    /// We are looking at /etc/fstab instead of /proc/mounts, because we want
    /// to know if we prefer to have the root filesystem in ro-state, instead
    /// of looking at the current state, that could be temporarily different.
    QFile file("/etc/fstab");
    if(file.open(QFile::ReadOnly)) {
      // UUID=4d518a9b-9ea8-4f15-8e75-e4fb4f7e4af9	/	ext4	noatime,errors=remount-ro,ro	0	0
      // /dev/disk/by-uuid/4d518a9b-9ea8-4f15-8e75-e4fb4f7e4af9 / ext4 rw,noatime,errors=remount-ro,user_xattr,barrier=1,data=ordered 0 0
      QRegExp re("(?:^|\\n)[^\\s]+\\s+/\\s+[^\\s]+\\s+([^\\s]+)\\s+\\d+\\s+\\d+(?:\\n|$)");
      if(re.indexIn(QString::fromUtf8(file.readAll())) >= 0) {
        QStringList mountOptions = re.cap(1).split(",");
        return mountOptions.contains("ro");
      }
    }
#endif
    return false;
  }

  void FileWriter::setCallback(std::function<void (FileWriter::FileWriterMode)> callback)
  {
    s_callback = callback;
  }

  FileWriterMerger::FileWriterMerger()
  {
    Guard g(s_fileWriterMutex);
    s_fileWriterCount++;
  }

  FileWriterMerger::~FileWriterMerger()
  {
    Guard g(s_fileWriterMutex);
    if (--s_fileWriterCount == 0 && s_fileWriterMountedRW) {
#if defined(RADIANT_LINUX) && !defined(RADIANT_MOBILE)
      s_mountRO();
#endif
      s_fileWriterMountedRW = false;
      if(s_callback)
        s_callback(FileWriter::READ_ONLY);
    }
  }

  /////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////

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

  bool FileUtils::isWritable(const QString &path)
  {
    return QFileInfo(path).isWritable();
  }

  bool FileUtils::fileReadable(const QString & filename)
  {
    QFileInfo fi(filename);

    return fi.exists() && fi.isReadable();
  }

  bool FileUtils::fileAppendable(const QString & filename)
  {
    if(!fileReadable(filename))
      return false;

    QFile file(filename);
    return file.open(QIODevice::ReadOnly | QIODevice::Append);
  }

  bool FileUtils::renameFile(const char * from, const char * to)
  {
    FileWriter writer("FileUtils::renameFile");
    int ok = rename(from, to);
    return (ok == 0);
  }

  bool FileUtils::removeFile(const char * filename)
  {
    FileWriter writer("FileUtils::removeFile");
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
    FileWriter writer("FileUtils::writeTextFile");
    uint32_t len = uint32_t(strlen(contents));

    QString tmpname = QString(filename)+".cornerstone_tmp";
    QFile tmp(tmpname), file(filename);
    if(tmp.open(QIODevice::WriteOnly)) {
      if(tmp.write(contents, len) != len) {
        /// @todo see #4259
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
          /// @todo see #4259
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
    QStringList list = paths.split(pathSeparator(), QString::SkipEmptyParts);
    /*needed to avoid bug(?) in Q_FOREACH when there are duplicate values in list*/
    list.removeDuplicates();
    Q_FOREACH(QString str, list) {
      QString fullPath = str + "/" + filename;
      if(fileReadable(fullPath))
        return fullPath;
    }

    return "";
  }

  QString FileUtils::findOverWritable(const QString & filename, const QString & paths)
  {
    Q_FOREACH(QString str, paths.split(pathSeparator(), QString::SkipEmptyParts)) {
      QString fullPath = str + "/" + filename;

      if(fileAppendable(fullPath))
        return fullPath;
    }
    return filename;
  }

  QString FileUtils::resolvePath(const QString & source)
  {
    int idx = source.indexOf(':');
    if (idx < 0) {
      return QFileInfo(source).absoluteFilePath();
    }

    const QString prefix = source.left(idx);
    const QString name = source.mid(idx + 1);

    auto lst = QDir::searchPaths(prefix);
    if (lst.isEmpty()) {
      return QFileInfo(source).absoluteFilePath();
    }

    return lst[0] + "/" + name;
  }

  QString FileUtils::makeFilenameUnique(const QString &filename)
  {
    QFileInfo info(filename);
    const QString origFilename = info.fileName();

    QString suffix = info.suffix();
    if (origFilename.endsWith(".tar.gz"))
      suffix = "tar.gz";
    else if (origFilename.endsWith(".mt-canvus-canvas.zip"))
      suffix = "mt-canvus-canvas.zip";

    QString cleanFilename;
    if (suffix.isEmpty())
      cleanFilename = origFilename.simplified();
    else
      cleanFilename = origFilename.left(origFilename.size() - suffix.size() - 1).simplified();

    QString path = info.path();

    // vfat forbidden characters (see vfat_bad_char in fs/fat/namei_vfat.c)
    QRegExp r("[\\x0001-\\x0019*?<>|\":/\\\\]+");
    cleanFilename.replace(r, "-");
    // It's a bit unclear what is the file size limit (in some cases 481?),
    // but 256 seems like a good practical limit.
    cleanFilename = cleanFilename.left(256);

    const QString dotSuffix = suffix.isEmpty() ? "" : QString(".%1").arg(suffix);

    QString file = path + "/" + cleanFilename + dotSuffix;
    for (int i = 1; QFile::exists(file); ++i) {
      file = QString("%2/%3 (%1)%4").arg(i).arg(path, cleanFilename, dotSuffix);
    }
    return file;
  }

  FILE * FileUtils::createFilePath(const QString & filePath)
  {
    if(filePath.isEmpty()) return nullptr;

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
        suffixMatch(filePath, "bmp") ||
        suffixMatch(filePath, "svg") ||
        suffixMatch(filePath, "tiff") ||
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

    return TimeStamp(newer);
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
#if defined RADIANT_WINDOWS
    return QString(";");
#else
    return QString(":");
#endif
  }

	QString FileUtils::directorySeparator()
	{
#if defined RADIANT_WINDOWS
		return QString("\\");
#else
		return QString("/");
#endif
  }

#if defined(RADIANT_LINUX) && !defined(RADIANT_MOBILE)

  int FileUtils::runInShell(QString cmd, QByteArray * out, QByteArray * err, bool quiet)
  {
    return run("/bin/sh", QStringList() << "-c" << cmd, out, err, quiet);
  }

  int FileUtils::run(QString cmd, QStringList argv, QByteArray * out, QByteArray * err, bool quiet)
  {
    QByteArray outStdout, outStderr;
    ProcessIO io = ProcessIO(OutputRedirect(&outStdout), OutputRedirect(&outStderr));

    auto runner = newProcessRunner();
    assert(runner);

    ProcessRunner::Result result = runner->run(cmd, argv, 300.0, io);

    if(out) *out = outStdout;
    if(err) *err = outStderr;

    if(!outStderr.isEmpty() && !quiet) {
      Radiant::error("%s: %s", cmd.toUtf8().data(), outStderr.data());
    }

    return result.exitCode;
  }

  int FileUtils::runAsRoot(QString cmd, QStringList argv, QByteArray * out, QByteArray * err, bool quiet)
  {
    if(geteuid() == 0) {
      return run(cmd, argv, out, err, quiet);
    } else {
      return run("sudo", (QStringList() << "-n" << "--" << cmd) + argv, out, err, quiet);
    }
    return 0;
  }

  void FileUtils::writeAsRoot(const QString & filename, const QByteArray & data, bool quiet)
  {
    Radiant::FileWriter writer("FileUtils::writeAsRoot");

    QTemporaryFile file(QDir::tempPath() + "/taction.tmpfile");
    if(file.open()) {
      file.write(data);
      file.close();
      runAsRoot("mv", QStringList() << file.fileName() << filename, 0, 0, quiet);
      runAsRoot("chown", QStringList() << "root:root" << filename, 0, 0, quiet);
      runAsRoot("chmod", QStringList() << "0644" << filename, 0, 0, quiet);

      file.setAutoRemove(false);
    } else {
      if (!quiet)
        Radiant::error("Failed to write %s", filename.toUtf8().data());
    }
  }
#endif
}

/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef VALUABLE_FILEWATCHER_HPP
#define VALUABLE_FILEWATCHER_HPP

#include "Node.hpp"

#include <QString>
#include <QStringList>

namespace Valuable
{

  /// FileWatcher provides an interface for monitoring files and directories
  /// for modifications.
  class VALUABLE_API FileWatcher : public Node
  {
  public:
    FileWatcher();
    ~FileWatcher();

    bool addPath(const QString & relativePath);
    bool addPaths(const QStringList & paths);

    QStringList directories() const;
    QStringList files() const;

    void removePath(const QString & path);
    void removePaths(const QStringList & paths);
    void clear();

  private:
    class D;
    D * m_d;
  };
}

#endif // VALUABLE_FILEWATCHER_HPP

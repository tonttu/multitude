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

#include <Radiant/TimeStamp.hpp>

#include <QString>
#include <QMap>
#include <QSet>

namespace Valuable
{

  class VALUABLE_API FileWatcher : public Node
  {
  public:
    void add(QString filename);
    void update();

    static FileWatcher & instance();

  private:
    /// @todo do not use polling, use QFileWatcher instead
    QMap<QString, Radiant::TimeStamp> m_files;
    QSet<QString> m_queue;
    FileWatcher();
  };
}

#endif // VALUABLE_FILEWATCHER_HPP

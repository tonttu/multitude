/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "AttributeAsset.hpp"
#include "FileWatcher.hpp"

#include <QFileInfo>

namespace Valuable
{

  AttributeAsset::AttributeAsset()
  {}

  AttributeAsset::AttributeAsset(Node *host, const QByteArray &name,
                                 const QString &filePath)
    : AttributeString(host, name, filePath)
  {}

  AttributeAsset::~AttributeAsset()
  {
  }

  bool AttributeAsset::operator==(const QString& that) const
  {
    return QFileInfo(that).absoluteFilePath() == QFileInfo(*this).absoluteFilePath();
  }

  bool AttributeAsset::operator!=(const QString& that) const
  {
    return !(*this == that);
  }

}

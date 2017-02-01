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

#ifndef VALUABLE_ATTRIBUTEASSET_HPP
#define VALUABLE_ATTRIBUTEASSET_HPP

#include <Valuable/Attribute.hpp>

#include <Valuable/AttributeString.hpp>

namespace Valuable
{

  /// @todo implement & add documentation about related file monitoring
  ///       functionalities

  /// Attribute asset is an attribute that corresponds to the item of
  /// binary data stored on the local filesystem. Typical examples are
  /// videos and images. The actual value of attribute asset is the file
  /// path of the asset.
  ///
  /// At the moment this class does not monitor underlying file.
  /// Notifications about the changes in the pointed file can be
  /// signaled by manually calling Attribute::emitChange.
  class VALUABLE_API AttributeAsset : public AttributeString
  {
  public:
    using AttributeString::operator =;

    AttributeAsset();
    AttributeAsset(Node* host, const QByteArray& name, const QString& filePath = "");
    virtual ~AttributeAsset();

    /// Compares whether the string points to the same asset
    bool operator==(const QString& that) const;
    bool operator!=(const QString& that) const;

    virtual QByteArray type() const { return "asset"; }
  };

}

#endif

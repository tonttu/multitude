#ifndef VALUABLE_ATTRIBUTEASSET_HPP
#define VALUABLE_ATTRIBUTEASSET_HPP

#include <Valuable/Attribute.hpp>

#include <Valuable/AttributeString.hpp>

namespace Valuable
{

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

    /// If set to true this will emit change events when the target file has changed.
    /// Only works for files not directories. Also will create the file watcher
    /// in the thread that calls the function, so need to call this in main thread!
    void setToMonitorFile(bool monitor);
    bool isMonitoringFile() const;

  private:
    class D;
    D* m_d;
  };

}

#endif

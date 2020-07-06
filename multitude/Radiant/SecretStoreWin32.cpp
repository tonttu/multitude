#include "OnDemandExecutor.hpp"
#include "SecretStore.hpp"
#include "StringUtils.hpp"
#include "Trace.hpp"

#include <QSettings>

#include <dpapi.h>

namespace Radiant
{
  class SecretStore::D
  {
  public:
    QString decrypt(QByteArray data);
    QByteArray crypt(const QString & data, const QString & label);

  public:
    std::unique_ptr<Radiant::OnDemandExecutor> m_executor{new Radiant::OnDemandExecutor()};
    QString m_organization;
    QString m_application;
    SecretStore::Flags m_flags;
  };

  QString SecretStore::D::decrypt(QByteArray data)
  {
    DATA_BLOB in;
    DATA_BLOB out;
    DWORD flags = (m_flags & SecretStore::FLAG_ALLOW_UI) ? 0 : CRYPTPROTECT_UI_FORBIDDEN;
    in.pbData = reinterpret_cast<BYTE*>(data.data());
    in.cbData = data.size();
    if (CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, flags, &out)) {
      QString ret = QString::fromUtf8(reinterpret_cast<const char*>(out.pbData), out.cbData);
      LocalFree(out.pbData);
      return ret;
    } else {
      Radiant::error("CryptUnprotectData failed: %s",
                     StringUtils::getLastErrorMessage().toUtf8().data());
      throw std::runtime_error("SecretStore failed to decrypt data");
    }
  }

  QByteArray SecretStore::D::crypt(const QString & description, const QString & data)
  {
    QByteArray str = data.toUtf8();
    DATA_BLOB in;
    DATA_BLOB out;
    DWORD flags = (m_flags & SecretStore::FLAG_ALLOW_UI) ? 0 : CRYPTPROTECT_UI_FORBIDDEN;
    in.pbData = reinterpret_cast<BYTE*>(str.data());
    in.cbData = str.size();
    if (CryptProtectData(&in, description.toStdWString().data(), nullptr, nullptr, nullptr,
                         flags, &out)) {
      QByteArray ret(reinterpret_cast<const char*>(out.pbData), out.cbData);
      LocalFree(out.pbData);
      return ret;
    } else {
      Radiant::error("CryptProtectData failed: %s",
                     StringUtils::getLastErrorMessage().toUtf8().data());
      throw std::runtime_error("SecretStore failed to encrypt data");
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  SecretStore::SecretStore(const QString & organization, const QString & application, Flags flags)
    : m_d(new D())
  {
    m_d->m_organization = organization;
    m_d->m_application = application;
    m_d->m_flags = flags;
  }

  SecretStore::~SecretStore()
  {
    m_d->m_executor.reset();
  }

  folly::Future<QString> SecretStore::secret(const QString & key)
  {
    return folly::via(m_d->m_executor.get(), [this, key] {
      QSettings settings(m_d->m_organization, m_d->m_application);
      int size = settings.beginReadArray("secrets");
      for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        if (settings.value("key").toString() == key) {
          QByteArray data = settings.value("secret").toByteArray();
          return m_d->decrypt(data);
        }
      }
      settings.endArray();
      throw std::runtime_error("Not found");
    });
  }

  folly::Future<folly::Unit> SecretStore::setSecret(
      const QString & label, const QString & key, const QString & secret)
  {
    return folly::via(m_d->m_executor.get(), [this, label, key, secret] {
      QSettings settings(m_d->m_organization, m_d->m_application);
      int size = settings.beginReadArray("secrets");
      for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        // Since QSettings can't delete array items, there can be empty
        // children after clearSecret. Reuse those items here.
        if (settings.childKeys().isEmpty() && settings.childGroups().isEmpty()) {
          settings.setValue("key", key);
          settings.setValue("secret", m_d->crypt(label, secret));
          return;
        }
        if (settings.value("key").toString() == key) {
          settings.setValue("secret", m_d->crypt(label, secret));
          return;
        }
      }
      settings.endArray();

      settings.beginWriteArray("secrets", size + 1);
      settings.setArrayIndex(size);
      settings.setValue("key", key);
      settings.setValue("secret", m_d->crypt(label, secret));
      settings.endArray();
    });
  }

  folly::Future<folly::Unit> SecretStore::clearSecret(const QString & key)
  {
    return folly::via(m_d->m_executor.get(), [this, key] {
      QSettings settings(m_d->m_organization, m_d->m_application);
      int size = settings.beginReadArray("secrets");
      for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        if (settings.value("key").toString() == key) {
          // QSettings can't actually remove array items properly, we can
          // just make items empty
          settings.remove("");
          break;
        }
      }
      settings.endArray();
    });
  }
}

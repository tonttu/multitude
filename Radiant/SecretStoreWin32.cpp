#include "OnDemandExecutor.hpp"
#include "SecretStore.hpp"
#include "StringUtils.hpp"
#include "Trace.hpp"

#include <QSettings>

#include <optional>

#include <dpapi.h>

namespace Radiant
{
  class SecretStoreWindows : public SecretStore
  {
  public:
    SecretStoreWindows(const QString & organization, const QString & application, Flags flags);
    ~SecretStoreWindows();

    virtual folly::Future<QString> secret(const QString & key) override;
    virtual folly::Future<folly::Unit> setSecret(
        const QString & label, const QString & key, const QString & secret) override;
    virtual folly::Future<folly::Unit> clearSecret(const QString & key) override;

    std::optional<QString> decrypt(QByteArray data);
    std::optional<QByteArray> crypt(const QString & data, const QString & label);

  public:
    std::unique_ptr<Radiant::OnDemandExecutor> m_executor{new Radiant::OnDemandExecutor()};
    QString m_organization;
    QString m_application;
    SecretStore::Flags m_flags;

    std::unique_ptr<SecretStore> m_fallback;
  };

  std::optional<QString> SecretStoreWindows::decrypt(QByteArray data)
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
      return {};
    }
  }

  std::optional<QByteArray> SecretStoreWindows::crypt(const QString & description, const QString & data)
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
      return {};
    }
  }

  SecretStoreWindows::SecretStoreWindows(const QString & organization, const QString & application, Flags flags)
    : m_organization(organization)
    , m_application(application)
    , m_flags(flags)
  {}

  SecretStoreWindows::~SecretStoreWindows()
  {
    m_executor.reset();
  }

  folly::Future<QString> SecretStoreWindows::secret(const QString & key)
  {
    if (m_fallback)
      return m_fallback->secret(key);

    return folly::via(m_executor.get(), [this, key] () -> folly::Future<QString> {
      QSettings settings(m_organization, m_application);
      int size = settings.beginReadArray("secrets");
      for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        if (settings.value("key").toString() == key) {
          QByteArray data = settings.value("secret").toByteArray();
          std::optional<QString> plain = decrypt(data);
          if (plain)
            return *plain;
          if (m_flags & FLAG_USE_FALLBACK) {
            if (!m_fallback)
              m_fallback = SecretStore::createFallback(m_organization, m_application);
            return m_fallback->secret(key);
          }
          throw std::runtime_error("SecretStore failed to decrypt data");
        }
      }
      settings.endArray();
      throw std::runtime_error("Not found");
    });
  }

  folly::Future<folly::Unit> SecretStoreWindows::setSecret(
      const QString & label, const QString & key, const QString & secret)
  {
    if (m_fallback)
      return m_fallback->setSecret(label, key, secret);

    return folly::via(m_executor.get(), [this, label, key, secret] () -> folly::Future<folly::Unit> {
      std::optional<QByteArray> encrypted = crypt(label, secret);
      if (!encrypted) {
        if (m_flags & FLAG_USE_FALLBACK) {
          if (!m_fallback)
            m_fallback = SecretStore::createFallback(m_organization, m_application);
          return m_fallback->setSecret(label, key, secret);
        }
        throw std::runtime_error("SecretStore failed to encrypt data");
      }

      QSettings settings(m_organization, m_application);
      int size = settings.beginReadArray("secrets");
      for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        // Since QSettings can't delete array items, there can be empty
        // children after clearSecret. Reuse those items here.
        if (settings.childKeys().isEmpty() && settings.childGroups().isEmpty()) {
          settings.setValue("key", key);
          settings.setValue("secret", *encrypted);
          return folly::Unit{};
        }
        if (settings.value("key").toString() == key) {
          settings.setValue("secret", *encrypted);
          return folly::Unit{};
        }
      }
      settings.endArray();

      settings.beginWriteArray("secrets", size + 1);
      settings.setArrayIndex(size);
      settings.setValue("key", key);
      settings.setValue("secret", *encrypted);
      settings.endArray();
      return folly::Unit{};
    });
  }

  folly::Future<folly::Unit> SecretStoreWindows::clearSecret(const QString & key)
  {
    if (m_fallback)
      return m_fallback->clearSecret(key);

    return folly::via(m_executor.get(), [this, key] {
      QSettings settings(m_organization, m_application);
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

  std::unique_ptr<SecretStore> SecretStore::create(
      const QString & organization, const QString & application, Flags flags)
  {
    return std::make_unique<SecretStoreWindows>(organization, application, flags);
  }
}

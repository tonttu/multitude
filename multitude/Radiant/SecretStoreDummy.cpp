/// This file contains a dummy implementation of SecretStore. It relies purely
/// on file system security to protect the data, although there is an additional
/// obfuscation layer to avoid storing anything in plain-text.

#include "SecretStore.hpp"
#include "BGThreadExecutor.hpp"
#include "PlatformUtils.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QSettings>

namespace Radiant
{
  namespace
  {
    QString prepareIniFilename(const QString & org, const QString & app)
    {
      QString secretsPath = PlatformUtils::localAppPath() + "/.secrets";
      QString path = secretsPath + "/" + org + "/" + app;
      QDir().mkpath(path);
      QFile::setPermissions(secretsPath, QFileDevice::ReadOwner |
                            QFileDevice::WriteOwner | QFileDevice::ExeOwner);
      return path + "/settings.ini";
    }

    QByteArray encryptionKey()
    {
      return PlatformUtils::getUserHomePath().toUtf8();
    }

    // Toy encryption algorithm to avoid extra dependencies and to obfuscate the content
    QByteArray encrypt(QByteArray data, const QByteArray & key = encryptionKey())
    {
      uint32_t len = data.size();
      data.prepend(reinterpret_cast<const char*>(&len), sizeof(len));
      data.append((64 - (data.size() % 64)) % 64, '0');

      QCryptographicHash hash(QCryptographicHash::Sha3_512);
      hash.addData(key);

      QByteArray result;
      QByteArray tmp(64, '0');

      for (int i = 0; i < data.size(); i += 64) {
        QByteArray cipher = hash.result();
        for (int j = 0; j < 64; ++j)
          tmp[j] = data[i+j] ^ cipher[j];
        result += tmp;
        hash.addData(tmp);
      }

      return result;
    }

    QByteArray decrypt(const QByteArray & data, const QByteArray & key = encryptionKey())
    {
      if (data.size() % 64)
        return QByteArray();

      QCryptographicHash hash(QCryptographicHash::Sha3_512);
      hash.addData(key);

      QByteArray result;
      QByteArray tmp(64, '0');

      for (int i = 0; i < data.size(); i += 64) {
        QByteArray cipher = hash.result();
        for (int j = 0; j < 64; ++j)
          tmp[j] = data[i+j] ^ cipher[j];
        result += tmp;
        hash.addData(data.data() + i, 64);
      }
      const uint32_t * len = reinterpret_cast<const uint32_t*>(result.data());
      if (*len <= result.size() - sizeof(*len)) {
        return result.mid(sizeof(*len), *len);
      } else {
        return QByteArray();
      }
    }
  }

  class SecretStore::D
  {
  public:
    const QString organization;
    const QString application;
    std::shared_ptr<BGThreadExecutor> m_executor = BGThreadExecutor::instance();
  };

  SecretStore::SecretStore(const QString & organization, const QString & application, Flags)
    : m_d(new D{organization, application})
  {}

  SecretStore::~SecretStore()
  {}

  folly::Future<QString> SecretStore::secret(const QString & key)
  {
    return folly::via(m_d->m_executor.get(), [org = m_d->organization, app = m_d->application, key] {
      QSettings settings(prepareIniFilename(org, app), QSettings::IniFormat);
      if (settings.contains(key)) {
        return QString::fromUtf8(decrypt(settings.value(key).toByteArray()));
      } else {
        return QString();
      }
    });
  }

  folly::Future<folly::Unit> SecretStore::setSecret(
      const QString &, const QString & key, const QString & secret)
  {
    return folly::via(m_d->m_executor.get(), [org = m_d->organization, app = m_d->application, key, secret] {
      QSettings settings(prepareIniFilename(org, app), QSettings::IniFormat);
      settings.setValue(key, encrypt(secret.toUtf8()));
    });
  }

  folly::Future<folly::Unit> SecretStore::clearSecret(const QString & key)
  {
    return folly::via(m_d->m_executor.get(), [org = m_d->organization, app = m_d->application, key] {
      QSettings settings(prepareIniFilename(org, app), QSettings::IniFormat);
      settings.remove(key);
    });
  }
}

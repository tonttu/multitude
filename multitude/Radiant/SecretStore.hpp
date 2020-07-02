#pragma once

#include "Export.hpp"

#include <folly/futures/Future.h>

#include <QString>

namespace Radiant
{
  /// Wrapper for libsecret / CryptProtectData
  class RADIANT_API SecretStore
  {
  public:
    SecretStore();
    ~SecretStore();

    /// Finds an existing secret from the store. A secret is identified by a
    /// freely chosen key and value, for instance "my-app-secret": "url".
    folly::Future<QString> secret(const QString & key, const QString & value);
    /// Sets a secret. Label is a human-readable description for the secret,
    /// visualized in password managers like seahorse.
    folly::Future<folly::Unit> setSecret(
        const QString & label, const QString & key, const QString & value, const QString & secret);
    /// Remove a previously set secret
    folly::Future<folly::Unit> clearSecret(const QString & key, const QString & value);

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}

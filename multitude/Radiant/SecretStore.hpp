#pragma once

#include "Export.hpp"
#include "Flags.hpp"

#include <folly/futures/Future.h>

#include <QString>

namespace Radiant
{
  /// Password manager that encrypts secrets in the active user session.
  class RADIANT_API SecretStore
  {
  public:
    enum Flag
    {
      FLAG_NONE      = 0,
      /// If set, SecretStore can open native dialogs (to ask confirmation or password)
      /// if needed.
      FLAG_ALLOW_UI  = 1 << 0
    };
    typedef FlagsT<Flag> Flags;

  public:
    /// On Windows the encrypted secrets are stored in
    /// Computer\HKEY_CURRENT_USER\Software\<organization>\<application>\secrets.
    /// On Linux these are ignored.
    SecretStore(const QString & organization, const QString & application, Flags flags = FLAG_NONE);
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

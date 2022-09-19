/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

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
      FLAG_NONE         = 0,
      /// If set, SecretStore can open native dialogs (to ask confirmation or password)
      /// if needed.
      FLAG_ALLOW_UI     = 1 << 0,
      /// If the native secret store is not available, use a less secure fallback method.
      FLAG_USE_FALLBACK = 1 << 1,
    };
    typedef FlagsT<Flag> Flags;

  public:
    virtual ~SecretStore() {}

    /// Finds an existing secret with the given key from the store. SecretStore
    /// is a key-value store where the effective key is tuple:
    /// (currently logged in user, organization, application, key argument)
    /// and the value is the secret.
    virtual folly::Future<QString> secret(const QString & key) = 0;
    /// Sets a secret for a key. Label is a human-readable description for the secret,
    /// visualized in password managers like seahorse.
    virtual folly::Future<folly::Unit> setSecret(
        const QString & label, const QString & key, const QString & secret) = 0;
    /// Remove a previously set secret
    virtual folly::Future<folly::Unit> clearSecret(const QString & key) = 0;

    /// Creates a native secret store, with a possible fallback to less secure
    /// store if FLAG_USE_FALLBACK is defined.
    ///
    /// On Windows the encrypted secrets are stored in
    /// Computer\HKEY_CURRENT_USER\Software\<organization>\<application>\secrets.
    /// On Linux these are used as attributes in libsecret.
    static std::unique_ptr<SecretStore> create(const QString & organization, const QString & application,
                                               Flags flags = FLAG_NONE);

    /// Creates a less secure "fallback" secret store.
    /// Obfuscated secrets are stored in PlatformUtils::localAppPath() + "/.secrets"
    static std::unique_ptr<SecretStore> createFallback(const QString & organization, const QString & application);

    /// Creates a store that just stores everything in memory. Use this for
    /// tests and debugging.
    static std::unique_ptr<SecretStore> createInMemoryStore();
  };
  MULTI_FLAGS(SecretStore::Flag)
}

/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "SecretStore.hpp"
#include "Trace.hpp"

#include <Radiant/OnDemandExecutor.hpp>

#include <boost/expected/expected.hpp>

#include <libsecret/secret.h>

namespace Radiant
{
  class SecretStoreLinux : public SecretStore
  {
  public:
    SecretStoreLinux(const QString & organization, const QString & application, Flags flags);
    ~SecretStoreLinux();

    virtual folly::Future<QString> secret(const QString & key) override;
    virtual folly::Future<folly::Unit> setSecret(
        const QString & label, const QString & key, const QString & secret) override;
    virtual folly::Future<folly::Unit> clearSecret(const QString & key) override;

    boost::expected<SecretService *, QString> service();
    GHashTable * createAttrs(QByteArray & key);
    void initFallback(const char * error);

  public:
    // We are using a custom worker thread and synchronous versions of libsecret
    // functions instead of just using the async versions, since the async ones
    // require a glib main context. Qt and CEF by default use glib which leads
    // Qt events being handled in CEF main thread and the other way around, so
    // we have disabled glib main context in Qt. If we would use async API here,
    // it would only work if we had a browser open, and even then the callbacks
    // would be called in CEF main thread, which is not what we want.
    SecretService * m_service = nullptr;
    SecretCollection * m_collection = nullptr;

    std::unique_ptr<Radiant::OnDemandExecutor> m_executor{new Radiant::OnDemandExecutor()};
    QByteArray m_organization;
    QByteArray m_application;
    Flags m_flags;

    // If this is defined, we had a non-recoverable error and are using
    // fallback store instead.
    std::unique_ptr<SecretStore> m_fallback;
  };

  boost::expected<SecretService *, QString> SecretStoreLinux::service()
  {
    if (m_fallback)
      return boost::make_unexpected(QString());

    GError * error = nullptr;
    if (!m_service) {
      m_service = secret_service_get_sync(SECRET_SERVICE_NONE, nullptr, &error);
      if (!m_service) {
        if (error && error->message)
          return boost::make_unexpected(error->message);
        return boost::make_unexpected("SecretStore # failed to open secret service");
      }
    }

    if (!m_collection) {
      m_collection = secret_collection_for_alias_sync(
            m_service, SECRET_COLLECTION_DEFAULT,
            SECRET_COLLECTION_LOAD_ITEMS, nullptr, &error);
      if (!m_collection) {
        if (error && error->message)
          return boost::make_unexpected(error->message);
        return boost::make_unexpected("SecretStore # failed to open default collection");
      }
    }

    if (secret_collection_get_locked(m_collection)) {
      if (m_flags & SecretStore::FLAG_ALLOW_UI) {
        GList * objects = g_list_append(nullptr, m_collection);
        GList * unlocked = nullptr;

        secret_service_unlock_sync(m_service, objects, nullptr, &unlocked, &error);

        bool ok = g_list_length(unlocked) == 1;
        g_list_free(objects);
        g_list_free(unlocked);

        if (ok) {
          return m_service;
        } else {
          if (error && error->message)
            return boost::make_unexpected(error->message);
          return boost::make_unexpected("SecretStore # failed to unlock secret store");
        }
      }

      return boost::make_unexpected("SecretStore # secret store is locked");
    }

    return m_service;
  }

  GHashTable * SecretStoreLinux::createAttrs(QByteArray & key)
  {
    GHashTable * attrs = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(attrs, const_cast<char*>("organization"), m_organization.data());
    g_hash_table_insert(attrs, const_cast<char*>("application"), m_application.data());
    g_hash_table_insert(attrs, const_cast<char*>("key"), key.data());
    return attrs;
  }

  void SecretStoreLinux::initFallback(const char * error)
  {
    if (!m_fallback) {
      Radiant::info("SecretStore # %s - falling back to a secondary secret store", error);
      m_fallback = SecretStore::createFallback(m_organization, m_application);
    }
  }

  SecretStoreLinux::SecretStoreLinux(const QString & organization, const QString & application, Flags flags)
    : m_organization(organization.toUtf8())
    , m_application(application.toUtf8())
    , m_flags(flags)
  {
  }

  SecretStoreLinux::~SecretStoreLinux()
  {
    m_executor.reset();
    if (m_collection)
      g_object_unref(m_collection);
    if (m_service)
      g_object_unref(m_service);
  }

  folly::Future<QString> SecretStoreLinux::secret(const QString & key)
  {
    if (m_fallback)
      return m_fallback->secret(key);

    return folly::via(m_executor.get(), [this, key] () -> folly::Future<QString> {
      auto srv = service();
      if (!srv) {
        if (m_flags & FLAG_USE_FALLBACK) {
          initFallback(srv.error().toUtf8().data());
          return m_fallback->secret(key);
        }
        throw std::runtime_error(srv.error().toStdString());
      }

      QByteArray keyStr = key.toUtf8();
      GHashTable * attrs = createAttrs(keyStr);

      GError * error = nullptr;
      SecretValue * secretValue = secret_service_lookup_sync(*srv, nullptr, attrs, nullptr, &error);
      g_hash_table_unref(attrs);

      if (secretValue) {
        gsize length = 0;
        const gchar * data = secret_value_get(secretValue, &length);
        QString ret = QString::fromUtf8(data, length);
        secret_value_unref(secretValue);
        return ret;
      }

      if (error && error->message)
        throw std::runtime_error(error->message);

      throw std::runtime_error("Not found");
    });
  }

  folly::Future<folly::Unit> SecretStoreLinux::setSecret(
      const QString & label, const QString & key, const QString & secret)
  {
    if (m_fallback)
      return m_fallback->setSecret(label, key, secret);

    return folly::via(m_executor.get(), [this, label, key, secret] () -> folly::Future<folly::Unit> {
      auto srv = service();
      if (!srv) {
        if (m_flags & FLAG_USE_FALLBACK) {
          initFallback(srv.error().toUtf8().data());
          return m_fallback->setSecret(label, key, secret);
        }
        throw std::runtime_error(srv.error().toStdString());
      }

      QByteArray keyStr = key.toUtf8();
      GHashTable * attrs = createAttrs(keyStr);

      SecretValue * secretValue = secret_value_new(secret.toUtf8().data(), -1, "text/plain");

      GError * error = nullptr;
      bool ok = secret_service_store_sync(*srv, nullptr, attrs, nullptr, label.toUtf8().data(),
                                          secretValue, nullptr, &error);
      secret_value_unref(secretValue);
      g_hash_table_unref(attrs);

      if (ok)
        return folly::Unit();

      if (error && error->message)
        throw std::runtime_error(error->message);

      throw std::runtime_error("SecretStore store failed");
    });
  }

  folly::Future<folly::Unit> SecretStoreLinux::clearSecret(const QString & key)
  {
    if (m_fallback)
      return m_fallback->clearSecret(key);

    return folly::via(m_executor.get(), [this, key] () -> folly::Future<folly::Unit> {
      auto srv = service();
      if (!srv) {
        if (m_flags & FLAG_USE_FALLBACK) {
          initFallback(srv.error().toUtf8().data());
          return m_fallback->clearSecret(key);
        }
        throw std::runtime_error(srv.error().toStdString());
      }

      QByteArray keyStr = key.toUtf8();
      GHashTable * attrs = createAttrs(keyStr);

      GError * error = nullptr;
      secret_service_clear_sync(*srv, nullptr, attrs, nullptr, &error);
      g_hash_table_unref(attrs);

      if (error && error->message)
        throw std::runtime_error(error->message);

      return folly::Unit();
    });
  }

  std::unique_ptr<SecretStore> SecretStore::create(
      const QString & organization, const QString & application, Flags flags)
  {
    return std::make_unique<SecretStoreLinux>(organization, application, flags);
  }
}

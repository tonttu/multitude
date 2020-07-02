#include "SecretStore.hpp"

#include <Radiant/OnDemandExecutor.hpp>

#include <libsecret/secret.h>

namespace Radiant
{
  class SecretStore::D
  {
  public:
    SecretService * service();

  public:
    // We are using a custom worker thread and synchronous versions of libsecret
    // functions instead of just using the async versions, since the async ones
    // require a glib main context. Qt and CEF by default use glib which leads
    // Qt events being handled in CEF main thread and the other way around, so
    // we have disabled glib main context in Qt. If we would use async API here,
    // it would only work if we had a browser open, and even then the callbacks
    // would be called in CEF main thread, which is not what we want.
    SecretService * m_service = nullptr;
    std::unique_ptr<Radiant::OnDemandExecutor> m_executor{new Radiant::OnDemandExecutor()};
    SecretStore::Flags m_flags;
  };

  SecretService * SecretStore::D::service()
  {
    if (m_service)
      return m_service;

    GError * error = nullptr;
    m_service = secret_service_get_sync(SECRET_SERVICE_NONE, nullptr, &error);
    if (!m_service) {
      if (error && error->message)
        throw std::runtime_error(error->message);

      throw std::runtime_error("SecretStore service failed");
    }
    return m_service;
  }

  /////////////////////////////////////////////////////////////////////////////

  SecretStore::SecretStore(const QString & /*organization*/, const QString & /*application*/, Flags flags)
    : m_d(new D())
  {
    m_d->m_flags = flags;
  }

  SecretStore::~SecretStore()
  {
    m_d->m_executor.reset();
    if (m_d->m_service)
      g_object_unref(m_d->m_service);
  }

  folly::Future<QString> SecretStore::secret(const QString & key, const QString & value)
  {
    return folly::via(m_d->m_executor.get(), [this, key, value] {
      SecretService * srv = m_d->service();

      QByteArray keyStr = key.toUtf8();
      QByteArray valueStr = value.toUtf8();
      GHashTable * attrs = g_hash_table_new(g_str_hash, g_str_equal);
      g_hash_table_insert(attrs, keyStr.data(), valueStr.data());

      GError * error = nullptr;
      SecretValue * secretValue = secret_service_lookup_sync(srv, nullptr, attrs, nullptr, &error);
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

  folly::Future<folly::Unit> SecretStore::setSecret(
      const QString & label, const QString & key, const QString & value, const QString & secret)
  {
    return folly::via(m_d->m_executor.get(), [this, label, key, value, secret] {
      SecretService * srv = m_d->service();

      QByteArray keyStr = key.toUtf8();
      QByteArray valueStr = value.toUtf8();
      GHashTable * attrs = g_hash_table_new(g_str_hash, g_str_equal);
      g_hash_table_insert(attrs, keyStr.data(), valueStr.data());

      SecretValue * secretValue = secret_value_new(secret.toUtf8().data(), -1, "text/plain");

      GError * error = nullptr;
      bool ok = secret_service_store_sync(srv, nullptr, attrs, nullptr, label.toUtf8().data(),
                                          secretValue, nullptr, &error);
      secret_value_unref(secretValue);
      g_hash_table_unref(attrs);

      if (ok)
        return;

      if (error && error->message)
        throw std::runtime_error(error->message);

      throw std::runtime_error("SecretStore store failed");
    });
  }

  folly::Future<folly::Unit> SecretStore::clearSecret(const QString & key, const QString & value)
  {
    return folly::via(m_d->m_executor.get(), [this, key, value] {
      SecretService * srv = m_d->service();

      QByteArray keyStr = key.toUtf8();
      QByteArray valueStr = value.toUtf8();
      GHashTable * attrs = g_hash_table_new(g_str_hash, g_str_equal);
      g_hash_table_insert(attrs, keyStr.data(), valueStr.data());

      GError * error = nullptr;
      secret_service_clear_sync(srv, nullptr, attrs, nullptr, &error);
      g_hash_table_unref(attrs);

      if (error && error->message)
        throw std::runtime_error(error->message);
    });
  }
}

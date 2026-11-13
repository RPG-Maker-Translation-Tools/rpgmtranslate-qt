#pragma once

#include "Utils.hpp"

#include <QString>

// The credential store of the system, reached through the Rust core. A class so that a test can put one that refuses
// in its place: the window does not care which one it has.
class CredentialStore {
   public:
    CredentialStore() = default;
    CredentialStore(const CredentialStore&) = delete;
    auto operator=(const CredentialStore&) -> CredentialStore& = delete;
    virtual ~CredentialStore() = default;

    [[nodiscard]] virtual auto store(const QByteArrayView account, const QByteArrayView secret) -> bool {
        return rpgm_store_credential(strtoffi(account), strtoffi(secret));
    }

    [[nodiscard]] virtual auto remove(const QByteArrayView account) -> bool {
        return rpgm_delete_credential(strtoffi(account));
    }

    // Why the last call that returned false failed
    [[nodiscard]] virtual auto error() -> QString { return ffitostr(rpgm_error()).toString(); }
};

#pragma once

#include "CredentialStore.hpp"

#include <gmock/gmock.h>

// A credential store whose answers a test decides, and whose calls it checks
class MockCredentialStore : public CredentialStore {
   public:
    MOCK_METHOD(bool, store, (QByteArrayView account, QByteArrayView secret), (override));
    MOCK_METHOD(bool, remove, (QByteArrayView account), (override));
    MOCK_METHOD(QString, error, (), (override));
};

// QtTest does not know GMock, and GMock reports its failures to GoogleTest: this collects them for the test to check
class GMockFailures final : public testing::EmptyTestEventListener {
   public:
    // The listener list owns what it is given, and so does the one test that makes it
    static auto install() -> GMockFailures* {
        static GMockFailures* listener = [] -> GMockFailures* {
            int argc = 1;
            char program[] = "tst";
            char* argv[] = { program, nullptr };
            testing::InitGoogleMock(&argc, argv);

            auto* const added = new GMockFailures();
            testing::UnitTest::GetInstance()->listeners().Append(added);
            return added;
        }();

        listener->messages.clear();
        return listener;
    }

    void OnTestPartResult(const testing::TestPartResult& result) override {
        if (result.failed()) {
            messages.append(QString::fromUtf8(result.message()));
        }
    }

    QStringList messages;
};

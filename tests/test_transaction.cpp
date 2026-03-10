#include "sspm/transaction.h"
#include <gtest/gtest.h>

TEST(TransactionTest, BeginReturnsId) {
    sspm::Transaction tx("/tmp/test_tx.db");
    std::string id = tx.begin();
    EXPECT_FALSE(id.empty());
    EXPECT_EQ(id.substr(0, 3), "tx-");
}

TEST(TransactionTest, CommitSucceeds) {
    sspm::Transaction tx("/tmp/test_tx.db");
    auto id = tx.begin();
    auto r = tx.commit(id);
    EXPECT_TRUE(r.success);
}

TEST(TransactionTest, RollbackSucceeds) {
    sspm::Transaction tx("/tmp/test_tx.db");
    auto id = tx.begin();
    auto r = tx.rollback(id);
    EXPECT_TRUE(r.success);
}

TEST(TransactionTest, ExecuteSuccess) {
    sspm::Transaction tx("/tmp/test_tx.db");
    auto r = tx.execute([]() -> sspm::Result {
        return { true, "ok", "" };
    });
    EXPECT_TRUE(r.success);
}

TEST(TransactionTest, ExecuteFailure) {
    sspm::Transaction tx("/tmp/test_tx.db");
    auto r = tx.execute([]() -> sspm::Result {
        return { false, "", "simulated error" };
    });
    EXPECT_FALSE(r.success);
}

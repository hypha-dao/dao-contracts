package dao_test

import (
	"testing"

    "github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-contracts/dao-go"
	"gotest.tools/assert"
)

func TestHvoiceMigration(t *testing.T) {
    teardownTestCase := setupTestCase(t)
    defer teardownTestCase(t)

    // var env Environment
    env = SetupEnvironment(t)

    t.Run("Configuring the DAO environment: ", func(t *testing.T) {
        t.Log(env.String())
        t.Log("\nDAO Environment Setup complete\n")
    })
    t.Run("Test Hvoice migration", func(t *testing.T) {
        hvoiceAsset, _ := eos.NewAssetFromString("1.00 HVOICE")

        // Initial issued is 5.00 (Alice, mem1, mem2, mem3, mem4) all have a genesis of 1.00 each
        stats, err := dao.GetHvoiceIssued(env.ctx, &env.api, env.HvoiceToken, hvoiceAsset.Symbol)
        assert.NilError(t, err)
        assert.Equal(t, stats.Supply.Amount, eos.Int64(500))

        _ ,err = dao.MigrateHVoice(env.ctx, &env.api, env.HvoiceToken, env.DAO, env.TelosDecide)
        assert.NilError(t, err)

        for _, member := range env.Members {
            AccountDetails, err := dao.GetMemberHVoiceAccount(env.ctx, &env.api, env.HvoiceToken, member.Member)
            assert.NilError(t, err)
            // 1.00 hvoice was added somewhere...
            assert.Equal(t, AccountDetails.Balance.Amount, eos.Int64(200))
        }

        AccountDetails, err := dao.GetMemberHVoiceAccount(env.ctx, &env.api, env.HvoiceToken, env.Alice.Member)
        assert.NilError(t, err)
        assert.Equal(t, AccountDetails.Balance.Amount, eos.Int64(10100))

        // All accounts started with 1.00 hvoice (because of the genesis hvoice)
        // All telos started with 1.00 hvoice except for Alice (100.00)
        // 1.00 hvoice was added somewhere to all the accounts
        stats, err = dao.GetHvoiceIssued(env.ctx, &env.api, env.HvoiceToken, hvoiceAsset.Symbol)
        assert.NilError(t, err)
        assert.Equal(t, stats.Supply.Amount, eos.Int64(10900))

    })
}

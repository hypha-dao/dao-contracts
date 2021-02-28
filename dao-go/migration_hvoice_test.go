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

        _ ,err := dao.MigrateHVoice(env.ctx, &env.api, env.HvoiceToken, env.DAO, env.TelosDecide)
        assert.NilError(t, err)

        for _, member := range env.Members {
            AccountDetails, err := dao.GetMemberHVoiceAccount(env.ctx, &env.api, env.HvoiceToken, member.Member)
            assert.NilError(t, err)
            assert.Equal(t, AccountDetails.Balance.Amount, eos.Int64(200))
        }

        AccountDetails, err := dao.GetMemberHVoiceAccount(env.ctx, &env.api, env.HvoiceToken, env.Alice.Member)
        assert.NilError(t, err)
        assert.Equal(t, AccountDetails.Balance.Amount, eos.Int64(10100))
    })
}

package dao_test

import (
	"testing"

    "github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-contracts/dao-go"
	"gotest.tools/assert"
)

func TestHvoiceXX(t *testing.T) {
    teardownTestCase := setupTestCase(t)
    defer teardownTestCase(t)

    // var env Environment
    env = SetupEnvironment(t)
    t.Run("Configuring the DAO environment: ", func(t *testing.T) {
        t.Log(env.String())
        t.Log("\nDAO Environment Setup complete\n")
    })

    t.Run("Test Hvoice", func(t *testing.T) {
        // There is no migration on this test, so consider these as the only votes.
        hvoiceAsset, _ := eos.NewAssetFromString("100.00 HVOICE")

        // Only issuer can issue to issuer account
        _ ,err := dao.IssueHVoice(env.ctx, &env.api, env.HvoiceToken, env.DAO, env.Members[0].Member, hvoiceAsset)
        assert.ErrorContains(t, err, "tokens can only be issued to issuer account")

        _ ,err = dao.IssueHVoice(env.ctx, &env.api, env.HvoiceToken, env.Members[0].Member, env.DAO, hvoiceAsset)
        assert.ErrorContains(t, err, "missing authority of dao.hypha")

        _ ,err = dao.IssueHVoice(env.ctx, &env.api, env.HvoiceToken, env.DAO, env.DAO, hvoiceAsset)
        assert.NilError(t, err)

        // Only issuer can transfer FROM issuer account to any account
        _, err = dao.TransferHVoice(env.ctx, &env.api, env.HvoiceToken, env.Members[1].Member, env.Members[1].Member, env.Members[0].Member, hvoiceAsset)
        assert.ErrorContains(t, err, "tokens can only be transferred by issuer account")

        _, err = dao.TransferHVoice(env.ctx, &env.api, env.HvoiceToken, env.Members[1].Member, env.DAO, env.Members[0].Member, hvoiceAsset)
        assert.ErrorContains(t, err, "missing authority of dao.hypha")

        // Test transfer
        _, err = dao.TransferHVoice(env.ctx, &env.api, env.HvoiceToken, env.DAO, env.DAO, env.Members[0].Member, hvoiceAsset)
        assert.NilError(t, err)

        voiceAccount, err := dao.GetMemberHVoiceAccount(env.ctx, &env.api, env.HvoiceToken, env.Members[0].Member)
        assert.NilError(t, err)

        // Wait 6 seconds (Decay is every 5 seconds and decays 50%).
        pause(t, 6000000000, "", "Waiting for a decay")

        t.Log("first decay period", voiceAccount.LastDecayPeriod)

        // Issue and transfer
        _ ,err = dao.IssueHVoice(env.ctx, &env.api, env.HvoiceToken, env.DAO, env.DAO, hvoiceAsset)
        assert.NilError(t, err)
        _, err = dao.TransferHVoice(env.ctx, &env.api, env.HvoiceToken, env.DAO, env.DAO, env.Members[0].Member, hvoiceAsset)
        assert.NilError(t, err)

        lastVoiceAccount, err := dao.GetMemberHVoiceAccount(env.ctx, &env.api, env.HvoiceToken, env.Members[0].Member)
        assert.NilError(t, err)

        t.Log("last decay period", lastVoiceAccount.LastDecayPeriod)

        assert.Equal(t, lastVoiceAccount.Balance.Amount, float32(voiceAccount.Balance.Amount) * 0.5 + float32(hvoiceAsset.Amount))

    })
}
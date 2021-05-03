package dao

import (
	"testing"

    eostest "github.com/digital-scarcity/eos-go-test"
    "github.com/eoscanada/eos-go"
	"gotest.tools/assert"
)

func TestHvoiceIssuingAndTransfering(t *testing.T) {
    teardownTestCase := setupTestCase(t)
    defer teardownTestCase(t)

    // var env Environment
    env = SetupEnvironmentWithFlags(t, false, true)
    t.Run("Configuring the DAO environment: ", func(t *testing.T) {
        t.Log(env.String())
        t.Log("\nDAO Environment Setup complete\n")
    })

    t.Run("Test Hvoice", func(t *testing.T) {
        // There is no migration on this test, so consider these as the only votes.
        hvoiceAsset, _ := eos.NewAssetFromString("100.00 HVOICE")

        // Initial issued is: 109.00
        // Alice: 100.00
        // mem1:    1.00
        // mem2:    1.00
        // mem3:    1.00
        // mem4:    1.00
        //  all have a genesis of 1.00 each
        stats, err := GetHvoiceIssued(env.ctx, &env.api, env.HvoiceToken, hvoiceAsset.Symbol)
        assert.NilError(t, err)
        assert.Equal(t, stats.Supply.Amount, eos.Int64(10900))

        // Each member takes some time to create, use the last member to avoid the "decay" while adding the others.
        // Only issuer can issue to issuer account
        _ ,err = IssueHVoice(env.ctx, &env.api, env.HvoiceToken, env.DAO, env.Members[0].Member, hvoiceAsset)
        assert.ErrorContains(t, err, "tokens can only be issued to issuer account")

        _ ,err = IssueHVoice(env.ctx, &env.api, env.HvoiceToken, env.Members[0].Member, env.DAO, hvoiceAsset)
        assert.ErrorContains(t, err, "missing authority of dao.hypha")

        _ ,err = IssueHVoice(env.ctx, &env.api, env.HvoiceToken, env.DAO, env.DAO, hvoiceAsset)
        assert.NilError(t, err)

        // Issued 100.00
        stats, err = GetHvoiceIssued(env.ctx, &env.api, env.HvoiceToken, hvoiceAsset.Symbol)
        assert.NilError(t, err)
        assert.Equal(t, stats.Supply.Amount, eos.Int64(20900))

        // Only issuer can transfer FROM issuer account to any account
        _, err = TransferHVoice(env.ctx, &env.api, env.HvoiceToken, env.Members[1].Member, env.Members[1].Member, env.Members[0].Member, hvoiceAsset)
        assert.ErrorContains(t, err, "tokens can only be transferred by issuer account")

        _, err = TransferHVoice(env.ctx, &env.api, env.HvoiceToken, env.Members[1].Member, env.DAO, env.Members[0].Member, hvoiceAsset)
        assert.ErrorContains(t, err, "missing authority of dao.hypha")

        // Test transfer
        _, err = TransferHVoice(env.ctx, &env.api, env.HvoiceToken, env.DAO, env.DAO, env.Members[3].Member, hvoiceAsset)
        assert.NilError(t, err)

        // Still 209.00
        stats, err = GetHvoiceIssued(env.ctx, &env.api, env.HvoiceToken, hvoiceAsset.Symbol)
        assert.NilError(t, err)
        assert.Equal(t, stats.Supply.Amount, eos.Int64(20900))

        voiceAccount, err := GetMemberHVoiceAccount(env.ctx, &env.api, env.HvoiceToken, env.Members[3].Member)
        assert.NilError(t, err)

        // Wait 6 seconds (Decay is every 5 seconds and decays 50%).
        eostest.Pause(6000000000, "", "Waiting for a decay")

        // Issued 105.00 (not yet decayed until a transaction takes place)
        stats, err = GetHvoiceIssued(env.ctx, &env.api, env.HvoiceToken, hvoiceAsset.Symbol)
        assert.NilError(t, err)
        assert.Equal(t, stats.Supply.Amount, eos.Int64(20900))

        // Issue and transfer
        _ ,err = IssueHVoice(env.ctx, &env.api, env.HvoiceToken, env.DAO, env.DAO, hvoiceAsset)
        assert.NilError(t, err)
        _, err = TransferHVoice(env.ctx, &env.api, env.HvoiceToken, env.DAO, env.DAO, env.Members[3].Member, hvoiceAsset)
        assert.NilError(t, err)

        // Decayed 50% only of mem1; 102.00 * .5 =  51.00, 100.00 added and the other members voice
        stats, err = GetHvoiceIssued(env.ctx, &env.api, env.HvoiceToken, hvoiceAsset.Symbol)
        assert.NilError(t, err)
        assert.Equal(t, stats.Supply.Amount, eos.Int64(25800))

        lastVoiceAccount, err := GetMemberHVoiceAccount(env.ctx, &env.api, env.HvoiceToken, env.Members[3].Member)
        assert.NilError(t, err)

        assert.Equal(t, lastVoiceAccount.Balance.Amount, eos.Int64(float32(voiceAccount.Balance.Amount) * 0.5 + float32(hvoiceAsset.Amount)))
    })
}
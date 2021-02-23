package dao

import (
	"context"
	"log"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
)

type MigrateHVoiceArgs struct {
    DaoContract    eos.AccountName
    TrailContract  eos.AccountName
}

type Account struct {
    Balance           eos.Asset   `json:"balance"`
    LastDecayPeriod   eos.Uint64  `json:"last_decay_period"`
}

func MigrateHVoice(ctx context.Context, api *eos.API, contract, daoContract eos.AccountName, trailContract  eos.AccountName) (string, error) {
    action := eos.ActN("migrate")
    actions := []*eos.Action{{
        Account: contract,
        Name:    action,
        Authorization: []eos.PermissionLevel{
            {Actor: contract, Permission: eos.PN("active")},
        },
        ActionData: eos.NewActionData(&MigrateHVoiceArgs{
            DaoContract:    daoContract,
            TrailContract:  trailContract,
        }),
    }}

    return eostest.ExecTrx(ctx, api, actions)
}

func GetMemberHVoiceAccount(ctx context.Context, api *eos.API, contract, memberAccount eos.AccountName) (Account, error) {
    var account []Account
    var request eos.GetTableRowsRequest
    request.Code = string(contract)
    request.Scope = string(memberAccount)
    request.Table = "accounts"
    request.Limit = 1000
    request.JSON = true
    response, err := api.GetTableRows(ctx, request)
    if err != nil {
        log.Println("Error with GetTableRows: ", err)
        return Account{}, err
    }

    err = response.JSONToStructs(&account)
    if err != nil {
        log.Println("Error with JSONToStructs: ", err)
        return Account{}, err
    }
    return account[0], nil
}

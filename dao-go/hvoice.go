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

type IssueHVoiceArgs struct {
    To          eos.AccountName
    Quantity    eos.Asset
    Memo        string
}

type TransferHVoiceArgs struct {
    From        eos.AccountName
    To          eos.AccountName
    Quantity    eos.Asset
    Memo        string
}

type Account struct {
    Balance           eos.Asset   `json:"balance"`
    LastDecayPeriod   eos.Uint64  `json:"last_decay_period"`
}

type Stats struct {
    Supply           eos.Asset          `json:"supply"`
    MaxSupply        eos.Asset          `json:"max_supply"`
    Name             eos.AccountName    `json:"name"`
    DecayPerPeriod   string             `json:"decay_per_period"`
    DecayPeriod      eos.Uint64         `json:"last_decay_period"`
}

func IssueHVoice(ctx context.Context, api *eos.API, contract, auth eos.AccountName,
                 to eos.AccountName, quantity eos.Asset) (string, error) {
    action := eos.ActN("issue")
    actions := []*eos.Action{{
        Account: contract,
        Name:    action,
        Authorization: []eos.PermissionLevel{
            {Actor: auth, Permission: eos.PN("active")},
        },
        ActionData: eos.NewActionData(&IssueHVoiceArgs{
            To:          to,
            Quantity:    quantity,
            Memo:        "HVoice transfer",
        }),
    }}

    return eostest.ExecTrx(ctx, api, actions)
}

func TransferHVoice(ctx context.Context, api *eos.API, contract, auth eos.AccountName,
                    from eos.AccountName, to eos.AccountName, quantity eos.Asset) (string, error) {
    action := eos.ActN("transfer")
    actions := []*eos.Action{{
        Account: contract,
        Name:    action,
        Authorization: []eos.PermissionLevel{
            {Actor: auth, Permission: eos.PN("active")},
        },
        ActionData: eos.NewActionData(&TransferHVoiceArgs{
            From:        from,
            To:          to,
            Quantity:    quantity,
            Memo:        "HVoice transfer",
        }),
    }}

    return eostest.ExecTrx(ctx, api, actions)
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

func GetHvoiceIssued(ctx context.Context, api *eos.API, contract eos.AccountName, symbol eos.Symbol) (Stats, error) {
    var stats []Stats
    var request eos.GetTableRowsRequest
    request.Code = string(contract)
    request.Scope = string(symbol.Symbol)
    request.Table = "stat"
    request.Limit = 1000
    request.JSON = true
    response, err := api.GetTableRows(ctx, request)
    if err != nil {
        log.Println("Error with GetTableRows: ", err)
        return Stats{}, err
    }

    err = response.JSONToStructs(&stats)
    if err != nil {
        log.Println("Error with JSONToStructs: ", err)
        return Stats{}, err
    }
    return stats[0], nil
}

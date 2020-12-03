package dao

import (
	"context"
	"fmt"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
)

type appVersion struct {
	AppVersion string
}

type fee struct {
	FeeName   eos.Name
	FeeAmount eos.Asset
}

// InitTD ...
func InitTD(ctx context.Context, api *eos.API, telosDecide eos.AccountName) (string, error) {
	actions := []*eos.Action{{
		Account: telosDecide,
		Name:    eos.ActN("init"),
		Authorization: []eos.PermissionLevel{
			{Actor: telosDecide, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(appVersion{
			AppVersion: "vtest",
		}),
	}}

	_, err := eostest.ExecTrx(ctx, api, actions)
	if err != nil {
		return "error", fmt.Errorf("cannot init telos decide %v", err)
	}

	zeroFee, _ := eos.NewAssetFromString("0.0000 TLOS")
	feeNames := []eos.Name{eos.Name("ballot"), eos.Name("treasury"), eos.Name("archival"), eos.Name("committee")}
	var fees []*eos.Action
	for _, feeName := range feeNames {
		fee := eos.Action{
			Account: telosDecide,
			Name:    eos.ActN("updatefee"),
			Authorization: []eos.PermissionLevel{
				{Actor: telosDecide, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(fee{
				FeeName:   feeName,
				FeeAmount: zeroFee,
			}),
		}
		fees = append(fees, &fee)
	}
	return eostest.ExecTrx(ctx, api, fees)
}

type tDTreasury struct {
	Manager   eos.AccountName
	MaxSupply eos.Asset
	Access    eos.Name
}

// NewTreasury ...
func NewTreasury(ctx context.Context, api *eos.API, telosDecide, treasuryManager eos.AccountName) (string, error) {
	maxSupply, _ := eos.NewAssetFromString("1000000000.00 HVOICE")
	actions := []*eos.Action{{
		Account: telosDecide,
		Name:    eos.ActN("newtreasury"),
		Authorization: []eos.PermissionLevel{
			{Actor: treasuryManager, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(tDTreasury{
			Manager:   treasuryManager,
			MaxSupply: maxSupply,
			Access:    eos.Name("public"),
		}),
	}}
	return eostest.ExecTrx(ctx, api, actions)
}

type transferP struct {
	From     eos.AccountName
	To       eos.AccountName
	Quantity eos.Asset
	Memo     string
}

// Transfer ...
func Transfer(ctx context.Context, api *eos.API, token, from, to eos.AccountName, amount eos.Asset, memo string) (string, error) {

	actions := []*eos.Action{{
		Account: token,
		Name:    eos.ActN("transfer"),
		Authorization: []eos.PermissionLevel{
			{Actor: from, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(transferP{
			From:     from,
			To:       to,
			Quantity: amount,
			Memo:     memo,
		}),
	}}
	return eostest.ExecTrx(ctx, api, actions)
}

type issuance struct {
	To       eos.AccountName
	Quantity eos.Asset
	Memo     string
}

// Issue ...
func Issue(ctx context.Context, api *eos.API, token, issuer eos.AccountName, amount eos.Asset) (string, error) {

	actions := []*eos.Action{{
		Account: token,
		Name:    eos.ActN("issue"),
		Authorization: []eos.PermissionLevel{
			{Actor: issuer, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(issuance{
			To:       issuer,
			Quantity: amount,
			Memo:     "memo",
		}),
	}}
	return eostest.ExecTrx(ctx, api, actions)
}

// Mint ...
func Mint(ctx context.Context, api *eos.API, telosDecide, issuer, receiver eos.AccountName, amount eos.Asset) (string, error) {

	actions := []*eos.Action{{
		Account: telosDecide,
		Name:    eos.ActN("mint"),
		Authorization: []eos.PermissionLevel{
			{Actor: issuer, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(issuance{
			To:       receiver,
			Quantity: amount,
			Memo:     "memo",
		}),
	}}
	return eostest.ExecTrx(ctx, api, actions)
}

// RegVoter ...
func RegVoter(ctx context.Context, api *eos.API, telosDecide, registrant eos.AccountName) (string, error) {

	hvoice, _ := eos.NewAssetFromString("1.00 HVOICE")

	actionData := make(map[string]interface{})
	actionData["voter"] = registrant
	actionData["treasury_symbol"] = hvoice.Symbol.String()
	actionData["referrer"] = registrant

	actionBinary, err := api.ABIJSONToBin(ctx, telosDecide, eos.Name("regvoter"), actionData)
	if err != nil {
		return "abi error", err
	}

	actions := []*eos.Action{{
		Account: telosDecide,
		Name:    eos.ActN("regvoter"),
		Authorization: []eos.PermissionLevel{
			{Actor: registrant, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionDataFromHexData([]byte(actionBinary)),
	}}

	return eostest.ExecTrx(ctx, api, actions)
}

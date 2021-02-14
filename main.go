package main

import (
	"bytes"
	"context"
	"fmt"
	"strconv"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-contracts/dao-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"github.com/spf13/viper"
)

// var mainnet = "https://api.telos.kitchen"
// var testnet = "https://test.telos.kitchen"

type notes struct {
	Notes string `json:"notes"`
}

type eraseobjs struct {
	Scope eos.Name `json:"scope"`
}

func reset4test(ctx context.Context, api *eos.API, contract eos.AccountName) {

	fmt.Println("\nRunning contract reset action on: " + string(contract))
	bar := dao.DefaultProgressBar(1)

	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("reset4test"),
		Authorization: []eos.PermissionLevel{
			{Actor: contract, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(notes{
			Notes: "resetting for testing",
		}),
	}}
	_, err := eostest.ExecTrx(ctx, api, actions)
	if err != nil {
		panic(err)
	}

	bar.Add(1)
}

func eraseobjsByScope(ctx context.Context, api *eos.API, contract eos.AccountName, scope eos.Name) {
	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("eraseobjs"),
		Authorization: []eos.PermissionLevel{
			{Actor: contract, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(eraseobjs{
			Scope: scope,
		}),
	}}
	trxID, err := eostest.ExecTrx(ctx, api, actions)
	if err != nil {
		panic(err)
	}

	fmt.Println("Completed the eraseobjs transaction: " + string(scope) + ": trxID: " + trxID)
	fmt.Println()
}

type claimNext struct {
	AssignmentHash eos.Checksum256 `json:"assignment_hash"`
}

func claimNextPeriod(ctx context.Context, api *eos.API, contract, claimer eos.AccountName, assignment docgraph.Document) (string, error) {

	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("claimnextper"),
		Authorization: []eos.PermissionLevel{
			{Actor: claimer, Permission: eos.PN("active")},
		},
		// ActionData: eos.NewActionDataFromHexData([]byte(actionBinary)),
		ActionData: eos.NewActionData(claimNext{
			AssignmentHash: assignment.Hash,
		}),
	}}

	trxID, err := eostest.ExecTrx(ctx, api, actions)

	if err != nil {
		fmt.Println("Waiting for a period to lapse...")
		time.Sleep(time.Second * 7)
		actions := []*eos.Action{{
			Account: contract,
			Name:    eos.ActN("claimnextper"),
			Authorization: []eos.PermissionLevel{
				{Actor: claimer, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(claimNext{
				AssignmentHash: assignment.Hash,
			}),
		}}

		trxID, err = eostest.ExecTrx(ctx, api, actions)
	}

	return trxID, err
}

func main() {

	viper.SetConfigType("yaml")

	var yamlExample = []byte(`
contract: dao.hypha
host: https://testnet.telos.caleos.io
pause: 1s
periodDuration: 24h
rootHash: 52a7ff82bd6f53b31285e97d6806d886eefb650e79754784e9d923d3df347c91
`)

	// dao.hypha: 52a7ff82bd6f53b31285e97d6806d886eefb650e79754784e9d923d3df347c91
	// dao1.hypha: 0f374e7a9d8ab17f172f8c478744cdd4016497e15229616f2ffd04d8002ef64a
	viper.ReadConfig(bytes.NewBuffer(yamlExample))

	ctx := context.Background()
	keyBag := &eos.KeyBag{}

	// api := eos.New(testnet)
	contract := eos.AccountName(viper.GetString("contract"))
	err := keyBag.ImportPrivateKey(ctx, "5KCZ9VBJMMiLaAY24Ro66mhx4vU1VcJELZVGrJbkUBATyqxyYmj")
	err = keyBag.ImportPrivateKey(ctx, "5HwnoWBuuRmNdcqwBzd1LABFRKnTk2RY2kUMYKkZfF8tKodubtK")
	err = keyBag.ImportPrivateKey(ctx, eostest.DefaultKey())

	// new test key: 5KCZ9VBJMMiLaAY24Ro66mhx4vU1VcJELZVGrJbkUBATyqxyYmj
	// old test key: 5HwnoWBuuRmNdcqwBzd1LABFRKnTk2RY2kUMYKkZfF8tKodubtK
	// api := eos.New("https://test.telos.kitchen")
	api := eos.New(viper.GetString("host"))

	if err != nil {
		panic(err)
	}
	api.SetSigner(keyBag)

	// erase environment except for settings
	// dao.EraseAllDocuments(ctx, api, contract)
	// reset4test(ctx, api, contract)
	// dao.EnrollMembers(ctx, api, contract)

	duration := viper.GetDuration("periodDuration")
	fmt.Println("Adding "+strconv.Itoa(20)+" periods with duration 		 ", duration)

	// settings, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, eos.Name("settings"))
	// if err != nil {
	// 	panic(err)
	// }

	// rootNode, err := settings.GetContent("root_node")
	// if err != nil {
	// 	panic(err)
	// }

	rootDocument, err := docgraph.LoadDocument(ctx, api, contract, "15a0610fa918bdf2cb189a8716887bcb1ec1a811bead03d5ad69dc423565b7c9")
	if err != nil {
		panic(err)
	}

	_, err = dao.AddPeriods(ctx, api, contract, rootDocument.Hash, 20, duration)
	if err != nil {
		panic(err)
	}
	time.Sleep(time.Second)

	// badgeAssignment, err := dao.CreatePretend(ctx, api, contract, eos.AN("trailservice"), eos.AN("mem2.hypha"))
	// if err != nil {
	// 	panic(err)
	// }
	// fmt.Println(badgeAssignment)

	// dao.CopyMembers(ctx, api, contract, "https://api.telos.kitchen")
	// dao.MigrateMembers(ctx, api, contract)

	// dao.CopyPeriods(ctx, api, contract, "https://api.telos.kitchen")
	// dao.MigratePeriods(ctx, api, contract)

	// scopes := []eos.Name{"role", "assignment", "payout"}

	// for _, scope := range scopes {
	// 	dao.CopyObjects(ctx, api, contract, scope, "https://api.telos.kitchen")
	// 	dao.MigrateObjects(ctx, api, contract, scope)
	// }

	// dao.CopyObjects(ctx, api, contract, eos.Name("payout"), "https://api.telos.kitchen")
	// dao.MigrateObjects(ctx, api, contract, eos.Name("payout"))

	// dao.CopyAssPayouts(ctx, api, contract, "https://api.telos.kitchen")
	// dao.MigrateAssPayouts(ctx, api, contract)

	// **********************************************************************
	// *******************
	// test data and use cases

}

// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"paused", "value":["int64","0"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"seeds_token_contract", "value":["name","token.seeds"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"voting_duration_sec", "value":["int64",3600]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"seeds_deferral_factor_x100", "value":["int64","100"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"telos_decide_contract", "value":["name","trailservice"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"husd_token_contract", "value":["name","husd.hypha"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"hypha_token_contract", "value":["name","token.hypha"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"seeds_escrow_contract", "value":["name","escrow.seeds"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"publisher_contract", "value":["name","publsh.hypha"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"treasury_contract", "value":["name","bank.hypha"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"last_ballot_id", "value":["name","hypha1....1ce"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"hypha_deferral_factor_x100", "value":["int64","25"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"seeds_deferral_factor_x100", "value":["int64","100"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"client_version", "value":["string","0.2.0 pre-release"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"contract_version", "value":["string","0.2.0 pre-release"]}' -p dao.hypha

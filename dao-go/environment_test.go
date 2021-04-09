package dao_test

import (
	"context"
	"fmt"
	"io/ioutil"
	"strconv"
	"testing"
	"time"

	"github.com/alexeyco/simpletable"
	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/eoscanada/eos-go/ecc"
	"github.com/eoscanada/eos-go/system"
	"github.com/hypha-dao/dao-contracts/dao-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"gotest.tools/assert"
)

var exchangeWasm, exchangeAbi string

var daoHome = ".."
var daoPrefix = daoHome + "/build/dao/dao."
var docPrefix = daoHome + "/build/doc/docs/docs."

const testingEndpoint = "http://localhost:8888"

type Member struct {
	Member eos.AccountName
	Doc    docgraph.Document
}

type Environment struct {
	ctx context.Context
	api eos.API

	DAO           eos.AccountName
	HusdToken     eos.AccountName
	HyphaToken    eos.AccountName
	HvoiceToken   eos.AccountName
	SeedsToken    eos.AccountName
	Bank          eos.AccountName
	SeedsEscrow   eos.AccountName
	SeedsExchange eos.AccountName
	Events        eos.AccountName
	TelosDecide   eos.AccountName
	Alice         Member
	Bob           Member
	Root          docgraph.Document

	VotingDurationSeconds int64
	HyphaDeferralFactor   int64
	SeedsDeferralFactor   int64

	NumPeriods     int
	PeriodDuration time.Duration

	PeriodPause        time.Duration
	VotingPause        time.Duration
	ChainResponsePause time.Duration

	Members []Member
	Periods []docgraph.Document

	//HVOICE Given to new test members
	GenesisHVOICE int64
}

func envHeader() *simpletable.Header {
	return &simpletable.Header{
		Cells: []*simpletable.Cell{
			{Align: simpletable.AlignCenter, Text: "Variable"},
			{Align: simpletable.AlignCenter, Text: "Value"},
		},
	}
}

func (e *Environment) String() string {
	table := simpletable.New()
	table.Header = envHeader()

	kvs := make(map[string]string)
	kvs["DAO"] = string(e.DAO)
	kvs["HUSD Token"] = string(e.HusdToken)
	kvs["HVOICE Token"] = string(e.HvoiceToken)
	kvs["HYPHA Token"] = string(e.HyphaToken)
	kvs["SEEDS Token"] = string(e.SeedsToken)
	kvs["Bank"] = string(e.Bank)
	kvs["Escrow"] = string(e.SeedsEscrow)
	kvs["Exchange"] = string(e.SeedsExchange)
	kvs["Telos Decide"] = string(e.TelosDecide)
	kvs["Alice"] = string(e.Alice.Member)
	kvs["Bob"] = string(e.Bob.Member)
	kvs["Voting Duration (s)"] = strconv.Itoa(int(e.VotingDurationSeconds))
	kvs["HYPHA deferral X"] = strconv.Itoa(int(e.HyphaDeferralFactor))
	kvs["SEEDS deferral X"] = strconv.Itoa(int(e.SeedsDeferralFactor))

	for key, value := range kvs {
		r := []*simpletable.Cell{
			{Align: simpletable.AlignLeft, Text: key},
			{Align: simpletable.AlignRight, Text: value},
		}
		table.Body.Cells = append(table.Body.Cells, r)
	}

	return table.String()
}

func SetupEnvironment(t *testing.T) *Environment {
	return SetupEnvironmentWithFlags(t, true, true)
}

type Executer func()

// This function swaps the dao and doc contracts to allow raw control of the documents (mocking)
func ExecuteDocgraphCall(t *testing.T, env *Environment, executer Executer) {
	t.Log("Deploying Doc (to allow mock documents) contract to 		: ", env.DAO)
	_, err := eostest.SetContract(env.ctx, &env.api, env.DAO, docPrefix+"wasm", docPrefix+"abi")
	assert.NilError(t, err)

	executer()

	t.Log("Restoring DAO contract to 		: ", env.DAO)
	_, err = eostest.SetContract(env.ctx, &env.api, env.DAO, daoPrefix+"wasm", daoPrefix+"abi")
	assert.NilError(t, err)
}

func SetupEnvironmentWithFlags(t *testing.T, addFakePeriods, addFakeMembers bool) *Environment {

	artifactsHome := "artifacts"
	decidePrefix := artifactsHome + "/decide/decide."
	treasuryPrefix := artifactsHome + "/treasury/treasury."
	monitorPrefix := artifactsHome + "/monitor/monitor."
	escrowPrefix := artifactsHome + "/escrow/escrow."
	voicePrefix := artifactsHome + "/voice/voice."

	exchangeWasm = "mocks/seedsexchg/build/seedsexchg/seedsexchg.wasm"
	exchangeAbi = "mocks/seedsexchg/build/seedsexchg/seedsexchg.abi"

	var env Environment

	env.api = *eos.New(testingEndpoint)
	// api.Debug = true
	env.ctx = context.Background()

	keyBag := &eos.KeyBag{}
	err := keyBag.ImportPrivateKey(env.ctx, eostest.DefaultKey())
	assert.NilError(t, err)

	env.api.SetSigner(keyBag)

	env.VotingDurationSeconds = 30
	env.SeedsDeferralFactor = 100
	env.HyphaDeferralFactor = 25

	env.PeriodDuration, _ = time.ParseDuration("120s")
	env.NumPeriods = 20

	// pauses
	env.ChainResponsePause = time.Second
	env.VotingPause = time.Duration(env.VotingDurationSeconds)*time.Second + time.Second + env.ChainResponsePause
	env.PeriodPause = env.PeriodDuration + time.Second

	var bankKey ecc.PublicKey
	env.DAO, _ = eostest.CreateAccountFromString(env.ctx, &env.api, "dao.hypha", eostest.DefaultKey())
	bankKey, env.Bank, _ = eostest.CreateAccountWithRandomKey(env.ctx, &env.api, "bank.hypha")

	bankPermissionActions := []*eos.Action{system.NewUpdateAuth(env.Bank,
		"active",
		"owner",
		eos.Authority{
			Threshold: 1,
			Keys: []eos.KeyWeight{{
				PublicKey: bankKey,
				Weight:    1,
			}},
			Accounts: []eos.PermissionLevelWeight{
				{
					Permission: eos.PermissionLevel{
						Actor:      env.Bank,
						Permission: "eosio.code",
					},
					Weight: 1,
				},
				{
					Permission: eos.PermissionLevel{
						Actor:      env.DAO,
						Permission: "eosio.code",
					},
					Weight: 1,
				}},
			Waits: []eos.WaitWeight{},
		}, "owner")}

	_, err = eostest.ExecWithRetry(env.ctx, &env.api, bankPermissionActions)
	assert.NilError(t, err)

	_, env.HusdToken, _ = eostest.CreateAccountWithRandomKey(env.ctx, &env.api, "husd.hypha")
	_, env.HvoiceToken, _ = eostest.CreateAccountWithRandomKey(env.ctx, &env.api, "hvoice.hypha")
	_, env.HyphaToken, _ = eostest.CreateAccountWithRandomKey(env.ctx, &env.api, "token.hypha")
	_, env.Events, _ = eostest.CreateAccountWithRandomKey(env.ctx, &env.api, "publsh.hypha")
	_, env.SeedsToken, _ = eostest.CreateAccountWithRandomKey(env.ctx, &env.api, "token.seeds")
	_, env.SeedsEscrow, _ = eostest.CreateAccountWithRandomKey(env.ctx, &env.api, "escrow.seeds")
	_, env.SeedsExchange, _ = eostest.CreateAccountWithRandomKey(env.ctx, &env.api, "tlosto.seeds")

	_, env.TelosDecide, _ = eostest.CreateAccountWithRandomKey(env.ctx, &env.api, "trailservice")

	t.Log("Deploying DAO contract to 		: ", env.DAO)
	_, err = eostest.SetContract(env.ctx, &env.api, env.DAO, daoPrefix+"wasm", daoPrefix+"abi")
	assert.NilError(t, err)

	t.Log("Deploying Treasury contract to 		: ", env.Bank)
	_, err = eostest.SetContract(env.ctx, &env.api, env.Bank, treasuryPrefix+"wasm", treasuryPrefix+"abi")
	assert.NilError(t, err)

	t.Log("Deploying Escrow contract to 		: ", env.SeedsEscrow)
	_, err = eostest.SetContract(env.ctx, &env.api, env.SeedsEscrow, escrowPrefix+"wasm", escrowPrefix+"abi")
	assert.NilError(t, err)

	t.Log("Deploying voice.hypha contract to 		: ", env.HvoiceToken)
	_, err = eostest.SetContract(env.ctx, &env.api, env.HvoiceToken, voicePrefix+"wasm", voicePrefix+"abi")
	assert.NilError(t, err)

	t.Log("Deploying SeedsExchange contract to 		: ", env.SeedsExchange)
	_, err = eostest.SetContract(env.ctx, &env.api, env.SeedsExchange, exchangeWasm, exchangeAbi)
	assert.NilError(t, err)
	loadSeedsTablesFromProd(t, &env, "https://api.telos.kitchen")

	t.Log("Deploying Events contract to 		: ", env.Events)
	_, err = eostest.SetContract(env.ctx, &env.api, env.Events, monitorPrefix+"wasm", monitorPrefix+"abi")
	assert.NilError(t, err)

	_, err = dao.CreateRoot(env.ctx, &env.api, env.DAO)
	assert.NilError(t, err)
	env.Root, err = docgraph.LoadDocument(env.ctx, &env.api, env.DAO, "52a7ff82bd6f53b31285e97d6806d886eefb650e79754784e9d923d3df347c91")
	assert.NilError(t, err)

	husdMaxSupply, _ := eos.NewAssetFromString("1000000000.00 HUSD")
	_, err = eostest.DeployAndCreateToken(env.ctx, t, &env.api, artifactsHome, env.HusdToken, env.Bank, husdMaxSupply)
	assert.NilError(t, err)

	hyphaMaxSupply, _ := eos.NewAssetFromString("1000000000.00 HYPHA")
	_, err = eostest.DeployAndCreateToken(env.ctx, t, &env.api, artifactsHome, env.HyphaToken, env.DAO, hyphaMaxSupply)
	assert.NilError(t, err)

	// Hvoice doesn't have any limit (max supply should be -1)
	hvoiceMaxSupply, _ := eos.NewAssetFromString("-1.00 HVOICE")
	_, err = CreateHVoiceToken(env.ctx, t, &env.api, env.HvoiceToken, env.DAO, hvoiceMaxSupply, 5, 5000000)
	assert.NilError(t, err)

	seedsMaxSupply, _ := eos.NewAssetFromString("1000000000.0000 SEEDS")
	_, err = eostest.DeployAndCreateToken(env.ctx, t, &env.api, artifactsHome, env.SeedsToken, env.DAO, seedsMaxSupply)
	assert.NilError(t, err)

	_, err = dao.Issue(env.ctx, &env.api, env.SeedsToken, env.DAO, seedsMaxSupply)
	assert.NilError(t, err)

	t.Log("Setting configuration options on DAO 		: ", env.DAO)
	_, err = dao.SetIntSetting(env.ctx, &env.api, env.DAO, "voting_duration_sec", env.VotingDurationSeconds)
	assert.NilError(t, err)

	_, err = dao.SetIntSetting(env.ctx, &env.api, env.DAO, "seeds_deferral_factor_x100", env.SeedsDeferralFactor)
	assert.NilError(t, err)

	_, err = dao.SetIntSetting(env.ctx, &env.api, env.DAO, "hypha_deferral_factor_x100", env.HyphaDeferralFactor)
	assert.NilError(t, err)

	_, err = dao.SetIntSetting(env.ctx, &env.api, env.DAO, "paused", 0)
	assert.NilError(t, err)

	dao.SetNameSetting(env.ctx, &env.api, env.DAO, "hypha_token_contract", env.HyphaToken)
	dao.SetNameSetting(env.ctx, &env.api, env.DAO, "hvoice_token_contract", env.HvoiceToken)
	dao.SetNameSetting(env.ctx, &env.api, env.DAO, "husd_token_contract", env.HusdToken)
	dao.SetNameSetting(env.ctx, &env.api, env.DAO, "seeds_token_contract", env.SeedsToken)
	dao.SetNameSetting(env.ctx, &env.api, env.DAO, "seeds_escrow_contract", env.SeedsEscrow)
	dao.SetNameSetting(env.ctx, &env.api, env.DAO, "publisher_contract", env.Events)
	dao.SetNameSetting(env.ctx, &env.api, env.DAO, "treasury_contract", env.Bank)
	dao.SetNameSetting(env.ctx, &env.api, env.DAO, "telos_decide_contract", env.TelosDecide)
	dao.SetNameSetting(env.ctx, &env.api, env.DAO, "last_ballot_id", "hypha......1")

	if addFakePeriods {
		t.Log("Adding "+strconv.Itoa(env.NumPeriods)+" periods with duration 		: ", env.PeriodDuration)
		env.Periods, err = dao.AddPeriods(env.ctx, &env.api, env.DAO, env.Root.Hash, env.NumPeriods, env.PeriodDuration)
		assert.NilError(t, err)
	}

	// setup TLOS system contract
	_, tlosToken, err := eostest.CreateAccountWithRandomKey(env.ctx, &env.api, "eosio.token")
	assert.NilError(t, err)

	tlosMaxSupply, _ := eos.NewAssetFromString("1000000000.0000 TLOS")
	_, err = eostest.DeployAndCreateToken(env.ctx, t, &env.api, artifactsHome, tlosToken, env.DAO, tlosMaxSupply)
	assert.NilError(t, err)

	_, err = dao.Issue(env.ctx, &env.api, tlosToken, env.DAO, tlosMaxSupply)
	assert.NilError(t, err)

	// deploy TD contract
	t.Log("Deploying/configuring Telos Decide contract 		: ", env.TelosDecide)
	_, err = eostest.SetContract(env.ctx, &env.api, env.TelosDecide, decidePrefix+"wasm", decidePrefix+"abi")
	assert.NilError(t, err)

	// call init action
	_, err = dao.InitTD(env.ctx, &env.api, env.TelosDecide)
	assert.NilError(t, err)

	// transfer
	_, err = dao.Transfer(env.ctx, &env.api, tlosToken, env.DAO, env.TelosDecide, tlosMaxSupply, "deposit")
	assert.NilError(t, err)

	_, err = dao.NewTreasury(env.ctx, &env.api, env.TelosDecide, env.DAO)
	assert.NilError(t, err)

	_, err = dao.RegVoter(env.ctx, &env.api, env.TelosDecide, env.DAO)
	assert.NilError(t, err)

	if addFakeMembers {
		daoTokens, _ := eos.NewAssetFromString("1.00 HVOICE")
		_, err = dao.Mint(env.ctx, &env.api, env.TelosDecide, env.DAO, env.DAO, daoTokens)
		assert.NilError(t, err)

		aliceTokens, _ := eos.NewAssetFromString("100.00 HVOICE")
		env.Alice, err = SetupMember(t, env.ctx, &env.api, env.DAO, env.TelosDecide, env.HvoiceToken, "alice", aliceTokens)
		assert.NilError(t, err)

		index := 1
		for index < 5 {

			memberNameIn := "mem" + strconv.Itoa(index) + ".hypha"

			newMember, err := SetupMember(t, env.ctx, &env.api, env.DAO, env.TelosDecide, env.HvoiceToken, memberNameIn, daoTokens)
			assert.NilError(t, err)

			env.Members = append(env.Members, newMember)
			index++
		}
	}

	env.GenesisHVOICE = 200

	return &env
}

func SetupMember(t *testing.T, ctx context.Context, api *eos.API,
	contract, telosDecide, hyphaVoice eos.AccountName, memberName string, hvoice eos.Asset) (Member, error) {

	t.Log("Creating and enrolling new member  		: ", memberName, " 	with voting power	: ", hvoice.String())
	memberAccount, err := eostest.CreateAccountFromString(ctx, api, memberName, eostest.DefaultKey())
	assert.NilError(t, err)

	_, err = dao.RegVoter(ctx, api, telosDecide, memberAccount)
	assert.NilError(t, err)

	_, err = dao.Mint(ctx, api, telosDecide, contract, memberAccount, hvoice)
	assert.NilError(t, err)

	_, err = dao.IssueHVoice(ctx, api, hyphaVoice, contract, contract, hvoice)
	assert.NilError(t, err)
	_, err = dao.TransferHVoice(ctx, api, hyphaVoice, contract, contract, memberAccount, hvoice)
	assert.NilError(t, err)

	_, err = dao.Apply(ctx, api, contract, memberAccount, "apply to DAO")
	assert.NilError(t, err)

	_, err = dao.Enroll(ctx, api, contract, contract, memberAccount)
	assert.NilError(t, err)

	pause(t, time.Second, "Build block...", "")

	memberDoc, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, "member")
	assert.NilError(t, err)

	memberNameFV, err := memberDoc.GetContent("member")
	assert.NilError(t, err)
	assert.Equal(t, eos.AN(string(memberNameFV.Impl.(eos.Name))), memberAccount)

	return Member{
		Member: memberAccount,
		Doc:    memberDoc,
	}, nil
}

func CreateHVoiceToken(ctx context.Context, t *testing.T, api *eos.API, contract, issuer eos.AccountName, maxSupply eos.Asset, decayPeriod eos.Uint64, decayPerPeriodX10M eos.Uint64) (string, error) {
	type tokenCreate struct {
		Issuer              eos.AccountName
		MaxSupply           eos.Asset
		DecayPeriod         eos.Uint64
		DecayPerPeriodX10M  eos.Uint64
	}

	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("create"),
		Authorization: []eos.PermissionLevel{
			{Actor: contract, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(tokenCreate{
			Issuer:                issuer,
			MaxSupply:             maxSupply,
			DecayPeriod:           decayPeriod,
			DecayPerPeriodX10M:    decayPerPeriodX10M,
		}),
	}}

	t.Log("Created Token : ", contract, " 		: ", maxSupply.String(), " (-1 means unlimited)")
	return eostest.ExecWithRetry(ctx, api, actions)
}

func SaveGraph(ctx context.Context, api *eos.API, contract eos.AccountName, folderName string) error {

	var request eos.GetTableRowsRequest
	request.Code = string(contract)
	request.Scope = string(contract)
	request.Table = "documents"
	request.Limit = 1000
	request.JSON = true
	response, err := api.GetTableRows(ctx, request)
	if err != nil {
		return fmt.Errorf("Unable to retrieve rows: %v", err)
	}

	data, err := response.Rows.MarshalJSON()
	if err != nil {
		return fmt.Errorf("Unable to marshal json: %v", err)
	}

	documentsFile := folderName + "/documents.json"
	err = ioutil.WriteFile(documentsFile, data, 0644)
	if err != nil {
		return fmt.Errorf("Unable to write file: %v", err)
	}

	request = eos.GetTableRowsRequest{}
	request.Code = string(contract)
	request.Scope = string(contract)
	request.Table = "edges"
	request.Limit = 1000
	request.JSON = true
	response, err = api.GetTableRows(ctx, request)
	if err != nil {
		return fmt.Errorf("Unable to retrieve rows: %v", err)
	}

	data, err = response.Rows.MarshalJSON()
	if err != nil {
		return fmt.Errorf("Unable to marshal json: %v", err)
	}

	edgesFile := folderName + "/edges.json"
	err = ioutil.WriteFile(edgesFile, data, 0644)
	if err != nil {
		return fmt.Errorf("Unable to write file: %v", err)
	}

	return nil
}

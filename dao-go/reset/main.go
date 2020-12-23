package main

import (
	"context"
	"encoding/json"
	"fmt"
	"strconv"
	"testing"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"github.com/k0kubun/go-ansi"
	"github.com/schollz/progressbar/v3"
	"github.com/spf13/viper"
)

type notes struct {
	Notes string `json:"notes"`
}

func main() {

	ctx := context.Background()
	keyBag := &eos.KeyBag{}

	api := eos.New("https://testnet.telos.caleos.io")
	contract := eos.AccountName("dao1.hypha")
	err := keyBag.ImportPrivateKey(ctx, "5KCZ9VBJMMiLaAY24Ro66mhx4vU1VcJELZVGrJbkUBATyqxyYmj")

	// api := eos.New("http://localhost:8888")
	// contract := eos.AccountName("dao.hypha")
	// err := keyBag.ImportPrivateKey(ctx, eostest.DefaultKey())

	if err != nil {
		panic(err)
	}
	api.SetSigner(keyBag)

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
	trxID, err := eostest.ExecTrx(ctx, api, actions)
	if err != nil {
		panic(err)
	}

	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"paused", "value":["int64","0"]}' -p dao1.hypha
	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"seeds_token_contract", "value":["name","token.seeds"]}' -p dao1.hypha
	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"voting_duration_sec", "value":["int64",600]}' -p dao1.hypha
	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"seeds_deferral_factor_x100", "value":["int64","100"]}' -p dao1.hypha
	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"telos_decide_contract", "value":["name","td1.hypha"]}' -p dao1.hypha
	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"husd_token_contract", "value":["name","husd.hypha"]}' -p dao1.hypha
	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"hypha_token_contract", "value":["name","token1.hypha"]}' -p dao1.hypha
	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"seeds_escrow_contract", "value":["name","escro1.hypha"]}' -p dao1.hypha
	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"publisher_contract", "value":["name","publs11.hypha"]}' -p dao1.hypha
	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"treasury_contract", "value":["name","bank1.hypha"]}' -p dao1.hypha
	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"last_ballot_id", "value":["name","hypha1.....1"]}' -p dao1.hypha
	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"hypha_deferral_factor_x100", "value":["int64","25"]}' -p dao1.hypha
	// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao1.hypha setsetting '{"key":"seeds_deferral_factor_x100", "value":["int64","100"]}' -p dao1.hypha

	fmt.Println("Completed the reset4test transaction: " + trxID)
	fmt.Println()

	// re-enroll members
	index := 1
	for index < 6 {

		memberNameIn := "mem" + strconv.Itoa(index) + ".hypha"
		//memberNameIn := "member" + strconv.Itoa(index)

		newMember, err := enrollMember(ctx, api, contract, eos.AN(memberNameIn))
		if err != nil {
			panic(err)
		}
		fmt.Println("Member enrolled : " + string(memberNameIn) + " with hash: " + newMember.Hash.String())
		fmt.Println()

		index++
	}

	copyPeriodsFromProd(api, contract)

	// roleFilename := "~/dev/hypha/daoctl/testing/role.json"
	// data, err := ioutil.ReadFile(roleFilename)
	// if err != nil {
	// 	fmt.Println("Unable to read file: ", roleFilename)
	// 	return
	// }

	// createRole(ctx, api, contract, contract, data)
}

func getProdPeriods() []dao.Period {
	endpointAPI := *eos.New("https://api.telos.kitchen")

	var objects []dao.Period
	var request eos.GetTableRowsRequest
	request.Code = "dao.hypha"
	request.Scope = "dao.hypha"
	request.Table = "periods"
	request.Limit = 10000
	request.JSON = true
	response, _ := endpointAPI.GetTableRows(context.Background(), request)
	response.JSONToStructs(&objects)
	return objects
}

type addPeriodBTS struct {
	Predecessor eos.Checksum256 `json:"predecessor"`
	StartTime   eos.TimePoint   `json:"start_time"`
	Label       string          `json:"label"`
}

// type addPeriod2 struct {
// 	Predecessor eos.Checksum256 `json:"predecessor"`
// 	StartTime   eos.TimePoint   `json:"start_time"`
// 	Label       string          `json:"label"`
// }

func copyPeriodsFromProd(api *eos.API, contract eos.AccountName) {

	periods := getProdPeriods()

	rootDoc, err := docgraph.LoadDocument(context.Background(), api, contract, "0f374e7a9d8ab17f172f8c478744cdd4016497e15229616f2ffd04d8002ef64a")
	if err != nil {
		panic(err)
	}

	predecessor := rootDoc.Hash
	var lastPeriod docgraph.Document

	for _, period := range periods {

		seconds := period.StartTime
		// fmt.Println("seconds (nano)		: ", strconv.Itoa(int(seconds.UnixNano())))

		microSeconds := seconds.UnixNano() / 1000
		// fmt.Println("seconds (micro)	: ", strconv.Itoa(int(microSeconds)))

		startTime := eos.TimePoint(microSeconds)

		addPeriodAction := eos.Action{
			Account: contract,
			Name:    eos.ActN("addperiod"),
			Authorization: []eos.PermissionLevel{
				{Actor: contract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(addPeriodBTS{
				Predecessor: predecessor,
				StartTime:   startTime,
				Label:       period.Phase,
			}),
		}

		trxID, err := eostest.ExecTrx(context.Background(), api, []*eos.Action{&addPeriodAction})
		if err != nil {
			fmt.Printf("Error adding period: %v", err)
		} else {
			fmt.Println(" Added period: " + trxID)
			t := testing.T{}
			pause(&t, time.Second, "Build block...", "")

			lastPeriod, err = docgraph.GetLastDocument(context.Background(), api, contract)
			if err != nil {
				panic(err)
			}
		}

		predecessor = lastPeriod.Hash
	}
}

func enrollMember(ctx context.Context, api *eos.API, contract, member eos.AccountName) (docgraph.Document, error) {
	fmt.Println("Enrolling " + member + " in DAO: " + contract)

	trxID, err := dao.Apply(ctx, api, contract, member, "apply to DAO")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error applying %v", err)
	}
	fmt.Println("Completed the apply transaction: " + trxID)

	trxID, err = dao.Enroll(ctx, api, contract, contract, member)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error enrolling %v", err)
	}
	fmt.Println("Completed the enroll transaction: " + trxID)

	t := testing.T{}
	pause(&t, time.Second, "Build block...", "")

	memberDoc, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, "member")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error enrolling %v", err)
	}

	return memberDoc, nil
}

func pause(t *testing.T, seconds time.Duration, headline, prefix string) {
	if headline != "" {
		t.Log(headline)
	}

	bar := progressbar.NewOptions(100,
		progressbar.OptionSetWriter(ansi.NewAnsiStdout()),
		progressbar.OptionEnableColorCodes(true),
		progressbar.OptionSetWidth(90),
		// progressbar.OptionShowIts(),
		progressbar.OptionSetDescription("[cyan]"+fmt.Sprintf("%20v", prefix)),
		progressbar.OptionSetTheme(progressbar.Theme{
			Saucer:        "[green]=[reset]",
			SaucerHead:    "[green]>[reset]",
			SaucerPadding: " ",
			BarStart:      "[",
			BarEnd:        "]",
		}))

	chunk := seconds / 100
	for i := 0; i < 100; i++ {
		bar.Add(1)
		time.Sleep(chunk)
	}
	fmt.Println()
	fmt.Println()
}

func getDefaultPeriod(api *eos.API, contract eos.AccountName) docgraph.Document {

	ctx := context.Background()

	root, err := docgraph.LoadDocument(ctx, api, contract, "0f374e7a9d8ab17f172f8c478744cdd4016497e15229616f2ffd04d8002ef64a")
	if err != nil {
		panic(err)
	}

	edges, err := docgraph.GetEdgesFromDocumentWithEdge(ctx, api, contract, root, eos.Name("start"))
	if err != nil || len(edges) <= 0 {
		panic("There are no next edges")
	}

	lastDocument, err := docgraph.LoadDocument(ctx, api, contract, edges[0].ToNode.String())
	if err != nil {
		panic("Next document not found: " + edges[0].ToNode.String())
	}

	index := 1
	for index < 6 {

		edges, err := docgraph.GetEdgesFromDocumentWithEdge(ctx, api, contract, lastDocument, eos.Name("next"))
		if err != nil || len(edges) <= 0 {
			panic("There are no next edges")
		}

		lastDocument, err = docgraph.LoadDocument(ctx, api, contract, edges[0].ToNode.String())
		if err != nil {
			panic("Next document not found: " + edges[0].ToNode.String())
		}

		index++
	}
	return lastDocument
}

type proposal struct {
	Proposer      eos.AccountName         `json:"proposer"`
	ProposalType  eos.Name                `json:"proposal_type"`
	ContentGroups []docgraph.ContentGroup `json:"content_groups"`
}

func createRole(ctx context.Context, api *eos.API, contract, member eos.AccountName, data []byte) (docgraph.Document, error) {

	var proposalDoc docgraph.Document
	err := json.Unmarshal([]byte(data), &proposalDoc)
	if err != nil {
		panic(err)
	}

	action := eos.ActN("propose")
	actions := []*eos.Action{{
		Account: contract,
		Name:    action,
		Authorization: []eos.PermissionLevel{
			{Actor: eos.AN(viper.GetString("DAOUser")), Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(proposal{
			Proposer:      eos.AN(viper.GetString("DAOUser")),
			ProposalType:  eos.Name("role"),
			ContentGroups: proposalDoc.ContentGroups,
		})}}

	trxID, err := eostest.ExecTrx(ctx, api, actions)
	if err != nil {
		panic(err)
	}
	fmt.Println("Proposed: Transaction ID: " + trxID)

	// retrieve the document we just created
	role, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, eos.Name("proposal"))
	if err != nil {
		panic(err)
	}

	ballot, err := role.GetContent("ballot_id")
	if err != nil {
		panic(err)
	}

	index := 1
	for index < 6 {

		memberNameIn := "mem" + strconv.Itoa(index) + ".hypha"
		//memberNameIn := "member" + strconv.Itoa(index)

		_, err := dao.TelosDecideVote(ctx, api, eos.AN("td1.hypha"), eos.AN(memberNameIn), ballot.Impl.(eos.Name), eos.Name("pass"))
		if err != nil {
			panic(err)
		}
		fmt.Println("Member voted : " + string(memberNameIn))
		fmt.Println()

		index++
	}

	t := testing.T{}
	pause(&t, time.Second*605, "Build block...", "")

	_, err = dao.CloseProposal(ctx, api, contract, member, role.Hash)
	if err != nil {
		panic(err)
	}
	return role, nil
}

func createAssignment(ctx context.Context, api *eos.API, contract, member eos.AccountName, data []byte) (docgraph.Document, error) {
	var proposalDoc docgraph.Document
	err := json.Unmarshal([]byte(data), &proposalDoc)
	if err != nil {
		panic(err)
	}

	var role docgraph.Document
	role, err = docgraph.GetLastDocumentOfEdge(ctx, api, contract, eos.Name("role"))
	if err != nil {
		panic(err)
	}

	// inject the role hash in the first content group of the document
	proposalDoc.ContentGroups[0] = append(proposalDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "role",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("checksum256"),
				Impl:   role.Hash,
			}},
	})

	// inject the period hash in the first content group of the document
	proposalDoc.ContentGroups[0] = append(proposalDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "start_period",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("checksum256"),
				Impl:   getDefaultPeriod(api, contract).Hash,
			}},
	})

	// inject the assignee in the first content group of the document
	proposalDoc.ContentGroups[0] = append(proposalDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "assignee",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("name"),
				Impl:   eos.Name(viper.GetString("DAOUser")),
			}},
	})

	action := eos.ActN("propose")
	actions := []*eos.Action{{
		Account: contract,
		Name:    action,
		Authorization: []eos.PermissionLevel{
			{Actor: eos.AN(viper.GetString("DAOUser")), Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(proposal{
			Proposer:      eos.AN(viper.GetString("DAOUser")),
			ProposalType:  eos.Name("assignment"),
			ContentGroups: proposalDoc.ContentGroups,
		})}}

	trxID, err := eostest.ExecTrx(ctx, api, actions)
	if err != nil {
		panic(err)
	}
	fmt.Println("Proposed: Transaction ID: " + trxID)

	// retrieve the document we just created
	assignment, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, eos.Name("proposal"))
	if err != nil {
		panic(err)
	}

	ballot, err := assignment.GetContent("ballot_id")
	if err != nil {
		panic(err)
	}

	index := 1
	for index < 6 {

		memberNameIn := "mem" + strconv.Itoa(index) + ".hypha"
		//memberNameIn := "member" + strconv.Itoa(index)

		_, err := dao.TelosDecideVote(ctx, api, eos.AN("td1.hypha"), eos.AN(memberNameIn), ballot.Impl.(eos.Name), eos.Name("pass"))
		if err != nil {
			panic(err)
		}
		fmt.Println("Member voted : " + string(memberNameIn))
		fmt.Println()

		index++
	}

	t := testing.T{}
	pause(&t, time.Second*605, "Build block...", "")

	fmt.Println("Member: ", member, " is closing assignment proposal	: ", assignment.Hash.String())
	_, err = dao.CloseProposal(ctx, api, contract, member, role.Hash)
	if err != nil {
		panic(err)
	}
	return assignment, nil
}

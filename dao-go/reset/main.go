package main

import (
	"context"
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
)

type notes struct {
	Notes string `json:"notes"`
}

func main() {

	ctx := context.Background()
	keyBag := &eos.KeyBag{}

	api := eos.New("https://test.telos.kitchen")
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

	copyPeriods(*api, "https://api.telos.kitchen", contract)
}

func getPeriods(endpoint string) []dao.Period {
	endpointAPI := *eos.New(endpoint)

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

type addPeriod2 struct {
	Predecessor eos.Checksum256 `json:"predecessor"`
	StartTime   eos.TimePoint   `json:"start_time"`
	Label       string          `json:"label"`
}

func copyPeriods(api eos.API, from string, contract eos.AccountName) {

	periods := getPeriods(from)

	rootDoc, err := docgraph.LoadDocument(context.Background(), &api, contract, "cb7574fd1cfbdcbede5c8d7860ae5772a69785211cd56a52cb31e7ed492d60fb")
	if err != nil {
		panic(err)
	}

	predecessor := rootDoc.Hash

	for _, period := range periods {

		addPeriodAction := eos.Action{
			Account: contract,
			Name:    eos.ActN("addperiod"),
			Authorization: []eos.PermissionLevel{
				{Actor: contract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(addPeriod2{
				Predecessor: predecessor,
				StartTime:   period.StartTime,
				Label:       period.Phase,
			}),
		}

		trxID, err := eostest.ExecTrx(context.Background(), &api, []*eos.Action{&addPeriodAction})
		fmt.Println(" Added period: " + trxID)

		lastPeriod, err := docgraph.GetLastDocument(context.Background(), &api, contract)
		if err != nil {
			panic(err)
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

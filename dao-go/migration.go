package dao

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

// MemberRecord represents a single row in the dao::members table
type MemberRecord struct {
	MemberName eos.Name `json:"member"`
}

// MigrateMembers ...
func MigrateMembers(ctx context.Context, api *eos.API, contract eos.AccountName, from string) {

	endpointAPI := *eos.New(from)

	var memberRecords []MemberRecord
	var request eos.GetTableRowsRequest
	request.Code = "dao.hypha"
	request.Scope = "dao.hypha"
	request.Table = "members"
	request.Limit = 500
	request.JSON = true
	response, _ := endpointAPI.GetTableRows(ctx, request)
	response.JSONToStructs(&memberRecords)

	for index, memberRecord := range memberRecords {
		actions := []*eos.Action{{
			Account: contract,
			Name:    eos.ActN("addmember"),
			Authorization: []eos.PermissionLevel{
				{Actor: contract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(memberRecord),
		}, {
			Account: contract,
			Name:    eos.ActN("migratemem"),
			Authorization: []eos.PermissionLevel{
				{Actor: contract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(memberRecord),
		}}

		trxID, err := eostest.ExecTrx(ctx, api, actions)
		if err != nil {
			panic(err)
		}

		fmt.Println("Added & migrated member: " + string(memberRecord.MemberName) + ", " + strconv.Itoa(index+1) + " / " + strconv.Itoa(len(memberRecords)) + " : trxID:  " + trxID)
	}
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

// MigratePeriods ...
func MigratePeriods(api *eos.API, predecessor eos.Checksum256, contract eos.AccountName) {

	periods := getProdPeriods()

	var lastPeriod docgraph.Document

	for _, period := range periods {

		seconds := period.StartTime
		microSeconds := seconds.UnixNano() / 1000
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

// MigrateObject ...
type MigrateObject struct {
	Scope eos.Name `json:"scope"`
	ID    uint64   `json:"id"`
}

func getObject(ctx context.Context, scope eos.Name, endpoint string, ID uint64) Object {
	endpointAPI := *eos.New(endpoint)

	var objects []Object
	var request eos.GetTableRowsRequest
	request.Code = "dao.hypha"
	request.Scope = string(scope)
	request.Table = "objects"
	request.LowerBound = strconv.Itoa(int(ID))
	request.UpperBound = strconv.Itoa(int(ID))
	request.Limit = 1
	request.JSON = true
	response, _ := endpointAPI.GetTableRows(ctx, request)
	response.JSONToStructs(&objects)
	return objects[0]
}

// GetObjects ...
func GetObjects(ctx context.Context, contract eos.AccountName, scope eos.Name, endpoint string) []Object {
	endpointAPI := *eos.New(endpoint)

	var objects []Object
	var request eos.GetTableRowsRequest
	request.Code = "dao.hypha"
	request.Scope = string(scope)
	request.Table = "objects"
	request.Limit = 10000
	request.JSON = true
	response, _ := endpointAPI.GetTableRows(ctx, request)
	response.JSONToStructs(&objects)
	return objects
}

// CopyObjects ...
func CopyObjects(ctx context.Context, api *eos.API, contract eos.AccountName, scope eos.Name, from string) {

	objects := GetObjects(ctx, contract, scope, from)

	for _, object := range objects {
		object.Scope = eos.Name(scope)

		actions := []*eos.Action{{
			Account: contract,
			Name:    eos.ActN("createobj"),
			Authorization: []eos.PermissionLevel{
				{Actor: contract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(object),
		}}

		trxID, err := eostest.ExecTrx(ctx, api, actions)
		if err != nil {
			panic(err)
		}
		fmt.Println("Created an object- trxID:  " + trxID)
	}
}

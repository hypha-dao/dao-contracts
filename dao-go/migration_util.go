package dao

import (
	"context"
	"fmt"
	"strconv"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"github.com/schollz/progressbar/v3"
)

func defaultPause() time.Duration {
	return time.Millisecond * 250
}

func getAssPayoutRange(ctx context.Context, api *eos.API, contract eos.AccountName, id, count int) ([]assPayoutIn, bool, error) {
	var records []assPayoutIn
	var request eos.GetTableRowsRequest
	request.LowerBound = strconv.Itoa(id)
	request.Code = string(contract)
	request.Scope = string(contract)
	request.Table = "asspayouts"
	request.Limit = uint32(count)
	request.JSON = true
	response, err := api.GetTableRows(ctx, request)
	if err != nil {
		return []assPayoutIn{}, false, fmt.Errorf("get table rows %v", err)
	}

	err = response.JSONToStructs(&records)
	if err != nil {
		return []assPayoutIn{}, false, fmt.Errorf("json to structs %v", err)
	}
	return records, response.More, nil
}

func getAllAssPayouts(ctx context.Context, api *eos.API, contract eos.AccountName) ([]assPayoutIn, error) {

	var allPayouts []assPayoutIn

	cursor := 0
	batchSize := 100

	batch, more, err := getAssPayoutRange(ctx, api, contract, cursor, batchSize)
	if err != nil {
		return []assPayoutIn{}, fmt.Errorf("json to structs %v", err)
	}
	allPayouts = append(allPayouts, batch...)

	for more {
		cursor += batchSize
		batch, more, err = getAssPayoutRange(ctx, api, contract, cursor, batchSize)
		if err != nil {
			return []assPayoutIn{}, fmt.Errorf("json to structs %v", err)
		}
		allPayouts = append(allPayouts, batch...)
	}

	return allPayouts, nil
}

func getLegacyPeriods(ctx context.Context, api *eos.API, contract eos.AccountName) []Period {

	var periods []Period
	var request eos.GetTableRowsRequest
	request.Code = string(contract)
	request.Scope = string(contract)
	request.Table = "periods"
	request.Limit = 1000
	request.JSON = true
	response, _ := api.GetTableRows(ctx, request)
	response.JSONToStructs(&periods)
	return periods
}

// DefaultProgressBar ...
func DefaultProgressBar(counter int) *progressbar.ProgressBar {
	return progressbar.Default(int64(counter))
	// ,
	// 	progressbar.OptionSetWriter(ansi.NewAnsiStdout()),
	// 	progressbar.OptionEnableColorCodes(true),
	// 	progressbar.OptionSetWidth(90),
	// 	// progressbar.OptionShowIts(),
	// 	progressbar.OptionSetDescription("[cyan]"+fmt.Sprintf("%20v", "")),
	// 	progressbar.OptionSetTheme(progressbar.Theme{
	// 		Saucer:        "[green]=[reset]",
	// 		SaucerHead:    "[green]>[reset]",
	// 		SaucerPadding: " ",
	// 		BarStart:      "[",
	// 		BarEnd:        "]",
	// 	}))
}

func pause(seconds time.Duration, headline, prefix string) {
	if headline != "" {
		fmt.Println(headline)
	}

	bar := DefaultProgressBar(100)

	chunk := seconds / 100
	for i := 0; i < 100; i++ {
		bar.Add(1)
		time.Sleep(chunk)
	}
	fmt.Println()
}

// GetObjects ...
func getLegacyObjects(ctx context.Context, api *eos.API, contract eos.AccountName, scope eos.Name) []Object {

	var objects []Object
	var request eos.GetTableRowsRequest
	request.Code = string(contract)
	request.Scope = string(scope)
	request.Table = "objects"
	request.Limit = 10000
	request.JSON = true
	response, _ := api.GetTableRows(ctx, request)
	response.JSONToStructs(&objects)
	return objects
}

type memberRecord struct {
	MemberName eos.Name `json:"member"`
}

func getLegacyMembers(ctx context.Context, api *eos.API, contract eos.AccountName) []memberRecord {
	var memberRecords []memberRecord
	var request eos.GetTableRowsRequest
	request.Code = string(contract)
	request.Scope = string(contract)
	request.Table = "members"
	request.Limit = 500
	request.JSON = true
	response, _ := api.GetTableRows(ctx, request)
	response.JSONToStructs(&memberRecords)
	return memberRecords
}

type eraseDoc struct {
	Hash eos.Checksum256 `json:"hash"`
}

// EraseAllDocuments ...
func EraseAllDocuments(ctx context.Context, api *eos.API, contract eos.AccountName) {

	documents, err := docgraph.GetAllDocuments(ctx, api, contract)
	if err != nil {
		fmt.Println(err)
	}

	fmt.Println("\nErasing documents: " + strconv.Itoa(len(documents)))
	bar := DefaultProgressBar(len(documents))

	for _, document := range documents {

		typeFV, err := document.GetContent("type")
		docType := typeFV.Impl.(eos.Name)

		if err != nil ||
			docType == eos.Name("settings") ||
			docType == eos.Name("dho") {
			// do not erase
		} else {
			actions := []*eos.Action{{
				Account: contract,
				Name:    eos.ActN("erasedoc"),
				Authorization: []eos.PermissionLevel{
					{Actor: contract, Permission: eos.PN("active")},
				},
				ActionData: eos.NewActionData(eraseDoc{
					Hash: document.Hash,
				}),
			}}

			_, err := eostest.ExecTrx(ctx, api, actions)
			if err != nil {
				// too many false positives
				// fmt.Println("\nFailed to erase : ", document.Hash.String())
				// fmt.Println(err)
			}
			time.Sleep(defaultPause())
		}
		bar.Add(1)
	}
}

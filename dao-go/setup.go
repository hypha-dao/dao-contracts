package dao

import (
	"context"
	"fmt"
	"strconv"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
)

type assPayoutIn struct {
	ID           uint64             `json:"ass_payment_id"`
	AssignmentID uint64             `json:"assignment_id"`
	Recipient    eos.Name           `json:"recipient"`
	PeriodID     uint64             `json:"period_id"`
	Payments     []eos.Asset        `json:"payments"`
	PaymentDate  eos.BlockTimestamp `json:"payment_date"`
}

type assPayoutOut struct {
	ID           uint64        `json:"ass_payment_id"`
	AssignmentID uint64        `json:"assignment_id"`
	Recipient    eos.Name      `json:"recipient"`
	PeriodID     uint64        `json:"period_id"`
	Payments     []eos.Asset   `json:"payments"`
	PaymentDate  eos.TimePoint `json:"payment_date"`
}

func payoutExists(ctx context.Context, api *eos.API, contract eos.AccountName, id int) bool {
	var records []assPayoutIn
	var request eos.GetTableRowsRequest
	request.Code = string(contract)
	request.Scope = string(contract)
	request.LowerBound = strconv.Itoa(id)
	request.UpperBound = strconv.Itoa(id)
	request.Table = "asspayouts"
	request.Limit = 1
	request.JSON = true
	response, err := api.GetTableRows(ctx, request)
	if err != nil {
		return false
	}

	err = response.JSONToStructs(&records)
	if err != nil {
		return false
	}

	if len(records) > 0 {
		return true
	}
	return false
}

// CopyAssPayouts ...
func CopyAssPayouts(ctx context.Context, api *eos.API, contract eos.AccountName, from string) {

	sourceAPI := *eos.New(from)

	payoutsIn, err := getAllAssPayouts(ctx, &sourceAPI, contract)
	if err != nil {
		panic(err)
	}

	fmt.Println("\nCopying " + strconv.Itoa(len(payoutsIn)) + " assignment payout records from " + from)
	bar := DefaultProgressBar(len(payoutsIn))

	for _, payoutIn := range payoutsIn {

		paymentDate := eos.TimePoint(payoutIn.PaymentDate.UnixNano() / 1000)

		if !payoutExists(ctx, api, contract, int(payoutIn.ID)) {
			actions := []*eos.Action{{
				Account: contract,
				Name:    eos.ActN("addasspayout"),
				Authorization: []eos.PermissionLevel{
					{Actor: contract, Permission: eos.PN("active")},
				},
				ActionData: eos.NewActionData(assPayoutOut{
					ID:           payoutIn.ID,
					AssignmentID: payoutIn.AssignmentID,
					Recipient:    payoutIn.Recipient,
					PeriodID:     payoutIn.PeriodID,
					Payments:     payoutIn.Payments,
					PaymentDate:  paymentDate,
				}),
			}}

			_, err := eostest.ExecTrx(ctx, api, actions)
			if err != nil {
				panic(err)
			}

			bar.Add(1)
			time.Sleep(defaultPause())
			// pause(defaultPause(), "Build blocks...", "")

			// fmt.Println("Added assignment pay: ", payoutIn.PaymentDate.Format("2006 Jan 02"), ", ", strconv.Itoa(index)+" / "+strconv.Itoa(len(payoutsIn))+" : trxID:  "+trxID)
		}
	}
}

type addLegPer struct {
	ID        uint64        `json:"id"`
	StartTime eos.TimePoint `json:"start_date"`
	EndTime   eos.TimePoint `json:"end_date"`
	Phase     string        `json:"phase"`
	Readable  string        `json:"readable"`
	NodeLabel string        `json:"label"`
}

func exists(ctx context.Context, api *eos.API, contract eos.AccountName, table, scope eos.Name, id int) bool {
	var records []interface{}
	var request eos.GetTableRowsRequest
	request.Code = string(contract)
	request.Scope = string(contract)
	request.LowerBound = strconv.Itoa(id)
	request.UpperBound = strconv.Itoa(id)
	request.Table = string(table)
	request.Limit = 1
	request.JSON = true
	response, err := api.GetTableRows(ctx, request)
	if err != nil {
		return false
	}

	err = response.JSONToStructs(&records)
	if err != nil {
		return false
	}

	if len(records) > 0 {
		return true
	}
	return false
}

func periodExists(ctx context.Context, api *eos.API, contract eos.AccountName, id int) bool {
	var records []Period
	var request eos.GetTableRowsRequest
	request.Code = string(contract)
	request.Scope = string(contract)
	request.LowerBound = strconv.Itoa(id)
	request.UpperBound = strconv.Itoa(id)
	request.Table = "periods"
	request.Limit = 1
	request.JSON = true
	response, err := api.GetTableRows(ctx, request)
	if err != nil {
		return false
	}

	err = response.JSONToStructs(&records)
	if err != nil {
		return false
	}

	if len(records) > 0 {
		return true
	}
	return false
}

// CopyPeriods ...
func CopyPeriods(ctx context.Context, api *eos.API, contract eos.AccountName, from string) {

	sourceAPI := *eos.New(from)
	periods := getLegacyPeriods(ctx, &sourceAPI, contract)

	fmt.Println("\nCopying " + strconv.Itoa(len(periods)) + " periods from " + from)

	bar := DefaultProgressBar(len(periods))

	for _, period := range periods {

		if !periodExists(ctx, api, contract, int(period.PeriodID)) {

			startTime := eos.TimePoint(period.StartTime.UnixNano() / 1000)
			endTime := eos.TimePoint(period.EndTime.UnixNano() / 1000)

			actions := []*eos.Action{{
				Account: contract,
				Name:    eos.ActN("addlegper"),
				Authorization: []eos.PermissionLevel{
					{Actor: contract, Permission: eos.PN("active")},
				},
				ActionData: eos.NewActionData(addLegPer{
					ID:        period.PeriodID,
					StartTime: startTime,
					EndTime:   endTime,
					Phase:     period.Phase,
					Readable:  period.StartTime.Time.Format("2006 Jan 02 15:04:05 MST"),
					NodeLabel: "Starting " + period.StartTime.Time.Format("2006 Jan 02"),
				}),
			}}

			_, err := eostest.ExecTrx(ctx, api, actions)
			if err != nil {
				panic(err)
			}

			bar.Add(1)
			time.Sleep(defaultPause())
		}
	}
}

// CopyObjects ...
func CopyObjects(ctx context.Context, api *eos.API, contract eos.AccountName, scope eos.Name, from string) {

	sourceAPI := *eos.New(from)
	objects := getLegacyObjects(ctx, &sourceAPI, contract, scope)

	fmt.Println("\nCopying " + strconv.Itoa(len(objects)) + " " + string(scope) + " objects from " + from)
	bar := DefaultProgressBar(len(objects))

	for _, object := range objects {

		if !exists(ctx, api, contract, eos.Name("objects"), scope, int(object.ID)) {

			object.Scope = eos.Name(scope)

			actions := []*eos.Action{{
				Account: contract,
				Name:    eos.ActN("createobj"),
				Authorization: []eos.PermissionLevel{
					{Actor: contract, Permission: eos.PN("active")},
				},
				ActionData: eos.NewActionData(object),
			}}

			_, err := eostest.ExecTrx(ctx, api, actions)
			if err != nil {
				panic(err)
			}

			bar.Add(1)
			time.Sleep(defaultPause())
		}
	}
}

func memberExists(ctx context.Context, api *eos.API, contract eos.AccountName, member eos.Name) bool {
	var records []memberRecord
	var request eos.GetTableRowsRequest
	request.Code = string(contract)
	request.Scope = string(contract)
	request.LowerBound = string(member)
	request.UpperBound = string(member)
	request.Table = "members"
	request.Limit = 1
	request.JSON = true
	response, err := api.GetTableRows(ctx, request)
	if err != nil {
		return false
	}

	err = response.JSONToStructs(&records)
	if err != nil {
		return false
	}

	if len(records) > 0 {
		return true
	}
	return false
}

// CopyMembers ...
func CopyMembers(ctx context.Context, api *eos.API, contract eos.AccountName, from string) {

	sourceAPI := *eos.New(from)
	memberRecords := getLegacyMembers(ctx, &sourceAPI, contract)

	fmt.Println("\nCopying " + strconv.Itoa(len(memberRecords)) + " members from " + from)
	bar := DefaultProgressBar(len(memberRecords))

	for _, memberRecord := range memberRecords {

		if !memberExists(ctx, api, contract, memberRecord.MemberName) {

			actions := []*eos.Action{{
				Account: contract,
				Name:    eos.ActN("addmember"),
				Authorization: []eos.PermissionLevel{
					{Actor: contract, Permission: eos.PN("active")},
				},
				ActionData: eos.NewActionData(memberRecord),
			}}

			_, err := eostest.ExecTrx(ctx, api, actions)
			if err != nil {
				panic(err)
			}

			bar.Add(1)
			time.Sleep(defaultPause())
		}
	}
}

package dao

import (
	"context"
	"fmt"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/document-graph/docgraph"
)

// SetSetting sets a single attribute on the configuration
func SetSetting(ctx context.Context, api *eos.API, contract eos.AccountName, configAtt string, flexValue *docgraph.FlexValue) (string, error) {

	action := eos.ActN("setsetting")
	actionData := make(map[string]interface{})
	actionData["key"] = configAtt
	actionData["value"] = flexValue

	actionBinary, err := api.ABIJSONToBin(ctx, contract, eos.Name(action), actionData)
	if err != nil {
		return "error", fmt.Errorf("cannot pack action data %v: %v", configAtt, err)
	}

	actions := []*eos.Action{{
		Account: contract,
		Name:    action,
		Authorization: []eos.PermissionLevel{
			{Actor: contract, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionDataFromHexData([]byte(actionBinary)),
	}}

	return eostest.ExecTrx(ctx, api, actions)
}

// RemSetting ...
func RemSetting(ctx context.Context, api *eos.API, contract eos.AccountName, settingAtt string) (string, error) {

	action := eos.ActN("remsetting")
	actionData := make(map[string]interface{})
	actionData["key"] = settingAtt

	actionBinary, err := api.ABIJSONToBin(ctx, contract, eos.Name(action), actionData)
	if err != nil {
		return "error", fmt.Errorf("cannot pack action data %v: %v", settingAtt, err)
	}

	actions := []*eos.Action{{
		Account: contract,
		Name:    action,
		Authorization: []eos.PermissionLevel{
			{Actor: contract, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionDataFromHexData([]byte(actionBinary)),
	}}

	return eostest.ExecTrx(ctx, api, actions)
}

// SetNameSetting is a helper for setting a single name type configuration item
func SetNameSetting(ctx context.Context, api *eos.API, contract eos.AccountName, label string, value eos.AccountName) (string, error) {
	return SetSetting(ctx, api, contract, label, &docgraph.FlexValue{
		BaseVariant: eos.BaseVariant{
			TypeID: docgraph.GetVariants().TypeID("name"),
			Impl:   value,
		},
	})
}

// SetIntSetting is a helper for setting a single name type configuration item
func SetIntSetting(ctx context.Context, api *eos.API, contract eos.AccountName, label string, value int64) (string, error) {
	return SetSetting(ctx, api, contract, label, &docgraph.FlexValue{
		BaseVariant: eos.BaseVariant{
			TypeID: docgraph.GetVariants().TypeID("int64"),
			Impl:   value,
		}})
}

type addPeriod struct {
	Predecessor eos.Checksum256 `json:"predecessor"`
	StartTime   eos.TimePoint   `json:"start_time"`
	Label       string          `json:"label"`
}

// AddPeriods adds the number of periods with the corresponding duration to the DAO
func AddPeriods(ctx context.Context, api *eos.API, daoContract eos.AccountName,
	predecessor eos.Checksum256,
	numPeriods int, periodDuration time.Duration) ([]docgraph.Document, error) {

	marker := time.Now()
	startTime := eos.TimePoint(marker.UnixNano() / 1000)
	periods := make([]docgraph.Document, numPeriods)

	for i := 0; i < numPeriods; i++ {
		startTime = eos.TimePoint((marker.UnixNano() / 1000) + 1)
		addPeriodAction := eos.Action{
			Account: daoContract,
			Name:    eos.ActN("addperiod"),
			Authorization: []eos.PermissionLevel{
				{Actor: daoContract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(addPeriod{
				Predecessor: predecessor,
				StartTime:   startTime,
				Label:       "test phase",
			}),
		}

		startTime = eos.TimePoint(marker.Add(periodDuration).UnixNano() / 1000)
		marker = marker.Add(periodDuration).Add(time.Millisecond)

		_, err := eostest.ExecTrx(ctx, api, []*eos.Action{&addPeriodAction})
		if err != nil {
			return periods, fmt.Errorf("cannot add period: %v", err)
		}

		periods[i], _ = docgraph.GetLastDocument(ctx, api, daoContract)
		predecessor = periods[i].Hash
	}

	return periods, nil
}

// // Period represents a period of time aligning to a payroll period, typically a week
// type Period struct {
// 	PeriodID  uint64             `json:"period_id"`
// 	StartTime eos.BlockTimestamp `json:"start_time"`
// 	EndTime   eos.BlockTimestamp `json:"end_time"`
// 	Phase     string             `json:"phase"`
// }

// LoadPeriods loads the period data from the blockchain
func LoadPeriods(api *eos.API, includePast, includeFuture bool) ([]Period, error) {

	var periods []Period
	var periodRequest eos.GetTableRowsRequest
	periodRequest.Code = "dao.hypha"
	periodRequest.Scope = "dao.hypha"
	periodRequest.Table = "periods"
	periodRequest.Limit = 1000
	periodRequest.JSON = true

	periodResponse, err := api.GetTableRows(context.Background(), periodRequest)
	if err != nil {
		return []Period{}, fmt.Errorf("cannot load periods %v", err)
	}

	periodResponse.JSONToStructs(&periods)
	return periods, nil
}

type applyParm struct {
	Applicant eos.AccountName
	Notes     string
}

// Apply applies for membership to the DAO
func Apply(ctx context.Context, api *eos.API, contract eos.AccountName,
	applicant eos.AccountName, notes string) (string, error) {

	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("apply"),
		Authorization: []eos.PermissionLevel{
			{Actor: applicant, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(applyParm{
			Applicant: applicant,
			Notes:     notes,
		}),
	}}
	return eostest.ExecTrx(ctx, api, actions)
}

type enrollParm struct {
	Enroller  eos.AccountName
	Applicant eos.AccountName
	Content   string
}

// Enroll an applicant in the DAO
func Enroll(ctx context.Context, api *eos.API, contract eos.AccountName, enroller, applicant eos.AccountName) (string, error) {
	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("enroll"),
		Authorization: []eos.PermissionLevel{
			{Actor: enroller, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(enrollParm{
			Enroller:  enroller,
			Applicant: applicant,
			Content:   string("enroll in dao"),
		}),
	}}
	return eostest.ExecTrx(ctx, api, actions)
}

// CreateRoot creates the root node
func CreateRoot(ctx context.Context, api *eos.API, contract eos.AccountName) (string, error) {
	actionData := make(map[string]interface{})
	actionData["notes"] = "notes"

	actionBinary, err := api.ABIJSONToBin(ctx, contract, eos.Name("createroot"), actionData)
	if err != nil {
		return "abi error", err
	}

	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("createroot"),
		Authorization: []eos.PermissionLevel{
			{Actor: contract, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionDataFromHexData([]byte(actionBinary)),
	}}
	return eostest.ExecTrx(ctx, api, actions)
}

type claim struct {
	AssignmentHash eos.Checksum256 `json:"hash"`
	PeriodID       uint64          `json:"period_id"`
}

// ClaimPay claims a period of pay for an assignment
func ClaimPay(ctx context.Context, api *eos.API, contract, claimer eos.AccountName, assignmentHash eos.Checksum256, periodID uint64) (string, error) {

	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("claimpay"),
		Authorization: []eos.PermissionLevel{
			{Actor: claimer, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(claim{
			AssignmentHash: assignmentHash,
			PeriodID:       periodID,
		}),
	}}
	return eostest.ExecTrx(ctx, api, actions)
}

// type AssignmentPay struct {
// 	ID           uint64             `json:"ass_payment_id"`
// 	AssignmentID uint64             `json:"assignment_id"`
// 	PeriodID     uint64             `json:"period_id"`
// 	Recipient    eos.Name           `json:"recipient"`
// 	Payments     []eos.Asset        `json:"payments"`
// 	PaymentDate  eos.BlockTimestamp `json:"payment_date"`
// }

type balance struct {
	Balance eos.Asset `json:"balance"`
}

// GetBalance return the token balance
func GetBalance(ctx context.Context, api *eos.API, tokenContract, member string) eos.Asset {
	var b []balance
	var request eos.GetTableRowsRequest
	request.Code = tokenContract
	request.Scope = member
	request.Table = "accounts"
	request.Limit = 1
	request.JSON = true
	response, _ := api.GetTableRows(ctx, request)
	response.JSONToStructs(&b)
	if len(b) == 0 {
		rv, _ := eos.NewAssetFromString("0.00 NOBAL")
		return rv
	}
	return b[0].Balance
}

// Lock is an escrow lock
type Lock struct {
	ID            uint64             `json:"id"`
	LockType      eos.Name           `json:"lock_type"`
	Sponsor       eos.Name           `json:"sponsor"`
	Beneficiary   eos.Name           `json:"beneficiary"`
	Quantity      eos.Asset          `json:"quantity"`
	TriggerEvent  eos.Name           `json:"trigger_event"`
	TriggerSource eos.Name           `json:"trigger_source"`
	VestingDate   eos.BlockTimestamp `json:"vesting_date"`
	Notes         string             `json:"notes"`
	CreatedDate   eos.BlockTimestamp `json:"created_date"`
	UpdatedDate   eos.BlockTimestamp `json:"updated_date"`
}

// GetEscrowBalance returns the total amount locked in escrow for this user
func GetEscrowBalance(ctx context.Context, api *eos.API, escrowContract, member string) eos.Asset {

	var locks []Lock
	var request eos.GetTableRowsRequest
	request.Code = escrowContract
	request.Scope = escrowContract
	request.Table = "locks"
	request.Limit = 1000
	request.Index = "3"
	request.KeyType = "i64"
	request.LowerBound = member
	request.UpperBound = member
	request.JSON = true
	response, _ := api.GetTableRows(ctx, request)
	response.JSONToStructs(&locks)

	escrowBalance, _ := eos.NewAssetFromString("0.0000 SEEDS")
	for _, lock := range locks {
		escrowBalance = escrowBalance.Add(lock.Quantity)
	}

	return escrowBalance
}

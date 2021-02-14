package dao

import (
	"context"
	"fmt"
	"strconv"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
)

type addPeriodBTS struct {
	Predecessor eos.Checksum256 `json:"predecessor"`
	StartTime   eos.TimePoint   `json:"start_time"`
	Label       string          `json:"label"`
}

type migratePer struct {
	ID uint64 `json:"id"`
}

// MigrateAssPayouts ...
func MigrateAssPayouts(ctx context.Context, api *eos.API, contract eos.AccountName) {

	payoutsIn, err := getAllAssPayouts(ctx, api, contract)
	if err != nil {
		panic(err)
	}

	fmt.Println("\nMigrating assignment payments : " + strconv.Itoa(len(payoutsIn)))
	bar := DefaultProgressBar(len(payoutsIn))

	for index, payoutIn := range payoutsIn {

		actions := []*eos.Action{{
			Account: contract,
			Name:    eos.ActN("migasspay"),
			Authorization: []eos.PermissionLevel{
				{Actor: contract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(migratePer{
				ID: payoutIn.ID,
			}),
		}}

		_, err := eostest.ExecTrx(ctx, api, actions)
		if err != nil {
			fmt.Println("\nFAILED to migrate assignment pay: ", payoutIn.PaymentDate.Format("2006 Jan 02"), ", ", strconv.Itoa(index)+" / "+strconv.Itoa(len(payoutsIn)))
			fmt.Println(err)
			fmt.Println()
		}

		bar.Add(1)
		time.Sleep(defaultPause())
	}
}

// MigratePeriods ...
func MigratePeriods(ctx context.Context, api *eos.API, contract eos.AccountName) {

	periods := getLegacyPeriods(ctx, api, contract)

	fmt.Println("\nMigrating periods: " + strconv.Itoa(len(periods)))
	bar := DefaultProgressBar(len(periods))

	for _, period := range periods {

		actions := []*eos.Action{{
			Account: contract,
			Name:    eos.ActN("migrateper"),
			Authorization: []eos.PermissionLevel{
				{Actor: contract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(migratePer{
				ID: period.PeriodID,
			}),
		}}

		_, err := eostest.ExecTrx(ctx, api, actions)
		if err != nil {
			fmt.Println("\nFAILED to migrate period: ", strconv.Itoa(int(period.PeriodID)))
			fmt.Println(err)
			fmt.Println()
		}
		bar.Add(1)
		time.Sleep(defaultPause())
	}
}

// MigrateMembers ...
func MigrateMembers(ctx context.Context, api *eos.API, contract eos.AccountName) {

	memberRecords := getLegacyMembers(ctx, api, contract)

	fmt.Println("\nMigrating members: " + strconv.Itoa(len(memberRecords)))
	bar := DefaultProgressBar(len(memberRecords))

	for index, memberRecord := range memberRecords {
		actions := []*eos.Action{{
			Account: contract,
			Name:    eos.ActN("migratemem"),
			Authorization: []eos.PermissionLevel{
				{Actor: contract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(memberRecord),
		}}

		_, err := eostest.ExecTrx(ctx, api, actions)
		if err != nil {
			fmt.Println("\n\nFAILED to migrate a member: ", memberRecord.MemberName, ", ", strconv.Itoa(index)+" / "+strconv.Itoa(len(memberRecords)))
			fmt.Println(err)
			fmt.Println()
		}

		bar.Add(1)
		time.Sleep(defaultPause())
	}
}

type migrate struct {
	Scope eos.Name `json:"scope"`
	ID    uint64   `json:"id"`
}

// MigrateObjects ...
func MigrateObjects(ctx context.Context, api *eos.API, contract eos.AccountName, scope eos.Name) {

	objects, _ := getLegacyObjects(ctx, api, contract, scope)

	fmt.Println("\nMigrating " + string(scope) + " objects: " + strconv.Itoa(len(objects)))
	bar := DefaultProgressBar(len(objects))

	for index, object := range objects {

		actions := []*eos.Action{{
			Account: contract,
			Name:    eos.ActN("migrate"),
			Authorization: []eos.PermissionLevel{
				{Actor: contract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(migrate{
				Scope: scope,
				ID:    object.ID,
			}),
		}}

		_, err := eostest.ExecTrx(ctx, api, actions)
		if err != nil {
			fmt.Println("\n\nFailed to migrate : ", strconv.Itoa(int(object.ID)), ", ", strconv.Itoa(index)+" / "+strconv.Itoa(len(objects)))
			fmt.Println(err)
			fmt.Println()
		}

		bar.Add(1)
		time.Sleep(defaultPause())
	}
}

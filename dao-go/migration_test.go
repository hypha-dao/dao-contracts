package dao_test

import (
	"context"
	"fmt"
	"strconv"
	"testing"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/hypha-dao/dao-contracts/dao-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"gotest.tools/assert"

	"github.com/eoscanada/eos-go"
)

type migrate struct {
	Scope eos.Name `json:"scope"`
	ID    uint64   `json:"id"`
}

func getObject(t *testing.T, env *Environment, scope eos.Name, endpoint string, ID uint64) Object {
	endpointApi := *eos.New(endpoint)

	var objects []Object
	var request eos.GetTableRowsRequest
	request.Code = "dao.hypha"
	request.Scope = string(scope)
	request.Table = "objects"
	request.LowerBound = strconv.Itoa(int(ID))
	request.UpperBound = strconv.Itoa(int(ID))
	request.Limit = 1
	request.JSON = true
	response, _ := endpointApi.GetTableRows(env.ctx, request)
	response.JSONToStructs(&objects)
	return objects[0]
}

func getObjects(t *testing.T, env *Environment, scope eos.Name, endpoint string) []Object {
	endpointApi := *eos.New(endpoint)

	var objects []Object
	var request eos.GetTableRowsRequest
	request.Code = "dao.hypha"
	request.Scope = string(scope)
	request.Table = "objects"
	request.Limit = 10000
	request.JSON = true
	response, _ := endpointApi.GetTableRows(env.ctx, request)
	response.JSONToStructs(&objects)
	return objects
}

func copyObjects(t *testing.T, env *Environment, scope eos.Name, from string) {

	objects := getObjects(t, env, scope, from)

	for _, object := range objects {
		object.Scope = eos.Name(scope)

		actions := []*eos.Action{{
			Account: env.DAO,
			Name:    eos.ActN("createobj"),
			Authorization: []eos.PermissionLevel{
				{Actor: env.DAO, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(object),
		}}

		_, err := eostest.ExecTrx(env.ctx, &env.api, actions)
		assert.NilError(t, err)
	}
}

func TestMigration(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	// var env Environment
	env = SetupEnvironment(t)

	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	t.Run("Test Object Migration", func(t *testing.T) {

		tests := []struct {
			name      string
			scope     eos.Name
			ID        uint64
			ballot_id eos.Name
		}{
			{
				name:      "Initial role",
				scope:     eos.Name("role"),
				ID:        0,
				ballot_id: eos.Name("hypha1.....2i"),
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)

			copyObjects(t, env, test.scope, "https://api.telos.kitchen")
			pause(t, env.ChainResponsePause, "", "Waiting...")

			object := getObject(t, env, test.scope, "http://localhost:8888", test.ID)

			actions := []*eos.Action{{
				Account: env.DAO,
				Name:    eos.ActN("migrate"),
				Authorization: []eos.PermissionLevel{
					{Actor: env.DAO, Permission: eos.PN("active")},
				},
				ActionData: eos.NewActionData(migrate{
					Scope: test.scope,
					ID:    object.ID,
				}),
			}}

			_, err := eostest.ExecTrx(env.ctx, &env.api, actions)
			assert.NilError(t, err)
			pause(t, env.ChainResponsePause, "", "Waiting...")

			role, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, test.scope)
			assert.NilError(t, err)

			ballot, err := role.GetContent("ballot_id")
			assert.NilError(t, err)
			assert.Equal(t, ballot.Impl.(eos.Name), test.ballot_id)
		}
	})
}

func TestMigrateRoles(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	// var env Environment
	env = SetupEnvironment(t)

	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	t.Run("Migrate all", func(t *testing.T) {

		tests := []struct {
			name      string
			scope     eos.Name
			ID        uint64
			ballot_id eos.Name
		}{
			{
				name:      "Initial role",
				scope:     eos.Name("role"),
				ID:        0,
				ballot_id: eos.Name("hypha1.....2i"),
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)

			copyObjects(t, env, test.scope, "https://api.telos.kitchen")
			pause(t, env.ChainResponsePause, "", "Waiting...")

			objects := getObjects(t, env, test.scope, "http://localhost:8888")

			for _, object := range objects {
				object.Scope = eos.Name(test.scope)

				actions := []*eos.Action{{
					Account: env.DAO,
					Name:    eos.ActN("migrate"),
					Authorization: []eos.PermissionLevel{
						{Actor: env.DAO, Permission: eos.PN("active")},
					},
					ActionData: eos.NewActionData(migrate{
						Scope: test.scope,
						ID:    object.ID,
					}),
				}}
				_, err := eostest.ExecTrx(env.ctx, &env.api, actions)
				assert.NilError(t, err)
			}
		}
	})
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

type reset4test struct {
	Notes string `json:"notes"`
}

func TestCopyPeriods(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	// var env Environment
	env = SetupEnvironment(t)

	actions := []*eos.Action{{
		Account: env.DAO,
		Name:    eos.ActN("reset4test"),
		Authorization: []eos.PermissionLevel{
			{Actor: env.DAO, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(reset4test{
			Notes: "notes",
		}),
	}}

	trxID, err := eostest.ExecTrx(env.ctx, &env.api, actions)
	assert.NilError(t, err)

	t.Log("Reset successful: " + trxID)
	pause(t, env.ChainResponsePause, "", "Waiting...")

	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	t.Run("Copy periods", func(t *testing.T) {

		periods := getProdPeriods()

		predecessor := env.Root.Hash
		var lastPeriod docgraph.Document

		for _, period := range periods {

			seconds := period.StartTime
			fmt.Println("seconds (nano)		: ", strconv.Itoa(int(seconds.UnixNano())))

			microSeconds := seconds.UnixNano() / 1000
			fmt.Println("seconds (micro)	: ", strconv.Itoa(int(microSeconds)))

			startTime := eos.TimePoint(microSeconds)

			addPeriodAction := eos.Action{
				Account: env.DAO,
				Name:    eos.ActN("addperiod"),
				Authorization: []eos.PermissionLevel{
					{Actor: env.DAO, Permission: eos.PN("active")},
				},
				ActionData: eos.NewActionData(addPeriodBTS{
					Predecessor: predecessor,
					StartTime:   startTime,
					Label:       period.Phase,
				}),
			}

			_, err := eostest.ExecTrx(env.ctx, &env.api, []*eos.Action{&addPeriodAction})
			assert.NilError(t, err)

			pause(t, time.Second, "Build block...", "")

			lastPeriod, err = docgraph.GetLastDocument(env.ctx, &env.api, env.DAO)
			assert.NilError(t, err)

			predecessor = lastPeriod.Hash
		}
	})
}

func TestReset(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	// var env Environment
	env = SetupEnvironment(t)

	pause(t, env.ChainResponsePause, "", "Waiting...")

	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	t.Run("reset4test", func(t *testing.T) {

		actions := []*eos.Action{{
			Account: env.DAO,
			Name:    eos.ActN("reset4test"),
			Authorization: []eos.PermissionLevel{
				{Actor: env.DAO, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(reset4test{
				Notes: "notes",
			}),
		}}

		trxID, err := eostest.ExecTrx(env.ctx, &env.api, actions)
		assert.NilError(t, err)

		t.Log("Reset successful: " + trxID)
	})
}

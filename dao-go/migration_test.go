package dao_test

import (
	"bytes"
	"testing"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/spf13/viper"
	"gotest.tools/assert"

	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-contracts/dao-go"
)

func TestMigrate(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	viper.SetConfigType("yaml")

	var yamlExample = []byte(`
contract: dao.hypha
host: http://localhost:8888
pause: 200ms
`)

	viper.ReadConfig(bytes.NewBuffer(yamlExample))

	env = SetupEnvironmentWithFlags(t, false, false)

	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	dao.CopyMembers(env.ctx, &env.api, env.DAO, "https://api.telos.kitchen")
	dao.MigrateMembers(env.ctx, &env.api, env.DAO)

	dao.CopyPeriods(env.ctx, &env.api, env.DAO, "https://api.telos.kitchen")
	dao.MigratePeriods(env.ctx, &env.api, env.DAO)

	scopes := []eos.Name{"role", "assignment"}

	for _, scope := range scopes {
		dao.CopyObjects(env.ctx, &env.api, env.DAO, scope, "https://api.telos.kitchen")
		dao.MigrateObjects(env.ctx, &env.api, env.DAO, scope)
	}

	dao.CopyObjects(env.ctx, &env.api, env.DAO, eos.Name("payout"), "https://api.telos.kitchen")
	// dao.MigrateObjects(env.ctx, &env.api, env.DAO, eos.Name("payout"))

	// dao.CopyAssPayouts(env.ctx, &env.api, env.DAO, "https://api.telos.kitchen")
	// dao.MigrateAssPayouts(env.ctx, &env.api, env.DAO)
}

// func TestMigrateRoles(t *testing.T) {
// 	teardownTestCase := setupTestCase(t)
// 	defer teardownTestCase(t)

// 	// var env Environment
// 	env = SetupEnvironment(t)

// 	t.Log(env.String())
// 	t.Log("\nDAO Environment Setup complete\n")

// 	t.Run("Migrate all", func(t *testing.T) {

// 		tests := []struct {
// 			name      string
// 			scope     eos.Name
// 			ID        uint64
// 			ballot_id eos.Name
// 		}{
// 			{
// 				name:      "Initial role",
// 				scope:     eos.Name("role"),
// 				ID:        0,
// 				ballot_id: eos.Name("hypha1.....2i"),
// 			},
// 		}

// 		for _, test := range tests {

// 			t.Log("\n\nStarting test: ", test.name)

// 			copyObjects(t, env, test.scope, "https://api.telos.kitchen")
// 			pause(t, env.ChainResponsePause, "", "Waiting...")

// 			migrateMembers(env.ctx, &env.api, env.DAO, "https://api.telos.kitchen")

// 			objects := getObjects(t, env, test.scope, "http://localhost:8888")

// 			for _, object := range objects {
// 				object.Scope = eos.Name(test.scope)

// 				actions := []*eos.Action{{
// 					Account: env.DAO,
// 					Name:    eos.ActN("migrate"),
// 					Authorization: []eos.PermissionLevel{
// 						{Actor: env.DAO, Permission: eos.PN("active")},
// 					},
// 					ActionData: eos.NewActionData(migrate{
// 						Scope: test.scope,
// 						ID:    object.ID,
// 					}),
// 				}}
// 				_, err := eostest.ExecTrx(env.ctx, &env.api, actions)
// 				assert.NilError(t, err)

// 				pause(t, env.ChainResponsePause, "", "Waiting...")

// 				role, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, test.scope)
// 				assert.NilError(t, err)

// 				ballot, err := role.GetContent("ballot_id")
// 				assert.NilError(t, err)
// 				assert.Equal(t, ballot.Impl.(eos.Name), test.ballot_id)
// 			}
// 		}
// 	})
// }

type notes struct {
	Notes string `json:"notes"`
}

// func TestCopyPeriods(t *testing.T) {
// 	teardownTestCase := setupTestCase(t)
// 	defer teardownTestCase(t)

// 	env = SetupEnvironment(t)

// 	actions := []*eos.Action{{
// 		Account: env.DAO,
// 		Name:    eos.ActN("reset4test"),
// 		Authorization: []eos.PermissionLevel{
// 			{Actor: env.DAO, Permission: eos.PN("active")},
// 		},
// 		ActionData: eos.NewActionData(notes{
// 			Notes: "notes",
// 		}),
// 	}}

// 	trxID, err := eostest.ExecTrx(env.ctx, &env.api, actions)
// 	assert.NilError(t, err)

// 	t.Log("Reset successful: " + trxID)
// 	pause(t, env.ChainResponsePause, "", "Waiting...")

// 	t.Log(env.String())
// 	t.Log("\nDAO Environment Setup complete\n")

// 	t.Run("Copy periods", func(t *testing.T) {

// 		periods := getProdPeriods()

// 		predecessor := env.Root.Hash
// 		var lastPeriod docgraph.Document

// 		for _, period := range periods {

// 			seconds := period.StartTime
// 			fmt.Println("seconds (nano)		: ", strconv.Itoa(int(seconds.UnixNano())))

// 			microSeconds := seconds.UnixNano() / 1000
// 			fmt.Println("seconds (micro)	: ", strconv.Itoa(int(microSeconds)))

// 			startTime := eos.TimePoint(microSeconds)

// 			addPeriodAction := eos.Action{
// 				Account: env.DAO,
// 				Name:    eos.ActN("addperiod"),
// 				Authorization: []eos.PermissionLevel{
// 					{Actor: env.DAO, Permission: eos.PN("active")},
// 				},
// 				ActionData: eos.NewActionData(addPeriodBTS{
// 					Predecessor: predecessor,
// 					StartTime:   startTime,
// 					Label:       period.Phase,
// 				}),
// 			}

// 			_, err := eostest.ExecTrx(env.ctx, &env.api, []*eos.Action{&addPeriodAction})
// 			assert.NilError(t, err)

// 			pause(t, time.Second, "Build block...", "")

// 			lastPeriod, err = docgraph.GetLastDocument(env.ctx, &env.api, env.DAO)
// 			assert.NilError(t, err)

// 			predecessor = lastPeriod.Hash
// 		}
// 	})
// }

func TestReset(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

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
			ActionData: eos.NewActionData(notes{
				Notes: "notes",
			}),
		}}

		trxID, err := eostest.ExecTrx(env.ctx, &env.api, actions)
		assert.NilError(t, err)

		t.Log("Reset successful: " + trxID)
	})
}

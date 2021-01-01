package dao_test

import (
	"testing"

	eostest "github.com/digital-scarcity/eos-go-test"
	"gotest.tools/assert"

	"github.com/eoscanada/eos-go"
)

func TestMigrate(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)

	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	t.Run("Test Migrate", func(t *testing.T) {
		// dao.CopyMembers(env.ctx, &env.api, env.DAO, "https://api.telos.kitchen")
		// dao.MigrateMembers(env.ctx, &env.api, env.DAO)

		// dao.CopyPeriods(env.ctx, &env.api, env.DAO, "https://api.telos.kitchen")
		// dao.MigratePeriods(env.ctx, &env.api, env.DAO)

		// scopes := []eos.Name{"role", "assignment"}

		// for _, scope := range scopes {
		// 	dao.CopyObjects(env.ctx, &env.api, env.DAO, scope, "https://api.telos.kitchen")
		// 	dao.MigrateObjects(env.ctx, &env.api, env.DAO, scope)
		// }

		// dao.CopyAssPayouts(env.ctx, &env.api, env.DAO, "https://api.telos.kitchen")
		// dao.MigrateAssPayouts(env.ctx, &env.api, env.DAO)
	})

	// t.Run("Test Migrate Periods", func(t *testing.T) {
	// 	dao.MigratePeriods(&env.api, env.Root.Hash, env.DAO)
	// })

	// t.Run("Test Migrate Objects", func(t *testing.T) {
	// 	scope := eos.Name("role")
	// 	dao.CopyObjects(env.ctx, &env.api, env.DAO, scope, "https://api.telos.kitchen")
	// 	pause(t, env.ChainResponsePause, "", "Waiting...")
	// 	objects := dao.GetObjects(env.ctx, env.DAO, scope, "http://localhost:8888")

	// 	for _, object := range objects {

	// 		t.Log("Running migration on scope: " + string(scope) + ", ID: " + strconv.Itoa(int(object.ID)))

	// 		actions := []*eos.Action{{
	// 			Account: env.DAO,
	// 			Name:    eos.ActN("migrate"),
	// 			Authorization: []eos.PermissionLevel{
	// 				{Actor: env.DAO, Permission: eos.PN("active")},
	// 			},
	// 			ActionData: eos.NewActionData(dao.MigrateObject{
	// 				Scope: scope,
	// 				ID:    object.ID,
	// 			}),
	// 		}}

	// 		trxID, err := eostest.ExecTrx(env.ctx, &env.api, actions)
	// 		assert.NilError(t, err)

	// 		t.Log("Migration completed. Transaction ID:  " + trxID)
	// 	}
	// })
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

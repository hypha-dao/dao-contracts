package dao_test

import (
	"fmt"
	"io/ioutil"
	"testing"

	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-contracts/dao-go"
	"gotest.tools/assert"
)
// func TestSetup(t *testing.T) {
// 	teardownTestCase := setupTestCase(t)
// 	defer teardownTestCase(t)

// 	env = SetupEnvironment(t)

// 	t.Log(env.String())
// 	t.Log("\nDAO Environment Setup complete\n")

// 	// enroll the test members
// 	// dao.EnrollMembers(env.ctx, &env.api, env.DAO)

// 	mem2 := env.Members[2].Member
// 	roleFilename := "/Users/max/dev/hypha/daoctl/testing/role.json"
// 	roleData, err := ioutil.ReadFile(roleFilename)
// 	if err != nil {
// 		fmt.Println("Unable to read file: ", roleFilename)
// 		return
// 	}

// 	role, err := dao.CreateRole(env.ctx, &env.api, env.DAO, env.TelosDecide, mem2, roleData)
// 	if err != nil {
// 		panic(err)
// 	}
// 	fmt.Println("Created role document	: ", role.Hash.String())

// 	assignmentData, err := ioutil.ReadFile("/Users/max/dev/hypha/daoctl/testing/assignment.json")
// 	if err != nil {
// 		fmt.Println("Unable to read file: ", assignmentData)
// 		return
// 	}

// 	roleAssignment, err := dao.CreateAssignment(env.ctx, &env.api, env.DAO, env.TelosDecide, mem2, eos.Name("role"), eos.Name("assignment"), assignmentData)
// 	if err != nil {
// 		panic(err)
// 	}
// 	fmt.Println("Created role assignment document	: ", roleAssignment.Hash.String())

// 	_, err = ClaimNextPeriod(t, env, mem2, roleAssignment)
// 	assert.NilError(t, err)

// 	payoutData, err := ioutil.ReadFile("/Users/max/dev/hypha/daoctl/testing/payout.json")
// 	if err != nil {
// 		fmt.Println("Unable to read file: ", payoutData)
// 		return
// 	}

// 	payAmt, _ := eos.NewAssetFromString("1000.00 USD")
// 	payout, err := dao.CreatePayout(env.ctx, &env.api, env.DAO, env.TelosDecide, mem2, mem2, payAmt, 50, payoutData)
// 	if err != nil {
// 		panic(err)
// 	}
// 	fmt.Println("Created payout document	: ", payout.Hash.String())

// 	badgeData, err := ioutil.ReadFile("/Users/max/dev/hypha/daoctl/testing/badge.json")
// 	if err != nil {
// 		panic(err)
// 	}

// 	badge, err := dao.CreateBadge(env.ctx, &env.api, env.DAO, env.TelosDecide, mem2, badgeData)
// 	if err != nil {
// 		panic(err)
// 	}
// 	fmt.Println("Created badge document	: ", badge.Hash.String())

// 	badgeAssignmentData, err := ioutil.ReadFile("/Users/max/dev/hypha/daoctl/testing/badge-assignment.json")
// 	if err != nil {
// 		panic(err)
// 	}

// 	badgeAssignment, err := dao.CreateAssignment(env.ctx, &env.api, env.DAO, env.TelosDecide, mem2, eos.Name("badge"), eos.Name("assignbadge"), badgeAssignmentData)
// 	if err != nil {
// 		panic(err)
// 	}
// 	fmt.Println("Created badge assignment document	: ", badgeAssignment.Hash.String())

// 	t.Log("Waiting for a period to lapse...")
// 	pause(t, env.PeriodPause, "", "Waiting...")

// 	_, err = ClaimNextPeriod(t, env, mem2, roleAssignment)
// 	assert.NilError(t, err)
// }

// func TestPretend(t *testing.T) {
// 	teardownTestCase := setupTestCase(t)
// 	defer teardownTestCase(t)

// 	env = SetupEnvironment(t)

// 	t.Log(env.String())
// 	t.Log("\nDAO Environment Setup complete\n")

// 	// enroll the test members
// 	//dao.EnrollMembers(env.ctx, &env.api, env.DAO)

// 	mem2 := env.Members[2].Member
// 	roleAssignment, err := dao.CreatePretend(env.ctx, &env.api, env.DAO, env.TelosDecide, mem2)
// 	assert.NilError(t, err)

// 	t.Log("Waiting for a period to lapse...")
// 	pause(t, env.PeriodPause, "", "Waiting...")

// 	_, err = ClaimNextPeriod(t, env, mem2, roleAssignment)
// 	assert.NilError(t, err)

// 	t.Log("Waiting for a period to lapse...")
// 	pause(t, env.PeriodPause, "", "Waiting...")

// 	_, err = ClaimNextPeriod(t, env, mem2, roleAssignment)
// 	assert.NilError(t, err)

// 	t.Log("Waiting for a period to lapse...")
// 	pause(t, env.PeriodPause, "", "Waiting...")

// 	_, err = ClaimNextPeriod(t, env, mem2, roleAssignment)
// 	assert.NilError(t, err)
// }

// func TestReset2(t *testing.T) {
// 	teardownTestCase := setupTestCase(t)
// 	defer teardownTestCase(t)

// 	env = SetupEnvironment(t)

// 	pause(t, env.ChainResponsePause, "", "Waiting...")

// 	t.Log(env.String())
// 	t.Log("\nDAO Environment Setup complete\n")

// 	t.Run("reset4test", func(t *testing.T) {

// 		// actions := []*eos.Action{{
// 		// 	Account: env.DAO,
// 		// 	Name:    eos.ActN("reset4test"),
// 		// 	Authorization: []eos.PermissionLevel{
// 		// 		{Actor: env.DAO, Permission: eos.PN("active")},
// 		// 	},
// 		// 	ActionData: eos.NewActionData(notes{
// 		// 		Notes: "notes",
// 		// 	}),
// 		// }}

// 		// trxID, err := eostest.ExecTrx(env.ctx, &env.api, actions)
// 		// assert.NilError(t, err)

// 		// t.Log("Reset successful: " + trxID)
// 	})
// }

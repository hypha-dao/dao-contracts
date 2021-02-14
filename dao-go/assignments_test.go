package dao_test

import (
	"strconv"
	"testing"

	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-contracts/dao-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"gotest.tools/assert"
)

func TestAssignmentProposalDocument(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	// roles
	proposer := env.Members[0]
	assignee := env.Members[1]
	closer := env.Members[2]

	role1Doc := CreateRole(t, env, proposer, closer, role1)
	role2Doc := CreateRole(t, env, proposer, closer, role2)
	role3Doc := CreateRole(t, env, proposer, closer, role3)

	t.Run("Test Assignment Document Proposal", func(t *testing.T) {

		tests := []struct {
			name       string
			roleTitle  string
			title      string
			role       docgraph.Document
			assignment string
			husd       string
			hypha      string
			hvoice     string
			usd        string
		}{
			{
				name:       "role1 - 100% 100%",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role1Doc,
				assignment: assignment1,
				husd:       "0.00 HUSD",
				hypha:      "759.75 HYPHA",
				hvoice:     "6078.02 HVOICE",
				usd:        "3039.01 USD",
			},
			{
				name:       "role2 - 100% commit, 70% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role2Doc,
				assignment: assignment2,
				husd:       "455.85 HUSD",
				hypha:      "265.91 HYPHA",
				hvoice:     "3039.00 HVOICE",
				usd:        "1519.50 USD",
			},
			{
				name:       "role2 - 75% commit, 50% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role2Doc,
				assignment: assignment3,
				husd:       "569.81 HUSD",
				hypha:      "142.45 HYPHA",
				hvoice:     "2279.26 HVOICE",
				usd:        "1519.50 USD",
			},
			{
				name:       "role3 - 51% commit, 100% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role3Doc,
				assignment: assignment4,
				husd:       "0.00 HUSD",
				hypha:      "452.05 HYPHA",
				hvoice:     "3616.42 HVOICE",
				usd:        "3545.51 USD",
			},
			{
				name:       "role3 - 25% commit, 50% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role3Doc,
				assignment: assignment5,
				husd:       "443.18 HUSD",
				hypha:      "110.79 HYPHA",
				hvoice:     "1772.74 HVOICE",
				usd:        "3545.51 USD",
			},
			{
				name:       "role3 - 100% commit, 15% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role3Doc,
				assignment: assignment6,
				husd:       "3013.68 HUSD",
				hypha:      "132.95 HYPHA",
				hvoice:     "7091.02 HVOICE",
				usd:        "3545.51 USD",
			},
			{
				name:       "role3 - 10% commit, 0% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role3Doc,
				assignment: assignment7,
				husd:       "354.55 HUSD",
				hypha:      "0.00 HYPHA",
				hvoice:     "709.10 HVOICE",
				usd:        "3545.51 USD",
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)
			_, err := dao.ProposeAssignment(env.ctx, &env.api, env.DAO, proposer.Member, assignee.Member, test.role.Hash, env.Periods[0].Hash, test.assignment)
			assert.NilError(t, err)

			// retrieve the document we just created
			assignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, assignment.Creator, proposer.Member)

			fv, err := assignment.GetContent("title")
			assert.NilError(t, err)
			assert.Equal(t, fv.String(), test.title)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// root 		---proposal---> 	propDocument
			// member 		---owns-------> 	propDocument
			// propDocument ---ownedby----> 	member
			checkEdge(t, env, env.Root, assignment, eos.Name("proposal"))
			checkEdge(t, env, proposer.Doc, assignment, eos.Name("owns"))
			checkEdge(t, env, assignment, proposer.Doc, eos.Name("ownedby"))
			checkEdge(t, env, assignment, test.role, eos.Name("role"))

			// roleHash, err := assignment.GetContent("role")
			// assert.NilError(t, err)

			// roleDocument, err := docgraph.LoadDocument(env.ctx, &env.api, env.DAO, roleHash.String())
			// assert.NilError(t, err)

			// // check edge from assignment to role
			// exists, err := docgraph.EdgeExists(env.ctx, &env.api, env.DAO, assignment, roleDocument, eos.Name("role"))
			// assert.NilError(t, err)
			// if !exists {
			// 	t.Log("Edge does not exist	: ", assignment.Hash.String(), "	-- role	--> 	", roleDocument.Hash.String())
			// }
			// assert.Check(t, exists)

			ballot, err := assignment.GetContent("ballot_id")
			assert.NilError(t, err)
			voteToPassTD(t, env, ballot.Impl.(eos.Name))

			t.Log("Member: ", closer.Member, " is closing assignment proposal	: ", assignment.Hash.String())
			_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, assignment.Hash)
			assert.NilError(t, err)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// update graph edges:
			//  member          ---- assigned           ---->   role_assignment
			//  role_assignment ---- assignee           ---->   member
			//  role_assignment ---- role               ---->   role
			//  role            ---- role_assignment    ---->   role_assignment
			checkEdge(t, env, assignee.Doc, assignment, eos.Name("assigned"))
			checkEdge(t, env, assignment, assignee.Doc, eos.Name("assignee"))
			checkEdge(t, env, assignment, test.role, eos.Name("role"))
			checkEdge(t, env, test.role, assignment, eos.Name("assignment"))

			//  root ---- passedprops        ---->   role_assignment
			checkEdge(t, env, env.Root, assignment, eos.Name("passedprops"))

			fetchedAssignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("assignment"))

			husd, err := fetchedAssignment.GetContent("husd_salary_per_phase")
			assert.NilError(t, err)
			t.Log("test: ", test.name, ": husd: "+husd.String())
			assert.Equal(t, husd.String(), test.husd)

			hypha, err := fetchedAssignment.GetContent("hypha_salary_per_phase")
			assert.NilError(t, err)
			t.Log("test: ", test.name, ": hypha: "+hypha.String())
			assert.Equal(t, hypha.String(), test.hypha)

			hvoice, err := fetchedAssignment.GetContent("hvoice_salary_per_phase")
			assert.NilError(t, err)
			t.Log("test: ", test.name, ": hvoice: "+hvoice.String())
			assert.Equal(t, hvoice.String(), test.hvoice)

			usd, err := fetchedAssignment.GetContent("usd_salary_value_per_phase")
			assert.NilError(t, err)
			t.Log("test: ", test.name, ": usd: "+usd.String())
			assert.Equal(t, usd.String(), test.usd)
		}
	})
}

func TestAssignmentDefaults(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	// roles
	proposer := env.Members[0]
	assignee := env.Members[1]
	closer := env.Members[2]

	role1Doc := CreateRole(t, env, proposer, closer, role1)

	t.Run("Test Assignment Document Proposal", func(t *testing.T) {

		tests := []struct {
			name               string
			roleTitle          string
			title              string
			role               docgraph.Document
			assignment         string
			defaultPeriodCount int64
			hypha              string
			hvoice             string
			usd                string
		}{
			{
				name:               "role1 - 100% 100%",
				roleTitle:          "Underwater Basketweaver",
				title:              "Underwater Basketweaver - Atlantic",
				role:               role1Doc,
				assignment:         assignment8,
				defaultPeriodCount: 13,
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)
			_, err := dao.ProposeAssignment(env.ctx, &env.api, env.DAO, proposer.Member, assignee.Member, test.role.Hash, env.Periods[0].Hash, test.assignment)
			assert.NilError(t, err)

			// retrieve the document we just created
			assignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, assignment.Creator, proposer.Member)

			fv, err := assignment.GetContent("period_count")
			assert.NilError(t, err)
			assert.Equal(t, fv.Impl.(int64), test.defaultPeriodCount)
		}
	})
}

func TestAssignmentPayClaim(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	balances = append(balances, NewBalance())

	// roles
	proposer := env.Members[0]
	assignee := env.Members[1]
	closer := env.Members[2]

	t.Run("Test Assignment Document Proposal", func(t *testing.T) {

		tests := []struct {
			name       string
			roleTitle  string
			title      string
			role       string
			assignment string
			husd       string
			hypha      string
			hvoice     string
			usd        string
		}{
			{
				name:       "role2 - 100% commit, 70% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role2,
				assignment: assignment2,
				// husd:       "455.85 HUSD",
				// hypha:      "265.91 HYPHA",
				// hvoice:     "3039.00 HVOICE",
				// usd:        "1519.50 USD",
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)
			role := CreateRole(t, env, proposer, closer, test.role)

			trxID, err := dao.ProposeAssignment(env.ctx, &env.api, env.DAO, proposer.Member, assignee.Member, role.Hash, env.Periods[0].Hash, test.assignment)
			t.Log("Assignment proposed: ", trxID)
			assert.NilError(t, err)

			// retrieve the document we just created
			assignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, assignment.Creator, proposer.Member)

			fv, err := assignment.GetContent("title")
			assert.NilError(t, err)
			assert.Equal(t, fv.String(), test.title)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// root 		---proposal---> 	propDocument
			// member 		---owns-------> 	propDocument
			// propDocument ---ownedby----> 	member
			checkEdge(t, env, env.Root, assignment, eos.Name("proposal"))
			checkEdge(t, env, proposer.Doc, assignment, eos.Name("owns"))
			checkEdge(t, env, assignment, proposer.Doc, eos.Name("ownedby"))

			ballot, err := assignment.GetContent("ballot_id")
			assert.NilError(t, err)
			voteToPassTD(t, env, ballot.Impl.(eos.Name))

			t.Log("Member: ", closer.Member, " is closing assignment proposal	: ", assignment.Hash.String())
			_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, assignment.Hash)
			assert.NilError(t, err)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// update graph edges:
			//  member          ---- assigned           ---->   role_assignment
			//  role_assignment ---- assignee           ---->   member
			//  role_assignment ---- role               ---->   role
			//  role            ---- role_assignment    ---->   role_assignment
			checkEdge(t, env, assignee.Doc, assignment, eos.Name("assigned"))
			checkEdge(t, env, assignment, assignee.Doc, eos.Name("assignee"))
			checkEdge(t, env, assignment, role, eos.Name("role"))
			checkEdge(t, env, role, assignment, eos.Name("assignment"))

			//  root ---- passedprops        ---->   role_assignment
			checkEdge(t, env, env.Root, assignment, eos.Name("passedprops"))

			t.Log("Waiting for a period to lapse...")
			pause(t, env.PeriodPause, "", "Waiting...")

			_, err = ClaimNextPeriod(t, env, assignee.Member, assignment)
			assert.NilError(t, err)

			fetchedAssignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("assignment"))

			husd, err := fetchedAssignment.GetContent("husd_salary_per_phase")
			assert.NilError(t, err)

			hypha, err := fetchedAssignment.GetContent("hypha_salary_per_phase")
			assert.NilError(t, err)

			hvoice, err := fetchedAssignment.GetContent("hvoice_salary_per_phase")
			assert.NilError(t, err)

			// first payment is a partial payment, so should be less than the amount on the assignment record
			// SEEDS escrow payment should be greater than zero
			payments = append(payments, CalcLastPayment(t, env, balances[len(balances)-1], assignee.Member))
			balances = append(balances, GetBalance(t, env, assignee.Member))
			assert.Assert(t, hypha.Impl.(*eos.Asset).Amount >= payments[len(payments)-1].Hypha.Amount)
			assert.Assert(t, husd.Impl.(*eos.Asset).Amount >= payments[len(payments)-1].Husd.Amount)
			t.Log("Hvoice from payment      : ", strconv.Itoa(int(payments[len(payments)-1].Hvoice.Amount)))
			t.Log("Hvoice from assignment   : ", strconv.Itoa(int(hvoice.Impl.(*eos.Asset).Amount)))
			assert.Assert(t, hvoice.Impl.(*eos.Asset).Amount+100 >= payments[len(payments)-1].Hvoice.Amount)
			assert.Assert(t, payments[len(payments)-1].SeedsEscrow.Amount > 0)

			t.Log("Waiting for a period to lapse...")
			pause(t, env.PeriodPause, "", "Waiting...")

			_, err = ClaimNextPeriod(t, env, assignee.Member, assignment)
			assert.NilError(t, err)

			// 2nd payment should be equal to the payment on the assignment record
			// 2nd SEEDS escrow payment should be greater than the first one
			payments = append(payments, CalcLastPayment(t, env, balances[len(balances)-1], assignee.Member))
			balances = append(balances, GetBalance(t, env, assignee.Member))
			assert.Equal(t, hypha.Impl.(*eos.Asset).Amount, payments[len(payments)-1].Hypha.Amount)
			assert.Equal(t, husd.Impl.(*eos.Asset).Amount, payments[len(payments)-1].Husd.Amount)
			assert.Equal(t, hvoice.Impl.(*eos.Asset).Amount, payments[len(payments)-1].Hvoice.Amount)
			assert.Assert(t, payments[len(payments)-1].SeedsEscrow.Amount >= payments[len(payments)-2].SeedsEscrow.Amount)
		}
	})
}

const assignment1 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
              "label": "url",
              "value": [
                  "string",
                  "https://dho.hypha.earth"
              ]
            },
            {
                "label": "annual_usd_salary",
                "value": [
                    "asset",
                    "150000.00 USD"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    9
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    100
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    100
                ]
            }
        ]
    ]
}`

const assignment2 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    25
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    100
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    70
                ]
            }
        ]
    ]
}`

const assignment3 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    9
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    75
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    50
                ]
            }
        ]
    ]
}`

const assignment4 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    9
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    51
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    100
                ]
            }
        ]
    ]
}`

const assignment5 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    9
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    25
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    50
                ]
            }
        ]
    ]
}`

const assignment6 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    9
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    100
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    15
                ]
            }
        ]
    ]
}`

const assignment7 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    9
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    10
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    0
                ]
            }
        ]
    ]
}`

const assignment8 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    100
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    100
                ]
            }
        ]
    ]
}`

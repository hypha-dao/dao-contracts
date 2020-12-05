package dao_test

import (
	"testing"

	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-contracts/dao-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"gotest.tools/assert"
)

func TestAssignmentProposalDocument(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	// var env Environment
	env = SetupEnvironment(t)

	// roles
	proposer := env.Members[0]
	assignee := env.Members[1]
	closer := env.Members[2]

	t.Run("Configuring the DAO environment: ", func(t *testing.T) {
		t.Log(env.String())
		t.Log("\nDAO Environment Setup complete\n")
	})

	t.Run("Test Assignment Document Proposal", func(t *testing.T) {

		tests := []struct {
			name       string
			roleTitle  string
			title      string
			role       string
			assignment string
		}{
			{
				name:       "Basketweaver",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role1_document,
				assignment: assignment1_document,
			},
		}

		// call propose with a role proposal
		// vote on the proposal
		// close the proposal
		// ensure that the proposal closed and the appropriate edges exist

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)
			role := CreateRole(t, env, proposer, closer, test.roleTitle, test.role)

			trxID, err := dao.ProposeAssignment(env.ctx, &env.api, env.DAO, proposer.Member, assignee.Member, role.Hash, test.assignment)
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
		}
	})
}

const assignment1_document = `{
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
                "label": "start_period",
                "value": [
                    "int64",
                    0
                ]
            },
            {
                "label": "end_period",
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
                    50
                ]
            }
        ]
    ]
}`

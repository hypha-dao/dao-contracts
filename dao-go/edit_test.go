package dao

import (
	"encoding/json"
	"testing"

	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"gotest.tools/assert"
)

func TestEditProposal(t *testing.T) {
	t.Skip("Skipping failing test")
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	// roles
	proposer := env.Members[0]
	closer := env.Members[2]

	t.Run("Test Edit Document Proposal", func(t *testing.T) {

		tests := []struct {
			name        string
			original    string
			edit        string
			editedField string
			beforeValue string
			afterValue  string
		}{
			{
				name:        "Basketweaver",
				original:    original,
				edit:        edit,
				editedField: "clause_hardware_threshold",
				beforeValue: "50000000.0000 SEEDS",
				afterValue:  "25000000.0000 SEEDS",
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)
			var originalDoc docgraph.Document
			err := json.Unmarshal([]byte(test.original), &originalDoc)
			assert.NilError(t, err)

			_, err = Propose(env.ctx, &env.api, env.DAO, proposer.Member, Proposal{
				Proposer:      proposer.Member,
				ProposalType:  eos.Name("attestation"),
				Publish:       true,
				ContentGroups: originalDoc.ContentGroups,
			})
			assert.NilError(t, err)

			t.Log("Policy document proposed")

			// retrieve the document we just created
			attestation, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, attestation.Creator, proposer.Member)

			beforeThreshold, err := attestation.GetContent(test.editedField)
			assert.NilError(t, err)

			expectedBeforeThreshold, _ := eos.NewAssetFromString(test.beforeValue)
			assert.Equal(t, beforeThreshold.Impl.(*eos.Asset).Amount, expectedBeforeThreshold.Amount)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// root 		---proposal---> 	propDocument
			// member 		---owns-------> 	propDocument
			// propDocument ---ownedby----> 	member
			checkEdge(t, env, env.Root, attestation, eos.Name("proposal"))
			checkEdge(t, env, proposer.Doc, attestation, eos.Name("owns"))
			checkEdge(t, env, attestation, proposer.Doc, eos.Name("ownedby"))

			err = voteToPassTD(t, env, attestation, closer)
			assert.NilError(t, err)

			_, err = ProposeEdit(env.ctx, &env.api, env.DAO, proposer.Member, attestation, test.edit)
			assert.NilError(t, err)

			t.Log("EDIT to Policy document proposed")

			// pause(t, env.ChainResponsePause, "Building block...", "Waiting")

			// retrieve the document we just created
			edit, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Assert(t, edit.ID > attestation.ID)
			assert.Equal(t, edit.Creator, proposer.Member)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// root 		---proposal---> 	propDocument
			// member 		---owns-------> 	propDocument
			// propDocument ---ownedby----> 	member
			checkEdge(t, env, env.Root, edit, eos.Name("proposal"))
			checkEdge(t, env, proposer.Doc, edit, eos.Name("owns"))
			checkEdge(t, env, edit, proposer.Doc, eos.Name("ownedby"))

			err = voteToPassTD(t, env, edit, closer)
			assert.NilError(t, err)

			merged, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("attestation"))
			assert.NilError(t, err)
			assert.Equal(t, edit.Creator, proposer.Member)

			afterThreshold, err := merged.GetContent(test.editedField)
			assert.NilError(t, err)
			expectedAfterThreshold, _ := eos.NewAssetFromString(test.afterValue)
			assert.Equal(t, afterThreshold.Impl.(*eos.Asset).Amount, expectedAfterThreshold.Amount)

		}
	})
}

const original = `{
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
                    "Security specs for Hypha treasuries"
                ]
			},
			{
                "label": "description",
                "value": [
                    "string",
                    "This proposal specifies some key policies for Hypha treasuries"
                ]
            },
            {
                "label": "clause_multisig",
                "value": [
                    "string",
                    "Hypha treasuries should all use at least 3 of 5 multisig"
                ]
            },
            {
                "label": "clause_hardware",
                "value": [
                    "string",
                    "Hardware wallets should be used for any treasury over the value of 50 million Seeds"
                ]
			},
			{
                "label": "clause_hardware_threshold",
                "value": [
                    "asset",
                    "50000000.0000 SEEDS"
                ]
			}
        ]
    ]
}`

const edit = `{
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
                    "Update hardware threshold"
                ]
			},
			{
                "label": "description",
                "value": [
                    "string",
                    "This proposal updates the hardware requirement threshold from 50 million to 25 million"
                ]
            },            
            {
                "label": "clause_hardware",
                "value": [
                    "string",
                    "Hardware wallets should be used for any treasury over the value of 25 million Seeds"
                ]
			},
			{
                "label": "clause_hardware_threshold",
                "value": [
                    "asset",
                    "25000000.0000 SEEDS"
                ]
			}
        ]
    ]
}`

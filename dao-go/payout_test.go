package dao

import (
	"encoding/json"
	"testing"

	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"gotest.tools/assert"
)

func TestPayoutProposal(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	// roles
	proposer := env.Members[0]
	closer := env.Members[2]

	t.Run("Test Payout Proposal", func(t *testing.T) {

		tests := []struct {
			name                string
			title               string
			recipient           Member
			usdAmount           string
			deferred            int64
			payout              string
			expectedHusd        string
			expectedHvoice      string
			expectedHypha       string
			expectedSeedsEscrow string
		}{
			{
				name:                "payout1 - basic",
				title:               "Down payment on a new farm",
				recipient:           env.Members[0],
				usdAmount:           "10000.00 USD",
				deferred:            75,
				payout:              payout1,
				expectedHusd:        "2500.00 HUSD",
				expectedHvoice:      "10000.00 HVOICE",
				expectedHypha:       "1875.00 HYPHA",
				expectedSeedsEscrow: "380069.2480 SEEDS",
			},
			{
				name:                "payout1 - small amount",
				title:               "Down payment on a new farm",
				recipient:           env.Members[1],
				usdAmount:           "7.25 USD",
				deferred:            55,
				payout:              payout1,
				expectedHusd:        "3.26 HUSD",
				expectedHvoice:      "7.25 HVOICE",
				expectedHypha:       "0.99 HYPHA",
				expectedSeedsEscrow: "201.6900 SEEDS",
			},
			{
				name:                "payout1 - no deferred",
				title:               "Down payment on a new farm",
				recipient:           env.Members[2],
				usdAmount:           "155.23 USD",
				deferred:            0,
				payout:              payout1,
				expectedHusd:        "155.23 HUSD",
				expectedHvoice:      "155.23 HVOICE",
				expectedHypha:       "0.00 HYPHA",
				expectedSeedsEscrow: "0.0000 SEEDS",
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)

			proposalAmount, _ := eos.NewAssetFromString(test.usdAmount)
			trxID, err := ProposePayout(env.ctx, &env.api, env.DAO, proposer.Member,
				test.recipient.Member, proposalAmount, test.deferred, test.payout)
			t.Log("Payout proposed: ", trxID)
			assert.NilError(t, err)

			// retrieve the document we just created
			payout, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, payout.Creator, proposer.Member)

			fv, err := payout.GetContent("title")
			assert.NilError(t, err)
			assert.Equal(t, fv.String(), test.title)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// root 		---proposal---> 	propDocument
			// member 		---owns-------> 	propDocument
			// propDocument ---ownedby----> 	member
			checkEdge(t, env, env.Root, payout, eos.Name("proposal"))
			checkEdge(t, env, proposer.Doc, payout, eos.Name("owns"))
			checkEdge(t, env, payout, proposer.Doc, eos.Name("ownedby"))

			voteToPassTD(t, env, payout)

			t.Log("Member: ", closer.Member, " is closing payout proposal	: ", payout.Hash.String())
			_, err = CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, payout.Hash)
			assert.NilError(t, err)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// update graph edges:
			//  member          ---- payout           ---->   payout
			//  dho 			---- payout           ---->   payout
			checkEdge(t, env, test.recipient.Doc, payout, eos.Name("payout"))
			checkEdge(t, env, env.Root, payout, eos.Name("payout"))

			//  root ---- passedprops        ---->   payout
			checkEdge(t, env, env.Root, payout, eos.Name("passedprops"))

			// there should also be an edge from the payout to a payment
			payment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("payment"))
			assert.NilError(t, err)

			checkEdge(t, env, payout, payment, eos.Name("payment"))

			expectedHusdAsset, _ := eos.NewAssetFromString(test.expectedHusd)
			expectedHvoiceAsset, _ := eos.NewAssetFromString(test.expectedHvoice)
			expectedHyphaAsset, _ := eos.NewAssetFromString(test.expectedHypha)
			// expectedSeedsEscrowAsset, _ := eos.NewAssetFromString(test.expectedSeedsEscrow)

			balance := HelperGetBalance(t, env, test.recipient.Member)
			assert.Equal(t, balance.Husd.Amount, expectedHusdAsset.Amount)
			assert.Equal(t, balance.Hvoice.Amount, expectedHvoiceAsset.Amount + eos.Int64(env.GenesisHVOICE))
			assert.Equal(t, balance.Hypha.Amount, expectedHyphaAsset.Amount)
			// assert.Equal(t, balance.SeedsEscrow.Amount, expectedSeedsEscrowAsset.Amount)

			balances = append(balances, balance)
		}
	})
}

func TestPayoutHistoricalPeriod(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	// roles
	proposer := env.Members[0]
	closer := env.Members[2]

	t.Run("Test Payout Proposal", func(t *testing.T) {

		tests := []struct {
			name                string
			title               string
			recipient           Member
			usdAmount           string
			deferred            int64
			payout              string
			expectedHusd        string
			expectedHvoice      string
			expectedHypha       string
			expectedSeedsEscrow string
		}{
			{
				name:                "payout1 - basic",
				title:               "Down payment on a new farm",
				recipient:           env.Members[0],
				usdAmount:           "10000.00 USD",
				deferred:            75,
				payout:              payout_historical,
				expectedHusd:        "2500.00 HUSD",
				expectedHvoice:      "10000.00 HVOICE",
				expectedHypha:       "1875.00 HYPHA",
				expectedSeedsEscrow: "380069.2480 SEEDS",
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)

			proposalAmount, _ := eos.NewAssetFromString(test.usdAmount)
			_, err := ProposePayoutWithPeriod(env.ctx, &env.api, env.DAO, proposer.Member,
				test.recipient.Member, env.Periods[0].Hash, proposalAmount, test.deferred, test.payout)
			assert.NilError(t, err)

			// retrieve the document we just created
			payout, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, payout.Creator, proposer.Member)

			fv, err := payout.GetContent("title")
			assert.NilError(t, err)
			assert.Equal(t, fv.String(), test.title)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// root 		---proposal---> 	propDocument
			// member 		---owns-------> 	propDocument
			// propDocument ---ownedby----> 	member
			checkEdge(t, env, env.Root, payout, eos.Name("proposal"))
			checkEdge(t, env, proposer.Doc, payout, eos.Name("owns"))
			checkEdge(t, env, payout, proposer.Doc, eos.Name("ownedby"))

			voteToPassTD(t, env, payout)

			t.Log("Member: ", closer.Member, " is closing payout proposal	: ", payout.Hash.String())
			_, err = CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, payout.Hash)
			assert.NilError(t, err)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// update graph edges:
			//  member          ---- payout           ---->   payout
			//  dho 			---- payout           ---->   payout
			checkEdge(t, env, test.recipient.Doc, payout, eos.Name("payout"))
			checkEdge(t, env, env.Root, payout, eos.Name("payout"))

			//  root ---- passedprops        ---->   payout
			checkEdge(t, env, env.Root, payout, eos.Name("passedprops"))

			// there should also be an edge from the payout to a payment
			payment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("payment"))
			assert.NilError(t, err)

			checkEdge(t, env, payout, payment, eos.Name("payment"))

			expectedHusdAsset, _ := eos.NewAssetFromString(test.expectedHusd)
			expectedHvoiceAsset, _ := eos.NewAssetFromString(test.expectedHvoice)
			expectedHyphaAsset, _ := eos.NewAssetFromString(test.expectedHypha)
			// expectedSeedsEscrowAsset, _ := eos.NewAssetFromString(test.expectedSeedsEscrow)

			balance := HelperGetBalance(t, env, test.recipient.Member)
			assert.Equal(t, balance.Husd.Amount, expectedHusdAsset.Amount)
			assert.Equal(t, balance.Hvoice.Amount, expectedHvoiceAsset.Amount + eos.Int64(env.GenesisHVOICE))
			assert.Equal(t, balance.Hypha.Amount, expectedHyphaAsset.Amount)
			// assert.Equal(t, balance.SeedsEscrow.Amount, expectedSeedsEscrowAsset.Amount)

			balances = append(balances, balance)
		}
	})
}

func TestCustomPayout(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	proposer := env.Members[0]
	closer := env.Members[2]

	t.Run("Test Custom Payout Proposal", func(t *testing.T) {

		tests := []struct {
			name                string
			title               string
			recipient           Member
			payout              string
			expectedHusd        string
			expectedHvoice      string
			expectedHypha       string
			expectedSeedsEscrow string
		}{
			{
				name:                "payout1 - custom",
				title:               "Custom payout test",
				recipient:           env.Members[0],
				payout:              payout_custom,
				expectedHusd:        "12.50 HUSD",
				expectedHvoice:      "1.50 HVOICE",
				expectedHypha:       "0.00 HYPHA",
				expectedSeedsEscrow: "400.0000 SEEDS",
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)
			var payoutDoc docgraph.Document
			err := json.Unmarshal([]byte(test.payout), &payoutDoc)
			assert.NilError(t, err)

			// inject the assignee in the first content group of the document
			payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
				Label: "recipient",
				Value: &docgraph.FlexValue{
					BaseVariant: eos.BaseVariant{
						TypeID: docgraph.GetVariants().TypeID("name"),
						Impl:   test.recipient,
					}},
			})

			trxID, err := Propose(env.ctx, &env.api, env.DAO, proposer.Member, Proposal{
				Proposer:      proposer.Member,
				ProposalType:  eos.Name("payout"),
				ContentGroups: payoutDoc.ContentGroups,
			})
			assert.NilError(t, err)

			t.Log("Payout proposed: ", trxID)

			// retrieve the document we just created
			payout, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, payout.Creator, proposer.Member)

			fv, err := payout.GetContent("title")
			assert.NilError(t, err)
			assert.Equal(t, fv.String(), test.title)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// root 		---proposal---> 	propDocument
			// member 		---owns-------> 	propDocument
			// propDocument ---ownedby----> 	member
			checkEdge(t, env, env.Root, payout, eos.Name("proposal"))
			checkEdge(t, env, proposer.Doc, payout, eos.Name("owns"))
			checkEdge(t, env, payout, proposer.Doc, eos.Name("ownedby"))

			voteToPassTD(t, env, payout)

			t.Log("Member: ", closer.Member, " is closing payout proposal	: ", payout.Hash.String())
			_, err = CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, payout.Hash)
			assert.NilError(t, err)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// update graph edges:
			//  member          ---- payout           ---->   payout
			//  dho 			---- payout           ---->   payout
			checkEdge(t, env, test.recipient.Doc, payout, eos.Name("payout"))
			checkEdge(t, env, env.Root, payout, eos.Name("payout"))

			//  root ---- passedprops        ---->   payout
			checkEdge(t, env, env.Root, payout, eos.Name("passedprops"))

			// there should also be an edge from the payout to a payment
			payment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("payment"))
			assert.NilError(t, err)

			checkEdge(t, env, payout, payment, eos.Name("payment"))

			expectedHusdAsset, _ := eos.NewAssetFromString(test.expectedHusd)
			expectedHvoiceAsset, _ := eos.NewAssetFromString(test.expectedHvoice)
			expectedHyphaAsset, _ := eos.NewAssetFromString(test.expectedHypha)
			// expectedSeedsEscrowAsset, _ := eos.NewAssetFromString(test.expectedSeedsEscrow)

			balance := HelperGetBalance(t, env, test.recipient.Member)
			assert.Equal(t, balance.Husd.Amount, expectedHusdAsset.Amount)
			assert.Equal(t, balance.Hvoice.Amount, expectedHvoiceAsset.Amount + eos.Int64(env.GenesisHVOICE))
			assert.Equal(t, balance.Hypha.Amount, expectedHyphaAsset.Amount)
			// assert.Equal(t, balance.SeedsEscrow.Amount, expectedSeedsEscrowAsset.Amount)

			balances = append(balances, balance)
		}
	})
}

func TestUnknownAssetPayout(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	proposer := env.Members[0]
	closer := env.Members[2]

	t.Run("Test Custom Unknown Payout Proposal", func(t *testing.T) {

		tests := []struct {
			name      string
			title     string
			recipient Member
			payout    string
		}{
			{
				name:      "payout1 - custom",
				title:     "Unknown asset test",
				recipient: env.Members[0],
				payout:    payout_unknown_asset,
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)
			var payoutDoc docgraph.Document
			err := json.Unmarshal([]byte(test.payout), &payoutDoc)
			assert.NilError(t, err)

			// inject the assignee in the first content group of the document
			payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
				Label: "recipient",
				Value: &docgraph.FlexValue{
					BaseVariant: eos.BaseVariant{
						TypeID: docgraph.GetVariants().TypeID("name"),
						Impl:   test.recipient,
					}},
			})

			trxID, err := Propose(env.ctx, &env.api, env.DAO, proposer.Member, Proposal{
				Proposer:      proposer.Member,
				ProposalType:  eos.Name("payout"),
				ContentGroups: payoutDoc.ContentGroups,
			})
			assert.NilError(t, err)

			t.Log("Payout proposed: ", trxID)

			// retrieve the document we just created
			payout, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, payout.Creator, proposer.Member)

			fv, err := payout.GetContent("title")
			assert.NilError(t, err)
			assert.Equal(t, fv.String(), test.title)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// root 		---proposal---> 	propDocument
			// member 		---owns-------> 	propDocument
			// propDocument ---ownedby----> 	member
			checkEdge(t, env, env.Root, payout, eos.Name("proposal"))
			checkEdge(t, env, proposer.Doc, payout, eos.Name("owns"))
			checkEdge(t, env, payout, proposer.Doc, eos.Name("ownedby"))

			voteToPassTD(t, env, payout)

			t.Log("Member: ", closer.Member, " is closing payout proposal	: ", payout.Hash.String())
			_, err = CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, payout.Hash)
			assert.ErrorContains(t, err, "Unknown")
		}
	})
}

const payout1 = `{
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
                    "Down payment on a new farm"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "I am going to purchase the farm to grow vegetables to sell for Seeds"
                ]
            }
        ]
    ]
}`

const payout_custom = `{
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
                    "Custom payout test"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "I am going to purchase the farm to grow vegetables to sell for Seeds"
                ]
			},
			{
                "label": "custom_husd_amount",
                "value": [
                    "asset",
                    "12.50 HUSD"
                ]
			},
			{
                "label": "custom_hvoice_amount",
                "value": [
                    "asset",
                    "1.50 HVOICE"
                ]
			},
			{
                "label": "custom_seeds_escrow_amount",
                "value": [
                    "asset",
                    "400.0000 DSEEDS"
                ]
			}
        ]
    ]
}`

const payout_unknown_asset = `{
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
                    "Unknown asset test"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "I am going to purchase the farm to grow vegetables to sell for Seeds"
                ]
			},
			{
                "label": "custom_unknown_amount",
                "value": [
                    "asset",
                    "12.50 UNKNOWN"
                ]
			}
        ]
    ]
}`

const payout_historical = `{
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
                    "Down payment on a new farm"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "I am going to purchase the farm to grow vegetables to sell for Seeds"
                ]
			}
        ]
    ]
}`

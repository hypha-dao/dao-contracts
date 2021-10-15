package dao

import (
	"testing"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"gotest.tools/assert"
)

func TestCloseOldProposal(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	// var env Environment
	env = SetupEnvironmentWithFlags(t, false, true)

	t.Run("Configuring the DAO environment: ", func(t *testing.T) {
		t.Log(env.String())
		t.Log("\nDAO Environment Setup complete\n")
	})

	t.Run("Test old ballots are still closeable", func(t *testing.T) {
		t.Skip("Skipping failing test")
		var doc docgraph.Document

		ExecuteDocgraphCall(t, env, func() {
			var err error
			doc, err = docgraph.CreateDocument(
				env.ctx,
				&env.api,
				env.DAO,
				env.Alice.Member,
				"./fixtures/proposal-with-old-ballot.json",
			)
			assert.NilError(t, err)

			docgraph.CreateEdge(
				env.ctx,
				&env.api,
				env.DAO,
				env.Alice.Member,
				env.Root.Hash,
				doc.Hash,
				"proposal",
			)
		})

		// Simulate the old interaction of dao when creating a ballot
		_, err := CreateBallot(env.ctx, &env.api, env.TelosDecide, env.DAO, "hypha1....1g")
		assert.NilError(t, err)
		_, err = OpenBallot(env.ctx, &env.api, env.TelosDecide, env.DAO, "hypha1....1g", 2)
		assert.NilError(t, err)
		eostest.Pause(time.Second*3, "", "Waiting before closing")

		// Close proposal
		t.Log("Member: ", env.Alice.Member, " is closing role proposal	: ", doc.Hash.String())
		_, err = CloseProposal(env.ctx, &env.api, env.DAO, env.Alice.Member, doc.Hash)
		assert.NilError(t, err)
	})
}

func voteToPassOldBallot(t *testing.T, env *Environment, ballot eos.Name) {
	t.Log("Voting all members to 'pass' on ballot: " + ballot)
	_, err := TelosDecideVote(env.ctx, &env.api, env.TelosDecide, env.Alice.Member, ballot, eos.Name("pass"))
	assert.NilError(t, err)
	for _, member := range env.Members {
		_, err = TelosDecideVote(env.ctx, &env.api, env.TelosDecide, member.Member, ballot, eos.Name("pass"))
		assert.NilError(t, err)
	}
}

func AssertDifferentLastTally(t *testing.T, tally docgraph.Document) docgraph.Document {
	eostest.Pause(time.Second * 3, "", "Waiting for block")
	lastTally, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("votetally"))
	assert.NilError(t, err)
	assert.Assert(t, tally.Hash.String() != lastTally.Hash.String())
	return lastTally
}

func AssertSameLastTally(t *testing.T, tally docgraph.Document) docgraph.Document {
	eostest.Pause(time.Second * 3, "", "Waiting for block")
	lastTally, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("votetally"))
	assert.NilError(t, err)
	assert.Assert(t, tally.Hash.String() == lastTally.Hash.String())
	return lastTally
}

func AssertTally(t *testing.T, tallyDocument docgraph.Document, passPower string, abstainPower string, failPower string) {
	assert.Assert(t, tallyDocument.IsEqual(docgraph.Document{
		ContentGroups: []docgraph.ContentGroup{
			docgraph.ContentGroup{
				docgraph.ContentItem{
					Label: "content_group_label",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("string"),
							Impl:   "pass",
						},
					},
				},
				docgraph.ContentItem{
					Label: "vote_power",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("asset"),
							Impl:   passPower,
						},
					},
				},
			},
			docgraph.ContentGroup{
				docgraph.ContentItem{
					Label: "content_group_label",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("string"),
							Impl:   "abstain",
						},
					},
				},
				docgraph.ContentItem{
					Label: "vote_power",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("asset"),
							Impl:   abstainPower,
						},
					},
				},
			},
			docgraph.ContentGroup{
				docgraph.ContentItem{
					Label: "content_group_label",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("string"),
							Impl:   "fail",
						},
					},
				},
				docgraph.ContentItem{
					Label: "vote_power",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("asset"),
							Impl:   failPower,
						},
					},
				},
			},
			docgraph.ContentGroup{
				docgraph.ContentItem{
					Label: "content_group_label",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("string"),
							Impl:   "system",
						},
					},
				},
				docgraph.ContentItem{
					Label: "type",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("name"),
							Impl:   "vote.tally",
						},
					},
				},
				docgraph.ContentItem{
					Label: "node_label",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("string"),
							Impl:   "VoteTally",
						},
					},
				},
			},
		},
	}))
}

func AssertVote(t *testing.T, voteDocument docgraph.Document, voter string, votePower string, vote string, before, after time.Time, notes string) {
	// date should be between before and after
	dateNode, err := voteDocument.GetContentFromGroup("vote", "date")
	assert.NilError(t, err)
	dateString := dateNode.String()

	// Based off time.RFC3339
	date, err := time.Parse("2006-01-02T15:04:05.000", dateString)
	assert.NilError(t, err)

	assert.Assert(t, before.Unix() <= int64(date.Unix()) && after.Unix() >= int64(date.Unix()))

	assert.Assert(t, voteDocument.IsEqual(docgraph.Document{
		ContentGroups: []docgraph.ContentGroup{
			docgraph.ContentGroup{
				docgraph.ContentItem{
					Label: "content_group_label",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("string"),
							Impl:   "vote",
						},
					},
				},
				docgraph.ContentItem{
					Label: "voter",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("name"),
							Impl:   voter,
						},
					},
				},
				docgraph.ContentItem{
					Label: "vote_power",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("asset"),
							Impl:   votePower,
						},
					},
				},
				docgraph.ContentItem{
					Label: "vote",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("string"),
							Impl:   vote,
						},
					},
				},
				docgraph.ContentItem{
					Label: "date",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("time_point"),
							Impl:   dateNode.String(),
						},
					},
				},
				docgraph.ContentItem{
					Label: "notes",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("string"),
							Impl:   notes,
						},
					},
				},
			},
			docgraph.ContentGroup{
				docgraph.ContentItem{
					Label: "content_group_label",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("string"),
							Impl:   "system",
						},
					},
				},
				docgraph.ContentItem{
					Label: "type",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("name"),
							Impl:   "vote",
						},
					},
				},
				docgraph.ContentItem{
					Label: "node_label",
					Value: &docgraph.FlexValue{
						BaseVariant: eos.BaseVariant{
							TypeID: docgraph.GetVariants().TypeID("string"),
							Impl:   "Vote: " + vote,
						},
					},
				},
			},
		},
	}))
}

package dao_test

import (
	"testing"
	"time"

	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-contracts/dao-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"gotest.tools/assert"
)

func TestProposalDocumentVote(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	// var env Environment
	env = SetupEnvironment(t)

	// roles
	proposer := env.Members[0]
	closer := env.Members[2]

	t.Run("Configuring the DAO environment: ", func(t *testing.T) {
		t.Log(env.String())
		t.Log("\nDAO Environment Setup complete\n")
	})

	t.Run("Test Native voting for proposals", func(t *testing.T) {
		_, err := dao.ProposeRole(env.ctx, &env.api, env.DAO, proposer.Member, role1)
		assert.NilError(t, err)

		// retrieve the document we just created
		role, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
		assert.NilError(t, err)
		assert.Equal(t, role.Creator, proposer.Member)

		// Tally must exist
		voteTally, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("votetally"))
		assert.NilError(t, err)

		// verify that the edges are created correctly
		// Graph structure post creating proposal:
		// root 		---proposal---> 	propDocument
		// member 		---owns-------> 	propDocument
		// propDocument ---ownedby----> 	member
		// propDocument ---votetally-->     voteTally
		checkEdge(t, env, env.Root, role, eos.Name("proposal"))
		checkEdge(t, env, proposer.Doc, role, eos.Name("owns"))
		checkEdge(t, env, role, proposer.Doc, eos.Name("ownedby"))
		checkEdge(t, env, role, voteTally, eos.Name("votetally"))

		t.Log("Checking initial tally")
		AssertTally(t, voteTally, "0.00 HVOICE", "0.00 HVOICE", "0.00 HVOICE")

		// alice votes "pass"
		t.Log("alice votes pass")
		_, err = dao.ProposalVote(env.ctx, &env.api, env.DAO, env.Alice.Member, "pass", role.Hash)
		assert.NilError(t, err)
		voteDocument := checkLastVote(t, env, role, env.Alice)
		AssertVote(t, voteDocument, "alice", "101.00 HVOICE", "pass")

		// New tally should be different. We have 1 vote
		voteTally = AssertDifferentLastTally(t, voteTally)
		AssertTally(t, voteTally, "101.00 HVOICE", "0.00 HVOICE", "0.00 HVOICE")

		// alice changes his mind and votes "fail"
		t.Log("alice votes fail")
		_, err = dao.ProposalVote(env.ctx, &env.api, env.DAO, env.Alice.Member, "fail", role.Hash)
		assert.NilError(t, err)
		voteDocument = checkLastVote(t, env, role, env.Alice)
		AssertVote(t, voteDocument, "alice", "101.00 HVOICE", "fail")

		// New tally should be different. We have a different vote
		voteTally = AssertDifferentLastTally(t, voteTally)
		AssertTally(t, voteTally, "0.00 HVOICE", "0.00 HVOICE", "101.00 HVOICE")

		// alice decides to vote again for "fail". Just in case ;-)
		t.Log("alice votes fail (again)")
		_, err = dao.ProposalVote(env.ctx, &env.api, env.DAO, env.Alice.Member, "fail", role.Hash)
		assert.NilError(t, err)
		voteDocument = checkLastVote(t, env, role, env.Alice)
		AssertVote(t, voteDocument, "alice", "101.00 HVOICE", "fail")

		// Tally should be the same. It was the same vote
		voteTally = AssertSameLastTally(t, voteTally)
		AssertTally(t, voteTally, "0.00 HVOICE", "0.00 HVOICE", "101.00 HVOICE")

		// Member1 decides to vote pass
		_, err = dao.ProposalVote(env.ctx, &env.api, env.DAO, env.Members[0].Member, "pass", role.Hash)
		assert.NilError(t, err)
		voteDocument = checkLastVote(t, env, role, env.Members[0])
		AssertVote(t, voteDocument, "mem1.hypha", "2.00 HVOICE", "pass")

		// Tally should be different. We have a new vote
		voteTally = AssertDifferentLastTally(t, voteTally)
		AssertTally(t, voteTally, "2.00 HVOICE", "0.00 HVOICE", "101.00 HVOICE")

		// Member2 decides to vote fail
		_, err = dao.ProposalVote(env.ctx, &env.api, env.DAO, env.Members[1].Member, "fail", role.Hash)
		assert.NilError(t, err)
		voteDocument = checkLastVote(t, env, role, env.Members[1])
		AssertVote(t, voteDocument, "mem2.hypha", "2.00 HVOICE", "fail")

		// Tally should be different. We have a new vote
		voteTally = AssertDifferentLastTally(t, voteTally)
		AssertTally(t, voteTally, "2.00 HVOICE", "0.00 HVOICE", "103.00 HVOICE")

		// Member1 decides to vote pass (again)
		_, err = dao.ProposalVote(env.ctx, &env.api, env.DAO, env.Members[0].Member, "pass", role.Hash)
		assert.NilError(t, err)
		voteDocument = checkLastVote(t, env, role, env.Members[0])
		AssertVote(t, voteDocument, "mem1.hypha", "2.00 HVOICE", "pass")

		// Tally should be the same.
		voteTally = AssertSameLastTally(t, voteTally)
		AssertTally(t, voteTally, "2.00 HVOICE", "0.00 HVOICE", "103.00 HVOICE")

		// Member3 decides to vote abstain
		_, err = dao.ProposalVote(env.ctx, &env.api, env.DAO, env.Members[2].Member, "abstain", role.Hash)
		assert.NilError(t, err)
		voteDocument = checkLastVote(t, env, role, env.Members[2])
		AssertVote(t, voteDocument, "mem3.hypha", "2.00 HVOICE", "abstain")

		// Tally should be the different.
		voteTally = AssertDifferentLastTally(t, voteTally)
		AssertTally(t, voteTally, "2.00 HVOICE", "2.00 HVOICE", "103.00 HVOICE")

		t.Log("Member: ", closer.Member, " is closing role proposal	: ", role.Hash.String())
		_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, role.Hash)
		assert.NilError(t, err)

		// Member1 decides to vote pass
		_, err = dao.ProposalVote(env.ctx, &env.api, env.DAO, env.Members[0].Member, "pass", role.Hash)
		// but can't, proposal is closed
		assert.ErrorContains(t, err, "Only allowed to vote active proposals")
	})
}

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

		var doc docgraph.Document

		ExecuteDocgraphCall(t, env, func() {
			t.Log("Inside docgraph call")
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
		_, err := dao.CreateBallot(env.ctx, &env.api, env.TelosDecide, env.DAO, "hypha1....1g")
		assert.NilError(t, err)
		_, err = dao.OpenBallot(env.ctx, &env.api, env.TelosDecide, env.DAO, "hypha1....1g", 2)
		assert.NilError(t, err)
		pause(t, time.Second*3, "", "Waiting before closing")

		// Close proposal
		t.Log("Member: ", env.Alice.Member, " is closing role proposal	: ", doc.Hash.String())
		_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, env.Alice.Member, doc.Hash)
		assert.NilError(t, err)
	})
}

func voteToPassOldBallot(t *testing.T, env *Environment, ballot eos.Name) {
	t.Log("Voting all members to 'pass' on ballot: " + ballot)
	_, err := dao.TelosDecideVote(env.ctx, &env.api, env.TelosDecide, env.Alice.Member, ballot, eos.Name("pass"))
	assert.NilError(t, err)
	for _, member := range env.Members {
		_, err = dao.TelosDecideVote(env.ctx, &env.api, env.TelosDecide, member.Member, ballot, eos.Name("pass"))
		assert.NilError(t, err)
	}
}

func AssertDifferentLastTally(t *testing.T, tally docgraph.Document) docgraph.Document {
	lastTally, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("votetally"))
	assert.NilError(t, err)
	assert.Assert(t, tally.Hash.String() != lastTally.Hash.String())
	return lastTally
}

func AssertSameLastTally(t *testing.T, tally docgraph.Document) docgraph.Document {
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
		},
	}))
}

func AssertVote(t *testing.T, voteDocument docgraph.Document, voter string, votePower string, vote string) {
	assert.Assert(t, voteDocument.IsEqual(docgraph.Document{
		ContentGroups: []docgraph.ContentGroup{
			docgraph.ContentGroup{
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
			},
		},
	}))
}

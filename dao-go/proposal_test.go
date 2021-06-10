package dao

import (
	"testing"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
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
		_, err := ProposeRole(env.ctx, &env.api, env.DAO, proposer.Member, role1)
		assert.NilError(t, err)

		// retrieve the document we just created
		role, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
		assert.NilError(t, err)
		assert.Equal(t, role.Creator, proposer.Member)

		// Closing before is expired
		_, err = CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, role.Hash)
		assert.ErrorContains(t, err, "Voting is still active for this proposal")

		// Tally must exist
		voteTally, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("votetally"))
		assert.NilError(t, err)

		// Create a second document to test some cases
		_, err = ProposeRole(env.ctx, &env.api, env.DAO, proposer.Member, role2)
		assert.NilError(t, err)
		otherRole, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
		assert.NilError(t, err)
		assert.Equal(t, otherRole.Creator, proposer.Member)

		// Tally must exist
		voteTally2, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("votetally"))
		assert.NilError(t, err)

		// Both are the zero-votes tally, must be the same document (not only content).
		assert.Equal(t, voteTally.Hash.String(), voteTally2.Hash.String())

		
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
		before_date := time.Now().UTC()
		_, err = ProposalVote(env.ctx, &env.api, env.DAO, env.Alice.Member, "pass", role.Hash, "my notes")
		assert.NilError(t, err)
		eostest.Pause(time.Second * 2, "", "Waiting for block")
		voteDocument := checkLastVote(t, env, role, env.Alice)
		after_date := time.Now().UTC()
		AssertVote(t, voteDocument, "alice", "101.00 HVOICE", "pass", before_date, after_date, "my notes")

		voteTally = AssertDifferentLastTally(t, voteTally)
		AssertTally(t, voteTally, "101.00 HVOICE", "0.00 HVOICE", "0.00 HVOICE")

		// New tally should be different than the zero votes one.
		assert.Assert(t, voteTally.Hash.String() != voteTally2.Hash.String())

		// Voting on otherRole
		t.Log("alice votes pass on other role")
		_, err = ProposalVoteWithoutNotes(env.ctx, &env.api, env.DAO, env.Alice.Member, "pass", otherRole.Hash)
		// zero-votes tally should no longer exist
		eostest.Pause(time.Second * 3, "", "Waiting for block")
		_, err = docgraph.LoadDocument(env.ctx, &env.api, env.DAO, voteTally2.Hash.String())
		assert.ErrorContains(t, err, "document not found")

		voteTally2, err = docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("votetally"))
		assert.NilError(t, err)

		// Both have the same vote - Should be the same document.
		assert.Equal(t, voteTally.Hash.String(), voteTally2.Hash.String())

		// alice changes his mind and votes "fail"
		t.Log("alice votes fail")
		before_date = time.Now().UTC()
		_, err = ProposalVoteWithoutNotes(env.ctx, &env.api, env.DAO, env.Alice.Member, "fail", role.Hash)
		assert.NilError(t, err)
		voteDocument = checkLastVote(t, env, role, env.Alice)
		after_date = time.Now().UTC()
		AssertVote(t, voteDocument, "alice", "101.00 HVOICE", "fail", before_date, after_date, "")

		// New tally should be different. We have a different vote
		eostest.Pause(time.Second * 3, "", "Waiting before fetching last tally")
		voteTally = AssertDifferentLastTally(t, voteTally)
		AssertTally(t, voteTally, "0.00 HVOICE", "0.00 HVOICE", "101.00 HVOICE")

		// alice decides to vote again for "fail". Just in case ;-)
		t.Log("alice votes fail (again)")
		before_date = time.Now().UTC()
		_, err = ProposalVoteWithoutNotes(env.ctx, &env.api, env.DAO, env.Alice.Member, "fail", role.Hash)
		assert.NilError(t, err)
		voteDocument = checkLastVote(t, env, role, env.Alice)
		after_date = time.Now().UTC()
		AssertVote(t, voteDocument, "alice", "101.00 HVOICE", "fail", before_date, after_date, "")

		// Tally should be the same. It was the same vote
		voteTally = AssertSameLastTally(t, voteTally)
		AssertTally(t, voteTally, "0.00 HVOICE", "0.00 HVOICE", "101.00 HVOICE")

		// Member1 decides to vote pass
		before_date = time.Now().UTC()
		_, err = ProposalVoteWithoutNotes(env.ctx, &env.api, env.DAO, env.Members[0].Member, "pass", role.Hash)
		assert.NilError(t, err)
		voteDocument = checkLastVote(t, env, role, env.Members[0])
		after_date = time.Now().UTC()
		AssertVote(t, voteDocument, "mem1.hypha", "2.00 HVOICE", "pass", before_date, after_date, "")

		// Tally should be different. We have a new vote
		voteTally = AssertDifferentLastTally(t, voteTally)
		AssertTally(t, voteTally, "2.00 HVOICE", "0.00 HVOICE", "101.00 HVOICE")

		// Member2 decides to vote fail
		before_date = time.Now().UTC()
		_, err = ProposalVoteWithoutNotes(env.ctx, &env.api, env.DAO, env.Members[1].Member, "fail", role.Hash)
		assert.NilError(t, err)
		voteDocument = checkLastVote(t, env, role, env.Members[1])
		after_date = time.Now().UTC()
		AssertVote(t, voteDocument, "mem2.hypha", "2.00 HVOICE", "fail", before_date, after_date, "")

		// Tally should be different. We have a new vote
		voteTally = AssertDifferentLastTally(t, voteTally)
		AssertTally(t, voteTally, "2.00 HVOICE", "0.00 HVOICE", "103.00 HVOICE")

		// Member1 decides to vote pass (again)
		before_date = time.Now().UTC()
		_, err = ProposalVoteWithoutNotes(env.ctx, &env.api, env.DAO, env.Members[0].Member, "pass", role.Hash)
		assert.NilError(t, err)
		voteDocument = checkLastVote(t, env, role, env.Members[0])
		after_date = time.Now().UTC()
		AssertVote(t, voteDocument, "mem1.hypha", "2.00 HVOICE", "pass", before_date, after_date, "")

		// Tally should be the same.
		voteTally = AssertSameLastTally(t, voteTally)
		AssertTally(t, voteTally, "2.00 HVOICE", "0.00 HVOICE", "103.00 HVOICE")

		// Member3 decides to vote abstain
		before_date = time.Now().UTC()
		_, err = ProposalVoteWithoutNotes(env.ctx, &env.api, env.DAO, env.Members[2].Member, "abstain", role.Hash)
		assert.NilError(t, err)
		voteDocument = checkLastVote(t, env, role, env.Members[2])
		after_date = time.Now().UTC()
		AssertVote(t, voteDocument, "mem3.hypha", "2.00 HVOICE", "abstain", before_date, after_date, "")

		// Tally should be the different.
		voteTally = AssertDifferentLastTally(t, voteTally)
		AssertTally(t, voteTally, "2.00 HVOICE", "2.00 HVOICE", "103.00 HVOICE")

		t.Log("Member: ", closer.Member, " is closing role proposal	: ", role.Hash.String())

		eostest.Pause(env.VotingPause, "", "Waiting for ballot to finish")
		_, err = CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, role.Hash)
		assert.NilError(t, err)

		// Member1 decides to vote pass
		_, err = ProposalVoteWithoutNotes(env.ctx, &env.api, env.DAO, env.Members[0].Member, "pass", role.Hash)
		// but can't, proposal is closed
		assert.ErrorContains(t, err, "Only allowed to vote active proposals")

		// otherRole votetally should exist
		_, err = docgraph.LoadDocument(env.ctx, &env.api, env.DAO, voteTally2.Hash.String())
		assert.NilError(t, err)
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

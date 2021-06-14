package dao

import (
	"testing"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"gotest.tools/assert"
)

type Balance struct {
	SnapshotTime time.Time
	Hypha        eos.Asset
	Hvoice       eos.Asset
	SeedsEscrow  eos.Asset
	Husd         eos.Asset
}

func (b *Balance) String() string {
	s := "\n"
	s += "Time: 		" + b.SnapshotTime.String() + "\n"
	s += "Hypha: 		" + b.Hypha.String() + "\n"
	s += "Husd: 		" + b.Husd.String() + "\n"
	s += "SeedsEscrow:	" + b.SeedsEscrow.String() + "\n"
	s += "Hvoice:		" + b.Hvoice.String() + "\n"
	return s
}

func PercentageChange(old, new int) (delta float64) {
	diff := float64(new - old)
	delta = (diff / float64(old)) * 100
	return
}

func NewBalance() Balance {

	hypha, _ := eos.NewAssetFromString("0.00 HYPHA")
	hvoice, _ := eos.NewAssetFromString("0.00 HVOICE")
	husd, _ := eos.NewAssetFromString("0.00 HUSD")
	seedsEscrow, _ := eos.NewAssetFromString("0.0000 SEEDS")
	return Balance{
		SnapshotTime: time.Now(),
		Hypha:        hypha,
		Hvoice:       hvoice,
		Husd:         husd,
		SeedsEscrow:  seedsEscrow,
	}
}

func CalcLastPayment(t *testing.T, env *Environment, prevBal Balance, acct eos.AccountName) Balance {
	currentBalance := HelperGetBalance(t, env, acct)
	return Balance{
		SnapshotTime: time.Now(),
		Hypha:        currentBalance.Hypha.Sub(prevBal.Hypha),
		SeedsEscrow:  currentBalance.SeedsEscrow.Sub(prevBal.SeedsEscrow),
		Husd:         currentBalance.Husd.Sub(prevBal.Husd),
		Hvoice:       currentBalance.Hvoice.Sub(prevBal.Hvoice),
	}
}

func HelperGetBalance(t *testing.T, env *Environment, acct eos.AccountName) Balance {

	return Balance{
		SnapshotTime: time.Now(),
		Hypha:        GetBalance(env.ctx, &env.api, string(env.HyphaToken), string(acct)),
		Husd:         GetBalance(env.ctx, &env.api, string(env.HusdToken), string(acct)),
		Hvoice:       GetVotingPower(env.ctx, &env.api, env.TelosDecide, acct),
		SeedsEscrow:  GetEscrowBalance(env.ctx, &env.api, string(env.SeedsEscrow), string(acct)),
	}
}

// IsClaimed ...
func IsClaimed(env *Environment, assignment docgraph.Document, periodHash eos.Checksum256) bool {

	periodClaim, err := docgraph.LoadDocument(env.ctx, &env.api, env.DAO, periodHash.String())
	if err != nil {
		return false
	}

	exists, _ := docgraph.EdgeExists(env.ctx, &env.api, env.DAO, assignment, periodClaim, eos.Name("claimed"))
	if exists {
		return true
	}
	return false
}

type helperClaimNext struct {
	AssignmentHash eos.Checksum256 `json:"assignment_hash"`
}

// ClaimNextPeriod claims a period of pay for an assignment
func ClaimNextPeriod(t *testing.T, env *Environment, claimer eos.AccountName, assignment docgraph.Document) (string, error) {

	actions := []*eos.Action{{
		Account: env.DAO,
		Name:    eos.ActN("claimnextper"),
		Authorization: []eos.PermissionLevel{
			{Actor: claimer, Permission: eos.PN("active")},
		},
		// ActionData: eos.NewActionDataFromHexData([]byte(actionBinary)),
		ActionData: eos.NewActionData(helperClaimNext{
			AssignmentHash: assignment.Hash,
		}),
	}}

	trxID, err := eostest.ExecWithRetry(env.ctx, &env.api, actions)

	return trxID, err
}

type WithdrawData struct {
	Owner eos.AccountName `json:"owner"`
	Hash eos.Checksum256 `json:"hash"`
}

func WithdrawAssignment(env *Environment, owner eos.AccountName, hash eos.Checksum256) (string, error) {
	actions := []*eos.Action{{
		Account: env.DAO,
		Name:    eos.ActN("withdraw"),
		Authorization: []eos.PermissionLevel{
			{Actor: owner, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(WithdrawData{
			Owner:     owner,
			Hash: 		 hash,
		}),
	}}

	return eostest.ExecWithRetry(env.ctx, &env.api, actions)
}

type SuspendData struct {
	Proposer eos.AccountName `json:"proposer"`
	Hash eos.Checksum256 `json:"hash"`
	Reason string `json:"reason"`	
}

func SuspendAssignment(env *Environment, proposer eos.AccountName, hash eos.Checksum256, reason string) (string, error) {
	actions := []*eos.Action{{
		Account: env.DAO,
		Name:    eos.ActN("suspend"),
		Authorization: []eos.PermissionLevel{
			{Actor: proposer, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(SuspendData{
			Proposer:     proposer,
			Hash:         hash,
			Reason:  			reason,
		}),
	}}

	return eostest.ExecWithRetry(env.ctx, &env.api, actions)
}

type AdjustData struct {
	Issuer     eos.AccountName         `json:"issuer"`
	AdjustInfo []docgraph.ContentGroup `json:"adjust_info"`
}

func AdjustCommitment(env *Environment, assignee eos.AccountName, adjustInfo []docgraph.ContentGroup) (string, error) {

	actions := []*eos.Action{{
		Account: env.DAO,
		Name:    eos.ActN("adjustcmtmnt"),
		Authorization: []eos.PermissionLevel{
			{Actor: assignee, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(AdjustData{
			Issuer:     assignee,
			AdjustInfo: adjustInfo,
		}),
	}}

	return eostest.ExecWithRetry(env.ctx, &env.api, actions)
}

func CreateAssignment(t *testing.T, env *Environment, role *docgraph.Document,
	proposer, closer, assignee Member, content string) docgraph.Document {

	trxID, err := ProposeAssignment(env.ctx, &env.api, env.DAO, proposer.Member, assignee.Member, role.Hash, env.Periods[0].Hash, content)
	t.Log("Assignment proposed: ", trxID)
	assert.NilError(t, err)

	// retrieve the document we just created
	assignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
	assert.NilError(t, err)
	assert.Equal(t, assignment.Creator, proposer.Member)

	// verify that the edges are created correctly
	// Graph structure post creating proposal:
	// root 		---proposal---> 	propDocument
	// member 		---owns-------> 	propDocument
	// propDocument ---ownedby----> 	member
	checkEdge(t, env, env.Root, assignment, eos.Name("proposal"))
	checkEdge(t, env, proposer.Doc, assignment, eos.Name("owns"))
	checkEdge(t, env, assignment, proposer.Doc, eos.Name("ownedby"))

	err = voteToPassTD(t, env, assignment, closer)
	assert.NilError(t, err)

	eostest.Pause(1000000000, "", "Waiting before fetching role")
	assignment, err = docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("assignment"))
	assert.NilError(t, err)
	assert.Equal(t, assignment.Creator, proposer.Member)
	// verify that the edges are created correctly
	// Graph structure post creating proposal:
	// update graph edges:
	//  member          ---- assigned           ---->   role_assignment
	//  role_assignment ---- assignee           ---->   member
	//  role_assignment ---- role               ---->   role
	//  role            ---- role_assignment    ---->   role_assignment
	checkEdge(t, env, assignee.Doc, assignment, eos.Name("assigned"))
	checkEdge(t, env, assignment, assignee.Doc, eos.Name("assignee"))
	checkEdge(t, env, assignment, *role, eos.Name("role"))
	checkEdge(t, env, *role, assignment, eos.Name("assignment"))

	//  root ---- passedprops        ---->   role_assignment
	checkEdge(t, env, env.Root, assignment, eos.Name("passedprops"))
	return assignment
}

func CreateRole(t *testing.T, env *Environment, proposer, closer Member, content string) docgraph.Document {
	_, err := ProposeRole(env.ctx, &env.api, env.DAO, proposer.Member, content)
	assert.NilError(t, err)

	// retrieve the document we just created
	role, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
	assert.NilError(t, err)
	assert.Equal(t, role.Creator, proposer.Member)

	votetally, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("votetally"))
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
	checkEdge(t, env, role, votetally, eos.Name("votetally"))


	err = voteToPassTD(t, env, role, closer)
	assert.NilError(t, err)

	eostest.Pause(1000000000, "", "Waiting before fetching role")
	//Must fetch again since the hash changes on proposal close
	role, err = docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("role"))
	assert.NilError(t, err)
	assert.Equal(t, role.Creator, proposer.Member)

	// verify that the edges are created correctly
	// Graph structure post creating proposal:
	// root 		---role---> 	role
	// root     ---passedprops--->  propDocument
	checkEdge(t, env, env.Root, role, eos.Name("role"))
	checkEdge(t, env, env.Root, role, eos.Name("passedprops"))
	return role
}

func loadSeedsTablesFromProd(t *testing.T, env *Environment, prodEndpoint string) {
	prodApi := *eos.New(prodEndpoint)

	var config []SeedsExchConfigTable
	var request eos.GetTableRowsRequest
	request.Code = "tlosto.seeds"
	request.Scope = "tlosto.seeds"
	request.Table = "config"
	request.Limit = 1
	request.JSON = true
	response, _ := prodApi.GetTableRows(env.ctx, request)
	response.JSONToStructs(&config)

	action := eos.ActN("updateconfig")

	actions := []*eos.Action{{
		Account: env.SeedsExchange,
		Name:    action,
		Authorization: []eos.PermissionLevel{
			{Actor: env.SeedsExchange, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(config[0])}}

	t.Log("Copying configuration table from production for 	: " + string(env.SeedsExchange))
	_, err := eostest.ExecWithRetry(env.ctx, &env.api, actions)
	assert.NilError(t, err)

	var priceHistory []SeedsPriceHistory
	var request2 eos.GetTableRowsRequest
	request2.Code = "tlosto.seeds"
	request2.Scope = "tlosto.seeds"
	request2.Table = "pricehistory"
	request2.Limit = 1000
	request2.JSON = true
	response2, _ := prodApi.GetTableRows(env.ctx, request2)
	response2.JSONToStructs(&priceHistory)

	action = eos.ActN("inshistory")

	t.Log("Copying Seeds price history records from production for 	: " + string(env.SeedsExchange))
	for _, record := range priceHistory {
		actions := []*eos.Action{{
			Account: env.SeedsExchange,
			Name:    action,
			Authorization: []eos.PermissionLevel{
				{Actor: env.SeedsExchange, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(record)},
		}

		_, err := eostest.ExecWithRetry(env.ctx, &env.api, actions)
		assert.NilError(t, err)
	}
}

func checkEdge(t *testing.T, env *Environment, fromEdge, toEdge docgraph.Document, edgeName eos.Name) {
	exists, err := docgraph.EdgeExists(env.ctx, &env.api, env.DAO, fromEdge, toEdge, edgeName)
	assert.NilError(t, err)
	if !exists {
		t.Log("Edge does not exist	: ", fromEdge.Hash.String(), "	-- ", edgeName, "	--> 	", toEdge.Hash.String())
	}
	assert.Check(t, exists)
}

func checkLastVote(t *testing.T, env *Environment, proposal docgraph.Document, voter Member) docgraph.Document {
	// Wait 1s - If we don't, some Edges are not found.
	eostest.Pause(1000000000, "", "Waiting before fetching last vote")
	vote, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("vote"))
	assert.NilError(t, err)

	// verify that the edges are created correctly
	// Graph structure post voting:
	// voter 		---vote-------> 	vote
	// propDocument ---vote-------> 	vote
	// vote			---ownedby----> 	voter
	// vote		    ---voteon----->     propDocument
	checkEdge(t, env, voter.Doc, vote, eos.Name("vote"))
	checkEdge(t, env, proposal, vote, eos.Name("vote"))
	checkEdge(t, env, vote, voter.Doc, eos.Name("ownedby"))
	checkEdge(t, env, vote, proposal, eos.Name("voteon"))

	return vote
}

func voteToPassTD(t *testing.T, env *Environment, proposal docgraph.Document, closer Member) (error) {
	proposal_hash := proposal.Hash
	t.Log("Voting all members to 'pass' on proposal: " + proposal_hash.String())

	_, err := ProposalVoteWithoutNotes(env.ctx, &env.api, env.DAO, env.Alice.Member, "pass", proposal_hash)
	assert.NilError(t, err)
	checkLastVote(t, env, proposal, env.Alice)

	for _, member := range env.Members {
		_, err = ProposalVoteWithoutNotes(env.ctx, &env.api, env.DAO, member.Member, "pass", proposal_hash)
		assert.NilError(t, err)
		checkLastVote(t, env, proposal, member)
	}

	eostest.Pause(env.VotingPause, "", "Waiting for ballot to finish")
	t.Log("Member: ", closer.Member, " is closing proposal	: ", proposal.Hash.String())
	_, err = CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, proposal.Hash)

	return err
}

func ReplaceContent(d *docgraph.Document, label string, value *docgraph.FlexValue) error {
	for _, contentGroup := range d.ContentGroups {
		for i := range contentGroup {
			if contentGroup[i].Label == label {
				contentGroup[i].Value = value
				return nil
			}
		}
	}
	return nil
}

// Max returns the larger of x or y.
func Max(x, y float32) float32 {
	if x < y {
		return y
	}	
	return x
}

// Min returns the smaller of x or y.
func Min(x, y float32) float32 {
	if x > y {
			return y
	}
	return x
}
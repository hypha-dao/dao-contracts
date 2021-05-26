package dao

import (
	"context"
	"encoding/json"
	"fmt"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/document-graph/docgraph"
)

// CloseDocProp is closing proposals for document proposals
type CloseDocProp struct {
	ProposalHash eos.Checksum256 `json:"proposal_hash"`
}

//VoteProposal votes for a proposal (native ballot)
type VoteProposal struct {
	Voter        eos.AccountName `json:"voter"`
	ProposalHash eos.Checksum256 `json:"proposal_hash"`
	Vote         string          `json:"vote"`
	Notes		 string	         `json:"notes"`
}

// Vote represents a set of options being cast as a vote to Telos Decide
type Vote struct {
	Voter      eos.AccountName `json:"voter"`
	BallotName eos.Name        `json:"ballot_name"`
	Options    []eos.Name      `json:"options"`
}

// Proposal is a document to be proposed
type Proposal struct {
	Proposer      eos.AccountName         `json:"proposer"`
	ProposalType  eos.Name                `json:"proposal_type"`
	ContentGroups []docgraph.ContentGroup `json:"content_groups"`
}

// Propose ...
func Propose(ctx context.Context, api *eos.API,
	contract, proposer eos.AccountName, proposal Proposal) (string, error) {
	action := eos.ActN("propose")
	actions := []*eos.Action{{
		Account: contract,
		Name:    action,
		Authorization: []eos.PermissionLevel{
			{Actor: proposer, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(proposal)}}

	return eostest.ExecWithRetry(ctx, api, actions)
}

// ProposePayout ...
func ProposePayout(ctx context.Context, api *eos.API,
	contract, proposer, recipient eos.AccountName,
	usdAmount eos.Asset, deferred int64, payout string) (string, error) {

	var payoutDoc docgraph.Document
	err := json.Unmarshal([]byte(payout), &payoutDoc)
	if err != nil {
		return "error", fmt.Errorf("ProposePayout unmarshal : %v", err)
	}

	// inject the assignee in the first content group of the document
	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "recipient",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("name"),
				Impl:   recipient,
			}},
	})

	// inject the assignee in the first content group of the document
	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "usd_amount",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("asset"),
				Impl:   usdAmount,
			}},
	})

	// inject the assignee in the first content group of the document
	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "deferred_perc_x100",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("int64"),
				Impl:   deferred,
			}},
	})

	return Propose(ctx, api, contract, proposer, Proposal{
		Proposer:      proposer,
		ProposalType:  eos.Name("payout"),
		ContentGroups: payoutDoc.ContentGroups,
	})
}

// ProposePayoutWithPeriod creates a proposal for an new payout/contribution
func ProposePayoutWithPeriod(ctx context.Context, api *eos.API,
	contract, proposer, recipient eos.AccountName, endPeriod eos.Checksum256,
	usdAmount eos.Asset, deferred int64, payout string) (string, error) {

	var payoutDoc docgraph.Document
	err := json.Unmarshal([]byte(payout), &payoutDoc)
	if err != nil {
		return "error", fmt.Errorf("ProposePayout unmarshal : %v", err)
	}

	// inject the assignee in the first content group of the document
	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "recipient",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("name"),
				Impl:   recipient,
			}},
	})

	// inject the assignee in the first content group of the document
	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "usd_amount",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("asset"),
				Impl:   usdAmount,
			}},
	})

	// inject the assignee in the first content group of the document
	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "deferred_perc_x100",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("int64"),
				Impl:   deferred,
			}},
	})

	// inject the assignee in the first content group of the document
	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "end_period",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("checksum256"),
				Impl:   endPeriod,
			}},
	})

	return Propose(ctx, api, contract, proposer, Proposal{
		Proposer:      proposer,
		ProposalType:  eos.Name("payout"),
		ContentGroups: payoutDoc.ContentGroups,
	})
}

// ProposeEdit creates an edit proposal
func ProposeEdit(ctx context.Context, api *eos.API,
	contract, proposer eos.AccountName, original docgraph.Document, edit string) (string, error) {

	var editDoc docgraph.Document
	err := json.Unmarshal([]byte(edit), &editDoc)
	if err != nil {
		return "error", fmt.Errorf("ProposeEdit unmarshal : %v", err)
	}

	// inject the assignee in the first content group of the document
	editDoc.ContentGroups[0] = append(editDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "original_document",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("checksum256"),
				Impl:   original.Hash,
			}},
	})

	return Propose(ctx, api, contract, proposer, Proposal{
		Proposer:      proposer,
		ProposalType:  eos.Name("edit"),
		ContentGroups: editDoc.ContentGroups,
	})
}

// ProposeRole creates a proposal for an new assignment
func ProposeRole(ctx context.Context, api *eos.API,
	contract, proposer eos.AccountName, role string) (string, error) {

	var roleDoc docgraph.Document
	err := json.Unmarshal([]byte(role), &roleDoc)
	if err != nil {
		return "error", fmt.Errorf("ProposeRole unmarshal : %v", err)
	}

	return Propose(ctx, api, contract, proposer, Proposal{
		Proposer:      proposer,
		ProposalType:  eos.Name("role"),
		ContentGroups: roleDoc.ContentGroups,
	})
}

// ProposeAssignment creates a proposal for an new assignment
func ProposeAssignment(ctx context.Context, api *eos.API,
	contract, proposer, assignee eos.AccountName,
	roleHash, startPeriod eos.Checksum256, assignment string) (string, error) {

	var assignmentDoc docgraph.Document
	err := json.Unmarshal([]byte(assignment), &assignmentDoc)
	if err != nil {
		return "error", fmt.Errorf("ProposeAssignment unmarshal : %v", err)
	}

	// inject the role hash in the first content group of the document
	assignmentDoc.ContentGroups[0] = append(assignmentDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "role",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("checksum256"),
				Impl:   roleHash,
			}},
	})

	// // inject the period hash in the first content group of the document
	// assignmentDoc.ContentGroups[0] = append(assignmentDoc.ContentGroups[0], docgraph.ContentItem{
	// 	Label: "start_period",
	// 	Value: &docgraph.FlexValue{
	// 		BaseVariant: eos.BaseVariant{
	// 			TypeID: docgraph.GetVariants().TypeID("checksum256"),
	// 			Impl:   startPeriod,
	// 		}},
	// })

	// inject the assignee in the first content group of the document
	assignmentDoc.ContentGroups[0] = append(assignmentDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "assignee",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("name"),
				Impl:   assignee,
			}},
	})

	return Propose(ctx, api, contract, proposer, Proposal{
		Proposer:      proposer,
		ProposalType:  eos.Name("assignment"),
		ContentGroups: assignmentDoc.ContentGroups,
	})
}

func ProposeAssExtension(ctx context.Context, api *eos.API,
	contract, proposer eos.AccountName, assignmentHash string, additionalPeriods int) (string, error) {

	actionData := make(map[string]interface{})
	actionData["assignment_hash"] = assignmentHash
	actionData["additional_periods"] = additionalPeriods

	actionBinary, err := api.ABIJSONToBin(ctx, contract, eos.Name("proposeextend"), actionData)
	if err != nil {
		return string(""), fmt.Errorf("ProposeBadge : %v", err)
	}

	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("proposeextend"),
		Authorization: []eos.PermissionLevel{
			{Actor: proposer, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionDataFromHexData([]byte(actionBinary)),
	}}

	return eostest.ExecWithRetry(ctx, api, actions)
}

// ProposeBadge proposes the badge to the specified DAO contract
func ProposeBadge(ctx context.Context, api *eos.API, contract, proposer eos.AccountName, content string) (string, error) {

	action := eos.ActN("propose")

	var dump map[string]interface{}
	err := json.Unmarshal([]byte(content), &dump)
	if err != nil {
		return "error", fmt.Errorf("ProposeBadge : %v", err)
	}

	dump["proposer"] = proposer
	dump["proposal_type"] = "badge"

	actionBinary, err := api.ABIJSONToBin(ctx, contract, eos.Name(action), dump)

	actions := []*eos.Action{{
		Account: contract,
		Name:    action,
		Authorization: []eos.PermissionLevel{
			{Actor: proposer, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionDataFromHexData([]byte(actionBinary)),
	}}

	return eostest.ExecWithRetry(ctx, api, actions)
}

// ProposeBadgeAssignment proposes the badge assignment to the specified DAO contract
func ProposeBadgeAssignment(ctx context.Context, api *eos.API,
	contract, proposer, assignee eos.AccountName,
	badgeHash, startPeriod eos.Checksum256, assignment string) (string, error) {

	var badgeAssignmentDoc docgraph.Document
	err := json.Unmarshal([]byte(assignment), &badgeAssignmentDoc)
	if err != nil {
		return "error", fmt.Errorf("ProposeBadgeAssignment unmarshal : %v", err)
	}

	action := eos.ActN("propose")

	// inject the badge hash in the first content group of the document
	badgeAssignmentDoc.ContentGroups[0] = append(badgeAssignmentDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "badge",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("checksum256"),
				Impl:   badgeHash,
			}},
	})

	// inject the assignee in the first content group of the document
	badgeAssignmentDoc.ContentGroups[0] = append(badgeAssignmentDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "assignee",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("name"),
				Impl:   assignee,
			}},
	})

	actions := []*eos.Action{{
		Account: contract,
		Name:    action,
		Authorization: []eos.PermissionLevel{
			{Actor: proposer, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(Proposal{
			Proposer:      proposer,
			ProposalType:  eos.Name("assignbadge"),
			ContentGroups: badgeAssignmentDoc.ContentGroups,
		})}}

	return eostest.ExecWithRetry(ctx, api, actions)
}

func VotePass(ctx context.Context, api *eos.API, contract,
	telosDecide, voter eos.AccountName, proposal *docgraph.Document) (string, error) {

	ballot, err := proposal.GetContent("ballot_id")
	if err == nil {
		// if ballot_id exists, we are using TelosDecide
		return TelosDecideVote(ctx, api, telosDecide, voter, ballot.Impl.(eos.Name), eos.Name("pass"))
	}
	return ProposalVoteWithoutNotes(ctx, api, contract, voter, "pass", proposal.Hash)
}

// TelosDecideVote ...
func TelosDecideVote(ctx context.Context, api *eos.API,
	telosDecide, voter eos.AccountName, ballot,
	passFail eos.Name) (string, error) {

	actions := []*eos.Action{{
		Account: telosDecide,
		Name:    eos.ActN("castvote"),
		Authorization: []eos.PermissionLevel{
			{Actor: voter, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(&Vote{
			Voter:      voter,
			BallotName: ballot,
			Options:    []eos.Name{passFail},
		}),
	}}

	return eostest.ExecWithRetry(ctx, api, actions)
}

// DocumentVote ....
func DocumentVote(ctx context.Context, api *eos.API,
	contract, proposer eos.AccountName,
	badgeHash eos.Checksum256, notes string,
	startPeriod, endPeriod uint64) error {

	return nil
}

// ProposalVote
func ProposalVote(
	ctx context.Context, api *eos.API,
	contract, voter eos.AccountName,
	vote string, proposalHash eos.Checksum256, notes string) (string, error) {

	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("vote"),
		Authorization: []eos.PermissionLevel{
			{Actor: voter, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(&VoteProposal{
			Voter:        voter,
			ProposalHash: proposalHash,
			Vote:         vote,
			Notes: 		  notes,
		}),
	}}

	return eostest.ExecWithRetry(ctx, api, actions)
}

// ProposalVote
func ProposalVoteWithoutNotes(
	ctx context.Context, api *eos.API,
	contract, voter eos.AccountName,
	vote string, proposalHash eos.Checksum256) (string, error) {

	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("vote"),
		Authorization: []eos.PermissionLevel{
			{Actor: voter, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(&VoteProposal{
			Voter:        voter,
			ProposalHash: proposalHash,
			Vote:         vote,
		}),
	}}

	return eostest.ExecWithRetry(ctx, api, actions)
}

// CloseProposal ...
func CloseProposal(ctx context.Context, api *eos.API, contract, closer eos.AccountName,
	proposalHash eos.Checksum256) (string, error) {

	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("closedocprop"),
		Authorization: []eos.PermissionLevel{
			{Actor: closer, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(&CloseDocProp{
			ProposalHash: proposalHash,
		}),
	}}

	return eostest.ExecWithRetry(ctx, api, actions)
}

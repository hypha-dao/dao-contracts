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
// func Propose(ctx context.Context, api *eos.API, contract, proposer eos.AccountName, proposalType eos.Name, content string) (string, error) {

// 	action := eos.ActN("propose")

// 	actions := []*eos.Action{{
// 		Account: contract,
// 		Name:    action,
// 		Authorization: []eos.PermissionLevel{
// 			{Actor: proposer, Permission: eos.PN("active")},
// 		},
// 		ActionData: eos.NewActionData()),
// 	}}

// 	return eostest.ExecTrx(ctx, api, actions)
// }

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

	return eostest.ExecTrx(ctx, api, actions)
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
	roleHash eos.Checksum256, assignment string) (string, error) {

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

// ProposeBadgeFromFile proposes the badge to the specified DAO contract
func ProposeBadgeFromFile(ctx context.Context, api *eos.API, contract, proposer eos.AccountName, content string) (string, error) {

	action := eos.ActN("propose")

	var dump map[string]interface{}
	err := json.Unmarshal([]byte(content), &dump)
	if err != nil {
		return "error", fmt.Errorf("ProposeBadgeFromFile : %v", err)
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

	return eostest.ExecTrx(ctx, api, actions)
}

// ProposeBadgeAssignment proposes the badge assignment to the specified DAO contract
func ProposeBadgeAssignment(ctx context.Context, api *eos.API,
	contract, proposer, assignee eos.AccountName,
	badgeHash eos.Checksum256, assignment string) (string, error) {

	// data, err := ioutil.ReadFile(fileName)
	// if err != nil {
	// 	return "error", fmt.Errorf("ProposeBadgeAssignment : %v", err)
	// }

	// var roleProposal dao.RawObject
	// err := json.Unmarshal([]byte(assignment), &roleProposal)
	// assert.NilError(t, err)

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

	return eostest.ExecTrx(ctx, api, actions)
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

	return eostest.ExecTrx(ctx, api, actions)
}

// DocumentVote ....
func DocumentVote(ctx context.Context, api *eos.API,
	contract, proposer eos.AccountName,
	badgeHash eos.Checksum256, notes string,
	startPeriod, endPeriod uint64) error {

	return nil
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

	return eostest.ExecTrx(ctx, api, actions)
}

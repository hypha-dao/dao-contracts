package dao

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"strconv"
	"testing"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"github.com/spf13/viper"
)

type updateDoc struct {
	Hash  eos.Checksum256    `json:"hash"`
	Group string             `json:"group"`
	Key   string             `json:"key"`
	Value docgraph.FlexValue `json:"value"`
}

type docGroups struct {
	ContentGroups []docgraph.ContentGroup `json:"content_groups"`
}

func ReplaceContent2(groups []docgraph.ContentGroup, label string, value *docgraph.FlexValue) error {
	for _, contentGroup := range groups {
		for i := range contentGroup {
			if contentGroup[i].Label == label {
				contentGroup[i].Value = value
				return nil
			}
		}
	}
	return nil
}

func UpdateAssignments(ctx context.Context, api *eos.API, contract eos.AccountName) error {

	// get all documents
	// if type is period, update
	// documents, err := docgraph.GetAllDocuments(ctx, api, contract)
	// if err != nil {
	// 	return fmt.Errorf("cannot get all documents %v", err)
	// }

	// counter := 0
	// for _, d := range documents {
	// 	docType, err := d.GetContentFromGroup("system", "type")
	// 	if err != nil {
	// 		return fmt.Errorf("cannot get type %v", err)
	// 	}

	// 	fmt.Println("Document type: ", docType.String())
	// 	if docType.String() == "assignment" {

	// 		legacyScope, err := d.GetContentFromGroup("system", "legacy_object_scope")
	// 		if err != nil {
	// 			return fmt.Errorf("cannot get legacy scope %v", err)
	// 		}

	// 		if legacyScope.String() == "proposal" {

	// 		}

	// 		startPeriod, err := docgraph.LoadDocument(ctx, api, contract, edges[0].ToNode.String())
	// 		if err != nil {
	// 			return fmt.Errorf("cannot load document %v", err)
	// 		}

	// 		fmt.Println("Assignment: ", d.GetNodeLabel())
	// 		fmt.Println("Start Period: ", startPeriod.GetNodeLabel())

	// assignee, err := d.GetContent("assignee")
	// if err != nil {
	// 	return fmt.Errorf("cannot get assignee %v", err)
	// }

	// newNodeLabel := assignee.String() + ": " + startPeriod.GetNodeLabel()

	// actions := []*eos.Action{
	// 	{
	// 		Account: contract,
	// 		Name:    eos.ActN("updatedoc"),
	// 		Authorization: []eos.PermissionLevel{
	// 			{Actor: contract, Permission: eos.PN("active")},
	// 		},
	// 		ActionData: eos.NewActionData(updateDoc{
	// 			Hash:  d.Hash,
	// 			Group: "system",
	// 			Key:   "node_label",
	// 			Value: docgraph.FlexValue{
	// 				BaseVariant: eos.BaseVariant{
	// 					TypeID: docgraph.GetVariants().TypeID("string"),
	// 					Impl:   newNodeLabel,
	// 				},
	// 			}}),
	// 	},
	// }

	// _, err = eostest.ExecWithRetry(ctx, api, actions)
	// if err != nil {
	// 	fmt.Println("\n\nFAILED to update document: ", d.Hash.String())
	// 	fmt.Println(err)
	// 	fmt.Println()
	// }
	// time.Sleep(defaultPause())

	// 		actions2 := []*eos.Action{
	// 			{
	// 				Account: contract,
	// 				Name:    eos.ActN("updatedoc"),
	// 				Authorization: []eos.PermissionLevel{
	// 					{Actor: contract, Permission: eos.PN("active")},
	// 				},
	// 				ActionData: eos.NewActionData(updateDoc{
	// 					Hash:  d.Hash,
	// 					Group: "details",
	// 					Key:   "start_period",
	// 					Value: docgraph.FlexValue{
	// 						BaseVariant: eos.BaseVariant{
	// 							TypeID: docgraph.GetVariants().TypeID("checksum256"),
	// 							Impl:   edges[0].ToNode,
	// 						},
	// 					}}),
	// 			},
	// 		}

	// 		_, err = eostest.ExecWithRetry(ctx, api, actions2)
	// 		if err != nil {
	// 			fmt.Println("\n\nFAILED to update document: ", d.Hash.String())
	// 			fmt.Println(err)
	// 			fmt.Println()
	// 		}
	// 		time.Sleep(defaultPause())

	// 		counter++
	// 		if counter > 10 {
	// 			return nil
	// 		}
	// 	}
	// }
	return nil
}

// UpdatePeriods ...
func UpdatePeriods(ctx context.Context, api *eos.API, contract eos.AccountName) error {

	documents, err := docgraph.GetAllDocuments(ctx, api, contract)
	if err != nil {
		return fmt.Errorf("cannot get all documents %v", err)
	}

	for _, d := range documents {
		docType, err := d.GetContentFromGroup("system", "type")
		if err != nil {
			return fmt.Errorf("cannot get type %v", err)
		}

		if docType.String() == "period" {

			startTime, err := d.GetContent("start_time")
			if err != nil {
				return fmt.Errorf("cannot get start_time %v", err)
			}

			startTimePoint := startTime.Impl.(eos.TimePoint)

			fmt.Println("Start time point: ", startTimePoint.String())
			unixTime := time.Unix(int64(startTimePoint)/1000000, 0).UTC()
			fmt.Println("Starting " + unixTime.Format("2006 Jan 02"))
			fmt.Println(unixTime.Format("15:04:05 MST"))

			actions := []*eos.Action{
				// {
				// 	Account: contract,
				// 	Name:    eos.ActN("updatedoc"),
				// 	Authorization: []eos.PermissionLevel{
				// 		{Actor: contract, Permission: eos.PN("active")},
				// 	},
				// 	ActionData: eos.NewActionData(updateDoc{
				// 		Hash:  d.Hash,
				// 		Group: "system",
				// 		Key:   "node_label",
				// 		Value: docgraph.FlexValue{
				// 			BaseVariant: eos.BaseVariant{
				// 				TypeID: docgraph.GetVariants().TypeID("string"),
				// 				Impl:   "Starting " + unixTime.Format("2006 Jan 02"),
				// 			},
				// 		}}),
				// },
				// {
				// 	Account: contract,
				// 	Name:    eos.ActN("updatedoc"),
				// 	Authorization: []eos.PermissionLevel{
				// 		{Actor: contract, Permission: eos.PN("active")},
				// 	},
				// 	ActionData: eos.NewActionData(updateDoc{
				// 		Hash:  d.Hash,
				// 		Group: "system",
				// 		Key:   "readable_start_date",
				// 		Value: docgraph.FlexValue{
				// 			BaseVariant: eos.BaseVariant{
				// 				TypeID: docgraph.GetVariants().TypeID("string"),
				// 				Impl:   unixTime.Format("2006 Jan 02"),
				// 			},
				// 		}}),
				// },
				{
					Account: contract,
					Name:    eos.ActN("updatedoc"),
					Authorization: []eos.PermissionLevel{
						{Actor: contract, Permission: eos.PN("active")},
					},
					ActionData: eos.NewActionData(updateDoc{
						Hash:  d.Hash,
						Group: "system",
						Key:   "readable_start_time",
						Value: docgraph.FlexValue{
							BaseVariant: eos.BaseVariant{
								TypeID: docgraph.GetVariants().TypeID("string"),
								Impl:   unixTime.Format("15:04:05 MST"),
							},
						}}),
				},
			}

			_, err = eostest.ExecWithRetry(ctx, api, actions)
			if err != nil {
				fmt.Println("\n\nFAILED to update document: ", d.Hash.String())
				fmt.Println(err)
				fmt.Println()
			}
			time.Sleep(defaultPause())
		}

	}
	return nil

}

// CreatePretend ...
func CreatePretend(ctx context.Context, api *eos.API, contract, telosDecide, member eos.AccountName) (docgraph.Document, error) {

	roleFilename := "fixtures/role.json"
	roleData, err := ioutil.ReadFile(roleFilename)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("unable to read file: %v %v", roleFilename, err)
	}

	role, err := CreateRole(ctx, api, contract, telosDecide, member, roleData)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("unable to create role: %v", err)
	}
	fmt.Println("Created role document	: ", role.Hash.String())

	assignmentData, err := ioutil.ReadFile("fixtures/assignment.json")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("unable to read file: %v", err)
	}

	roleAssignment, err := CreateAssignment(ctx, api, contract, telosDecide, member, eos.Name("role"), eos.Name("assignment"), assignmentData)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("unable to create assignment: %v", err)
	}
	fmt.Println("Created role assignment document	: ", roleAssignment.Hash.String())

	eostest.Pause(defaultPeriodDuration(), "", "Waiting for a period to lapse")

	_, err = claimNextPeriod(ctx, api, contract, member, roleAssignment)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("unable to claim pay on assignment: %v %v", roleAssignment.Hash.String(), err)
	}

	payoutData, err := ioutil.ReadFile("fixtures/payout.json")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("unable to read payout file: %v", err)
	}

	payAmt, _ := eos.NewAssetFromString("1000.00 USD")
	payout, err := CreatePayout(ctx, api, contract, telosDecide, member, member, payAmt, 50, payoutData)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("unable to create payout: %v", err)
	}
	fmt.Println("Created payout document	: ", payout.Hash.String())

	badgeData, err := ioutil.ReadFile("fixtures/badge.json")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("unable to read badge file: %v", err)
	}

	badge, err := CreateBadge(ctx, api, contract, telosDecide, member, badgeData)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("unable to create badge: %v", err)
	}
	fmt.Println("Created badge document	: ", badge.Hash.String())

	badgeAssignmentData, err := ioutil.ReadFile("fixtures/badge-assignment.json")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("unable to read badge assignment file: %v", err)
	}

	badgeAssignment, err := CreateAssignment(ctx, api, contract, telosDecide, member, eos.Name("badge"), eos.Name("assignbadge"), badgeAssignmentData)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("unable to create badge assignment: %v", err)
	}
	fmt.Println("Created badge assignment document	: ", badgeAssignment.Hash.String())
	return roleAssignment, nil
}

type claimNext struct {
	AssignmentHash eos.Checksum256 `json:"assignment_hash"`
}

func claimNextPeriod(ctx context.Context, api *eos.API, contract, claimer eos.AccountName, assignment docgraph.Document) (string, error) {

	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("claimnextper"),
		Authorization: []eos.PermissionLevel{
			{Actor: claimer, Permission: eos.PN("active")},
		},
		// ActionData: eos.NewActionDataFromHexData([]byte(actionBinary)),
		ActionData: eos.NewActionData(claimNext{
			AssignmentHash: assignment.Hash,
		}),
	}}

	trxID, err := eostest.ExecWithRetry(ctx, api, actions)

	if err != nil {
		eostest.Pause(defaultPeriodDuration(), "", "Waiting for a period to lapse")

		actions := []*eos.Action{{
			Account: contract,
			Name:    eos.ActN("claimnextper"),
			Authorization: []eos.PermissionLevel{
				{Actor: claimer, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(claimNext{
				AssignmentHash: assignment.Hash,
			}),
		}}

		trxID, err = eostest.ExecWithRetry(ctx, api, actions)
	}

	return trxID, err
}

// EnrollMembers ...
func EnrollMembers(ctx context.Context, api *eos.API, contract eos.AccountName) {

	// re-enroll members
	index := 1
	for index < 6 {

		memberNameIn := "mem" + strconv.Itoa(index) + ".hypha"
		//memberNameIn := "member" + strconv.Itoa(index)

		newMember, err := enrollMember(ctx, api, contract, eos.AN(memberNameIn))
		if err != nil {
			fmt.Println("Error attempting to enroll : " + string(memberNameIn) + " with hash: " + newMember.Hash.String())
		}
		fmt.Println("Member enrolled : " + string(memberNameIn) + " with hash: " + newMember.Hash.String())
		fmt.Println()

		index++
	}

	johnnyhypha, err := enrollMember(ctx, api, contract, eos.AN("johnnyhypha1"))
	if err != nil {
		fmt.Println("Error attempting to enroll : johnnyhypha1 with hash: " + johnnyhypha.Hash.String())
	}
	fmt.Println("Member enrolled : johnnyhypha1 with hash: " + johnnyhypha.Hash.String())
	fmt.Println()
}

func enrollMember(ctx context.Context, api *eos.API, contract, member eos.AccountName) (docgraph.Document, error) {
	fmt.Println("Enrolling " + member + " in DAO: " + contract)

	trxID, err := Apply(ctx, api, contract, member, "apply to DAO")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error applying %v", err)
	}
	fmt.Println("Completed the apply transaction: " + trxID)

	eostest.Pause(defaultPause(), "Building block...", "")

	trxID, err = Enroll(ctx, api, contract, contract, member)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error enrolling %v", err)
	}
	fmt.Println("Completed the enroll transaction: " + trxID)

	eostest.Pause(defaultPause(), "Building block...", "")

	memberDoc, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, "member")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error enrolling %v", err)
	}

	return memberDoc, nil
}

func getSettings(ctx context.Context, api *eos.API, contract eos.AccountName) (docgraph.Document, error) {

	rootHash := string("52a7ff82bd6f53b31285e97d6806d886eefb650e79754784e9d923d3df347c91")
	if len(viper.GetString("rootHash")) > 0 {
		rootHash = viper.GetString("rootHash")
	}

	root, err := docgraph.LoadDocument(ctx, api, contract, rootHash)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("Root document not found, required for default period %v", err)
	}

	edges, err := docgraph.GetEdgesFromDocumentWithEdge(ctx, api, contract, root, eos.Name("settings"))
	if err != nil || len(edges) <= 0 {
		return docgraph.Document{}, fmt.Errorf("error retrieving settings edge %v", err)
	}

	return docgraph.LoadDocument(ctx, api, contract, edges[0].ToNode.String())
}

func getDefaultPeriod(api *eos.API, contract eos.AccountName) docgraph.Document {

	ctx := context.Background()

	root, err := docgraph.LoadDocument(ctx, api, contract, viper.GetString("rootHash"))
	if err != nil {
		panic("Root document not found, required for default period.")
	}

	edges, err := docgraph.GetEdgesFromDocumentWithEdge(ctx, api, contract, root, eos.Name("start"))
	if err != nil || len(edges) <= 0 {
		panic("Next document not found: " + edges[0].ToNode.String())
	}

	lastDocument, err := docgraph.LoadDocument(ctx, api, contract, edges[0].ToNode.String())
	if err != nil {
		panic("Next document not found: " + edges[0].ToNode.String())
	}

	index := 1
	for index < 6 {

		edges, err := docgraph.GetEdgesFromDocumentWithEdge(ctx, api, contract, lastDocument, eos.Name("next"))
		if err != nil || len(edges) <= 0 {
			panic("There are no next edges")
		}

		lastDocument, err = docgraph.LoadDocument(ctx, api, contract, edges[0].ToNode.String())
		if err != nil {
			panic("Next document not found: " + edges[0].ToNode.String())
		}

		index++
	}
	return lastDocument
}

type proposal struct {
	Proposer      eos.AccountName         `json:"proposer"`
	ProposalType  eos.Name                `json:"proposal_type"`
	ContentGroups []docgraph.ContentGroup `json:"content_groups"`
}

func proposeAndPass(ctx context.Context, api *eos.API,
	contract, telosDecide, proposer eos.AccountName, proposal proposal) (docgraph.Document, error) {
	action := eos.ActN("propose")
	actions := []*eos.Action{{
		Account: contract,
		Name:    action,
		Authorization: []eos.PermissionLevel{
			{Actor: proposer, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(proposal)}}

	trxID, err := eostest.ExecWithRetry(ctx, api, actions)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error proposeAndPass %v", err)
	}
	fmt.Println("Proposed. Transaction ID: " + trxID)

	return closeLastProposal(ctx, api, contract, telosDecide, proposer)
}

func closeLastProposal(ctx context.Context, api *eos.API, contract, telosDecide, member eos.AccountName) (docgraph.Document, error) {

	// retrieve the last proposal
	proposal, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, eos.Name("proposal"))
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error retrieving proposal document %v", err)
	}
	fmt.Println("Retrieved proposal document to close: " + proposal.Hash.String())

	ballot, err := proposal.GetContent("ballot_id")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error retrieving ballot %v", err)
	}
	_, err = TelosDecideVote(ctx, api, telosDecide, member, ballot.Impl.(eos.Name), eos.Name("pass"))
	if err == nil {
		fmt.Println("Member voted : " + string(member))
	}
	eostest.Pause(defaultPause(), "Building block...", "")

	_, err = TelosDecideVote(ctx, api, telosDecide, eos.AN("alice"), ballot.Impl.(eos.Name), eos.Name("pass"))
	if err == nil {
		fmt.Println("Member voted : alice")
	}
	eostest.Pause(defaultPause(), "Building block...", "")

	_, err = TelosDecideVote(ctx, api, telosDecide, eos.AN("johnnyhypha1"), ballot.Impl.(eos.Name), eos.Name("pass"))
	if err == nil {
		fmt.Println("Member voted : johnnyhypha1")
	}
	eostest.Pause(defaultPause(), "Building block...", "")

	index := 1
	for index < 5 {

		memberNameIn := "mem" + strconv.Itoa(index) + ".hypha"
		//memberNameIn := "member" + strconv.Itoa(index)

		_, err := TelosDecideVote(ctx, api, telosDecide, eos.AN(memberNameIn), ballot.Impl.(eos.Name), eos.Name("pass"))
		if err != nil {
			return docgraph.Document{}, fmt.Errorf("error voting via telos decide %v", err)
		}
		eostest.Pause(defaultPause(), "Building block...", "")
		fmt.Println("Member voted : " + string(memberNameIn))
		index++
	}

	settings, err := getSettings(ctx, api, contract)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("cannot retrieve settings document %v", err)
	}

	votingPeriodDuration, err := settings.GetContent("voting_duration_sec")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("cannot retrieve voting_duration_sec setting %v", err)
	}

	votingPause := time.Duration((5 + votingPeriodDuration.Impl.(int64)) * 1000000000)
	eostest.Pause(votingPause, "Waiting on voting period to lapse: "+strconv.Itoa(int(5+votingPeriodDuration.Impl.(int64)))+" seconds", "")

	fmt.Println("Closing proposal: " + proposal.Hash.String())
	_, err = CloseProposal(ctx, api, contract, member, proposal.Hash)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("cannot close proposal %v", err)
	}

	passedProposal, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, eos.Name("passedprops"))
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error retrieving passed proposal document %v", err)
	}
	fmt.Println("Retrieved passed proposal document to close: " + passedProposal.Hash.String())

	return passedProposal, nil
}

func createParent(ctx context.Context, api *eos.API, contract, telosDecide, member eos.AccountName, parentType eos.Name, data []byte) (docgraph.Document, error) {
	var doc docgraph.Document
	err := json.Unmarshal([]byte(data), &doc)
	if err != nil {
		panic(err)
	}

	return proposeAndPass(ctx, api, contract, telosDecide, member, proposal{
		Proposer:      member,
		ProposalType:  parentType,
		ContentGroups: doc.ContentGroups,
	})
}

// CreateRole ...
func CreateRole(ctx context.Context, api *eos.API, contract, telosDecide, member eos.AccountName, data []byte) (docgraph.Document, error) {
	return createParent(ctx, api, contract, telosDecide, member, eos.Name("role"), data)
}

// CreateBadge ...
func CreateBadge(ctx context.Context, api *eos.API, contract, telosDecide, member eos.AccountName, data []byte) (docgraph.Document, error) {
	return createParent(ctx, api, contract, telosDecide, member, eos.Name("badge"), data)
}

// CreateAssignment ...
func CreateAssignment(ctx context.Context, api *eos.API, contract, telosDecide, member eos.AccountName, parentType, assignmentType eos.Name, data []byte) (docgraph.Document, error) {
	var proposalDoc docgraph.Document
	err := json.Unmarshal([]byte(data), &proposalDoc)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("cannot unmarshal error: %v ", err)
	}

	eostest.Pause(defaultPause(), "Building block...", "")

	// e.g. a "role" is parent to a "role assignment"
	// e.g. a "badge" is parent to a "badge assignment"
	var parent docgraph.Document
	parent, err = docgraph.GetLastDocumentOfEdge(ctx, api, contract, parentType)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("cannot retrieve last document of edge: "+string(parentType)+"  - error: %v ", err)
	}

	// inject the parent hash in the first content group of the document
	// TODO: use content_group_label to find details group instead of just the first one
	proposalDoc.ContentGroups[0] = append(proposalDoc.ContentGroups[0], docgraph.ContentItem{
		Label: string(parentType),
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("checksum256"),
				Impl:   parent.Hash,
			}},
	})

	// inject the assignee in the first content group of the document
	// TODO: use content_group_label to find details group instead of just the first one
	proposalDoc.ContentGroups[0] = append(proposalDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "assignee",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("name"),
				Impl:   member,
			}},
	})

	return proposeAndPass(ctx, api, contract, telosDecide, member, proposal{
		Proposer:      member,
		ProposalType:  assignmentType,
		ContentGroups: proposalDoc.ContentGroups,
	})
}

//Creates an assignment without the INIT_TIME_SHARE, CURRENT_TIME_SHARE & LAST_TIME_SHARE nodes
func CreateOldAssignment(t *testing.T, ctx context.Context, api *eos.API, contract, member eos.AccountName, memberDocHash, roleDocHash, startPeriodHash eos.Checksum256, assignment string) (docgraph.Document, error) {

	_, err := ProposeAssignment(ctx, api, contract, member, member, roleDocHash, startPeriodHash, assignment)
	if err != nil {
		return docgraph.Document{}, err
	}

	// retrieve the document we just created
	proposal, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, eos.Name("proposal"))
	if err != nil {
		return docgraph.Document{}, err
	}

	return proposal, nil
}

// CreatePayout ...
func CreatePayout(ctx context.Context, api *eos.API,
	contract, telosDecide, proposer, recipient eos.AccountName,
	usdAmount eos.Asset, deferred int64, data []byte) (docgraph.Document, error) {

	var payoutDoc docgraph.Document
	err := json.Unmarshal([]byte(data), &payoutDoc)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("cannot unmarshal error: %v ", err)
	}

	// inject the recipient in the first content group of the document
	// TODO: use content_group_label to find details group instead of just the first one
	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "recipient",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("name"),
				Impl:   recipient,
			}},
	})

	// TODO: use content_group_label to find details group instead of just the first one
	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "usd_amount",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("asset"),
				Impl:   usdAmount,
			}},
	})

	// TODO: use content_group_label to find details group instead of just the first one
	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
		Label: "deferred_perc_x100",
		Value: &docgraph.FlexValue{
			BaseVariant: eos.BaseVariant{
				TypeID: docgraph.GetVariants().TypeID("int64"),
				Impl:   deferred,
			}},
	})

	return proposeAndPass(ctx, api, contract, telosDecide, proposer, proposal{
		Proposer:      proposer,
		ProposalType:  eos.Name("payout"),
		ContentGroups: payoutDoc.ContentGroups,
	})
}

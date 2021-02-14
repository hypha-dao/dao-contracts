package dao

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"strconv"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"github.com/spf13/viper"
)

type assPayoutIn struct {
	ID           uint64             `json:"ass_payment_id"`
	AssignmentID uint64             `json:"assignment_id"`
	Recipient    eos.Name           `json:"recipient"`
	PeriodID     uint64             `json:"period_id"`
	Payments     []eos.Asset        `json:"payments"`
	PaymentDate  eos.BlockTimestamp `json:"payment_date"`
}

type assPayoutOut struct {
	ID           uint64        `json:"ass_payment_id"`
	AssignmentID uint64        `json:"assignment_id"`
	Recipient    eos.Name      `json:"recipient"`
	PeriodID     uint64        `json:"period_id"`
	Payments     []eos.Asset   `json:"payments"`
	PaymentDate  eos.TimePoint `json:"payment_date"`
}

func payoutExists(ctx context.Context, api *eos.API, contract eos.AccountName, id int) bool {
	var records []assPayoutIn
	var request eos.GetTableRowsRequest
	request.Code = string(contract)
	request.Scope = string(contract)
	request.LowerBound = strconv.Itoa(id)
	request.UpperBound = strconv.Itoa(id)
	request.Table = "asspayouts"
	request.Limit = 1
	request.JSON = true
	response, err := api.GetTableRows(ctx, request)
	if err != nil {
		return false
	}

	err = response.JSONToStructs(&records)
	if err != nil {
		return false
	}

	if len(records) > 0 {
		return true
	}
	return false
}

// CopyAssPayouts ...
func CopyAssPayouts(ctx context.Context, api *eos.API, contract eos.AccountName, from string) {

	sourceAPI := *eos.New(from)

	payoutsIn, err := getAllAssPayouts(ctx, &sourceAPI, contract)
	if err != nil {
		panic(err)
	}

	fmt.Println("\nCopying " + strconv.Itoa(len(payoutsIn)) + " assignment payout records from " + from)
	bar := DefaultProgressBar(len(payoutsIn))

	for index, payoutIn := range payoutsIn {

		paymentDate := eos.TimePoint(payoutIn.PaymentDate.UnixNano() / 1000)

		if !payoutExists(ctx, api, contract, int(payoutIn.ID)) {
			actions := []*eos.Action{{
				Account: contract,
				Name:    eos.ActN("addasspayout"),
				Authorization: []eos.PermissionLevel{
					{Actor: contract, Permission: eos.PN("active")},
				},
				ActionData: eos.NewActionData(assPayoutOut{
					ID:           payoutIn.ID,
					AssignmentID: payoutIn.AssignmentID,
					Recipient:    payoutIn.Recipient,
					PeriodID:     payoutIn.PeriodID,
					Payments:     payoutIn.Payments,
					PaymentDate:  paymentDate,
				}),
			}}

			_, err := eostest.ExecTrx(ctx, api, actions)
			if err != nil {
				fmt.Println("\n\nFAILED to migrate assignment pay: ", payoutIn.PaymentDate.Format("2006 Jan 02"), ", ", strconv.Itoa(index)+" / "+strconv.Itoa(len(payoutsIn)))
				fmt.Println(err)
				fmt.Println()
			}
			time.Sleep(defaultPause())
		}
		bar.Add(1)
	}
}

type addLegPer struct {
	ID        uint64        `json:"id"`
	StartTime eos.TimePoint `json:"start_date"`
	EndTime   eos.TimePoint `json:"end_date"`
	Phase     string        `json:"phase"`
	Readable  string        `json:"readable"`
	NodeLabel string        `json:"label"`
}

func exists(ctx context.Context, api *eos.API, contract eos.AccountName, table, scope eos.Name, id int) bool {
	var records []Object
	var request eos.GetTableRowsRequest
	request.Code = string(contract)
	request.Scope = string(scope)
	request.LowerBound = strconv.Itoa(id)
	request.UpperBound = strconv.Itoa(id)
	request.Table = string(table)
	request.Limit = 1
	request.JSON = true
	response, err := api.GetTableRows(ctx, request)
	if err != nil {
		return false
	}

	err = response.JSONToStructs(&records)
	if err != nil {
		return false
	}

	if len(records) > 0 {
		return true
	}
	return false
}

func periodExists(ctx context.Context, api *eos.API, contract eos.AccountName, id int) bool {
	var records []Period
	var request eos.GetTableRowsRequest
	request.Code = string(contract)
	request.Scope = string(contract)
	request.LowerBound = strconv.Itoa(id)
	request.UpperBound = strconv.Itoa(id)
	request.Table = "periods"
	request.Limit = 1
	request.JSON = true
	response, err := api.GetTableRows(ctx, request)
	if err != nil {
		return false
	}

	err = response.JSONToStructs(&records)
	if err != nil {
		return false
	}

	if len(records) > 0 {
		return true
	}
	return false
}

// CopyPeriods ...
func CopyPeriods(ctx context.Context, api *eos.API, contract eos.AccountName, from string) {

	sourceAPI := *eos.New(from)
	periods := getLegacyPeriods(ctx, &sourceAPI, eos.AN("dao.hypha"))

	fmt.Println("\nCopying " + strconv.Itoa(len(periods)) + " periods from " + from)

	bar := DefaultProgressBar(len(periods))

	for _, period := range periods {

		if !periodExists(ctx, api, contract, int(period.PeriodID)) {

			startTime := eos.TimePoint(period.StartTime.UnixNano() / 1000)
			endTime := eos.TimePoint(period.EndTime.UnixNano() / 1000)

			actions := []*eos.Action{{
				Account: contract,
				Name:    eos.ActN("addlegper"),
				Authorization: []eos.PermissionLevel{
					{Actor: contract, Permission: eos.PN("active")},
				},
				ActionData: eos.NewActionData(addLegPer{
					ID:        period.PeriodID,
					StartTime: startTime,
					EndTime:   endTime,
					Phase:     period.Phase,
					Readable:  period.StartTime.Time.Format("2006 Jan 02 15:04:05 MST"),
					NodeLabel: "Starting " + period.StartTime.Time.Format("2006 Jan 02"),
				}),
			}}

			_, err := eostest.ExecTrx(ctx, api, actions)
			if err != nil {
				fmt.Println("\n\nFAILED to copy period: ", strconv.Itoa(int(period.PeriodID)))
				fmt.Println(err)
				fmt.Println()
			}
			time.Sleep(defaultPause())
		}
		bar.Add(1)
	}
}

// CopyObjects ...
func CopyObjects(ctx context.Context, api *eos.API, contract eos.AccountName, scope eos.Name, from string) {

	sourceAPI := *eos.New(from)
	objects, _ := getLegacyObjects(ctx, &sourceAPI, contract, scope)

	fmt.Println("\nCopying " + strconv.Itoa(len(objects)) + " " + string(scope) + " objects from " + from)
	bar := DefaultProgressBar(len(objects))

	for _, object := range objects {

		if !exists(ctx, api, contract, eos.Name("objects"), scope, int(object.ID)) {

			object.Scope = eos.Name(scope)

			actions := []*eos.Action{{
				Account: contract,
				Name:    eos.ActN("createobj"),
				Authorization: []eos.PermissionLevel{
					{Actor: contract, Permission: eos.PN("active")},
				},
				ActionData: eos.NewActionData(object),
			}}

			_, err := eostest.ExecTrx(ctx, api, actions)
			if err != nil {
				fmt.Println("\n\nFAILED to createobj object - scope: ", string(scope)+", ", strconv.Itoa(int(object.ID)))
				fmt.Println(err)
				fmt.Println()
			} else {
				time.Sleep(defaultPause())
			}
		}
		bar.Add(1)
	}
}

func memberExists(ctx context.Context, api *eos.API, contract eos.AccountName, member eos.Name) bool {
	var records []memberRecord
	var request eos.GetTableRowsRequest
	request.Code = string(contract)
	request.Scope = string(contract)
	request.LowerBound = string(member)
	request.UpperBound = string(member)
	request.Table = "members"
	request.Limit = 1
	request.JSON = true
	response, err := api.GetTableRows(ctx, request)
	if err != nil {
		return false
	}

	err = response.JSONToStructs(&records)
	if err != nil {
		return false
	}

	if len(records) > 0 {
		return true
	}
	return false
}

// CopyMembers ...
func CopyMembers(ctx context.Context, api *eos.API, contract eos.AccountName, from string) {

	sourceAPI := *eos.New(from)
	memberRecords := getLegacyMembers(ctx, &sourceAPI, eos.AN("dao.hypha"))

	fmt.Println("\nCopying " + strconv.Itoa(len(memberRecords)) + " members from " + from)
	bar := DefaultProgressBar(len(memberRecords))

	for _, memberRecord := range memberRecords {

		if !memberExists(ctx, api, contract, memberRecord.MemberName) {

			actions := []*eos.Action{{
				Account: contract,
				Name:    eos.ActN("addmember"),
				Authorization: []eos.PermissionLevel{
					{Actor: contract, Permission: eos.PN("active")},
				},
				ActionData: eos.NewActionData(memberRecord),
			}}

			_, err := eostest.ExecTrx(ctx, api, actions)
			if err != nil {
				fmt.Println("\n\nFAILED to copy member: ", string(memberRecord.MemberName))
				fmt.Println(err)
				fmt.Println()
			}
			time.Sleep(defaultPause())
		} else {
			fmt.Println("\nMember already exists: " + string(memberRecord.MemberName))
		}
		bar.Add(1)
	}
}

// CreatePretend ...
func CreatePretend(ctx context.Context, api *eos.API, contract, telosDecide, member eos.AccountName) (docgraph.Document, error) {

	// roleFilename := "/Users/max/dev/hypha/daoctl/testing/role.json"
	// roleData, err := ioutil.ReadFile(roleFilename)
	// if err != nil {
	// 	return docgraph.Document{}, fmt.Errorf("Unable to read file: %v %v", roleFilename, err)
	// }

	// role, err := CreateRole(ctx, api, contract, telosDecide, member, roleData)
	// if err != nil {
	// 	return docgraph.Document{}, fmt.Errorf("Unable to create role: %v", err)
	// }
	// fmt.Println("Created role document	: ", role.Hash.String())

	assignmentData, err := ioutil.ReadFile("/Users/max/dev/hypha/daoctl/testing/assignment.json")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("Unable to read file: %v", err)
	}

	roleAssignment, err := CreateAssignment(ctx, api, contract, telosDecide, member, eos.Name("role"), eos.Name("assignment"), assignmentData)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("Unable to create assignment: %v", err)
	}
	fmt.Println("Created role assignment document	: ", roleAssignment.Hash.String())

	// fmt.Println("Waiting for a period to lapse...")
	// time.Sleep(defaultPeriodDuration())

	// _, err = claimNextPeriod(ctx, api, contract, member, roleAssignment)

	payoutData, err := ioutil.ReadFile("/Users/max/dev/hypha/daoctl/testing/payout.json")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("Unable to read payout file: %v", err)
	}

	payAmt, _ := eos.NewAssetFromString("1000.00 USD")
	payout, err := CreatePayout(ctx, api, contract, telosDecide, member, member, payAmt, 50, payoutData)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("Unable to create payout: %v", err)
	}
	fmt.Println("Created payout document	: ", payout.Hash.String())

	badgeData, err := ioutil.ReadFile("/Users/max/dev/hypha/daoctl/testing/badge.json")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("Unable to read badge file: %v", err)
	}

	badge, err := CreateBadge(ctx, api, contract, telosDecide, member, badgeData)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("Unable to create badge: %v", err)
	}
	fmt.Println("Created badge document	: ", badge.Hash.String())

	badgeAssignmentData, err := ioutil.ReadFile("/Users/max/dev/hypha/daoctl/testing/badge-assignment.json")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("Unable to read badge assignment file: %v", err)
	}

	badgeAssignment, err := CreateAssignment(ctx, api, contract, telosDecide, member, eos.Name("badge"), eos.Name("assignbadge"), badgeAssignmentData)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("Unable to create badge assignment: %v", err)
	}
	fmt.Println("Created badge assignment document	: ", badgeAssignment.Hash.String())
	return badgeAssignment, nil //roleAssignment, nil
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

	trxID, err := eostest.ExecTrx(ctx, api, actions)

	if err != nil {
		fmt.Println("Waiting for a period to lapse...")
		time.Sleep(time.Second * 7)
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

		trxID, err = eostest.ExecTrx(ctx, api, actions)
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

	pause(defaultPause(), "Building block...", "")

	trxID, err = Enroll(ctx, api, contract, contract, member)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error enrolling %v", err)
	}
	fmt.Println("Completed the enroll transaction: " + trxID)

	pause(defaultPause(), "Building block...", "")

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

	trxID, err := eostest.ExecTrx(ctx, api, actions)
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

	ballot, err := proposal.GetContent("ballot_id")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error retrieving ballot %v", err)
	}

	_, err = dao.TelosDecideVote(ctx, api, telosDecide, member, ballot.Impl.(eos.Name), eos.Name("pass"))
	if err == nil {
		fmt.Println("Member voted : " + string(member))
	}
	pause(defaultPause(), "Building block...", "")

	_, err = dao.TelosDecideVote(ctx, api, telosDecide, eos.AN("alice"), ballot.Impl.(eos.Name), eos.Name("pass"))
	if err == nil {
		fmt.Println("Member voted : alice")
	}
	pause(defaultPause(), "Building block...", "")

	_, err = dao.TelosDecideVote(ctx, api, telosDecide, eos.AN("johnnyhypha1"), ballot.Impl.(eos.Name), eos.Name("pass"))
	if err == nil {
		fmt.Println("Member voted : johnnyhypha1")
	}
	pause(defaultPause(), "Building block...", "")

	index := 1
	for index < 5 {

		memberNameIn := "mem" + strconv.Itoa(index) + ".hypha"
		//memberNameIn := "member" + strconv.Itoa(index)

		_, err := dao.TelosDecideVote(ctx, api, telosDecide, eos.AN(memberNameIn), ballot.Impl.(eos.Name), eos.Name("pass"))
		if err != nil {
			return docgraph.Document{}, fmt.Errorf("error voting via telos decide %v", err)
		}
		pause(defaultPause(), "Building block...", "")
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
	pause(votingPause, "Waiting on voting period to lapse: "+strconv.Itoa(int(5+votingPeriodDuration.Impl.(int64)))+" seconds", "")
	_, err = dao.CloseProposal(ctx, api, contract, member, proposal.Hash)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("cannot close proposal %v", err)
	}
	return proposal, nil
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

	pause(defaultPause(), "Building block...", "")

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

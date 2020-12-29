package main

import (
	"context"
	"encoding/json"
	"fmt"
	"strconv"
	"testing"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-contracts/dao-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"github.com/k0kubun/go-ansi"
	"github.com/schollz/progressbar/v3"
)

type notes struct {
	Notes string `json:"notes"`
}

type migrate struct {
	Scope eos.Name `json:"scope"`
	ID    uint64   `json:"id"`
}

type eraseobjs struct {
	Scope eos.Name `json:"scope"`
}

// Object ...
type Object struct {
	ID         uint64            `json:"id"`
	Scope      eos.Name          `json:"scope"`
	Names      []dao.NameKV      `json:"names"`
	Strings    []dao.StringKV    `json:"strings"`
	Assets     []dao.AssetKV     `json:"assets"`
	TimePoints []dao.TimePointKV `json:"time_points"`
	Ints       []dao.IntKV       `json:"ints"`
}

// MemberRecord represents a single row in the dao::members table
type MemberRecord struct {
	MemberName eos.Name `json:"member"`
}

func migrateMembers(ctx context.Context, api *eos.API, contract eos.AccountName, from string) {

	endpointAPI := *eos.New(from)

	var memberRecords []MemberRecord
	var request eos.GetTableRowsRequest
	request.Code = "dao.hypha"
	request.Scope = "dao.hypha"
	request.Table = "members"
	request.Limit = 500
	request.JSON = true
	response, _ := endpointAPI.GetTableRows(ctx, request)
	response.JSONToStructs(&memberRecords)

	for index, memberRecord := range memberRecords {
		actions := []*eos.Action{{
			Account: contract,
			Name:    eos.ActN("addmember"),
			Authorization: []eos.PermissionLevel{
				{Actor: contract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(memberRecord),
		}}

		trxID, err := eostest.ExecTrx(ctx, api, actions)
		if err != nil {
			panic(err)
		}

		fmt.Println("Added a member: ", memberRecord.MemberName, ", ", strconv.Itoa(index)+" / "+strconv.Itoa(len(memberRecords))+" : trxID:  "+trxID)
	}

	for index, memberRecord := range memberRecords {
		actions := []*eos.Action{{
			Account: contract,
			Name:    eos.ActN("migratemem"),
			Authorization: []eos.PermissionLevel{
				{Actor: contract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(memberRecord),
		}}

		trxID, err := eostest.ExecTrx(ctx, api, actions)
		if err != nil {
			panic(err)
		}

		fmt.Println("Migrated a member: ", memberRecord.MemberName, ", ", strconv.Itoa(index)+" / "+strconv.Itoa(len(memberRecords))+" : trxID:  "+trxID)
	}
}

func reset4test(ctx context.Context, api *eos.API, contract eos.AccountName) {
	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("reset4test"),
		Authorization: []eos.PermissionLevel{
			{Actor: contract, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(notes{
			Notes: "resetting for testing",
		}),
	}}
	trxID, err := eostest.ExecTrx(ctx, api, actions)
	if err != nil {
		panic(err)
	}

	fmt.Println("Completed the reset4test transaction: " + trxID)
	fmt.Println()
}

func eraseobjsByScope(ctx context.Context, api *eos.API, contract eos.AccountName, scope eos.Name) {
	actions := []*eos.Action{{
		Account: contract,
		Name:    eos.ActN("eraseobjs"),
		Authorization: []eos.PermissionLevel{
			{Actor: contract, Permission: eos.PN("active")},
		},
		ActionData: eos.NewActionData(eraseobjs{
			Scope: scope,
		}),
	}}
	trxID, err := eostest.ExecTrx(ctx, api, actions)
	if err != nil {
		panic(err)
	}

	fmt.Println("Completed the eraseobjs transaction: " + string(scope) + ": trxID: " + trxID)
	fmt.Println()
}

func enrollMembers(ctx context.Context, api *eos.API, contract eos.AccountName) {

	// re-enroll members
	index := 1
	for index < 6 {

		memberNameIn := "mem" + strconv.Itoa(index) + ".hypha"
		//memberNameIn := "member" + strconv.Itoa(index)

		newMember, err := enrollMember(ctx, api, contract, eos.AN(memberNameIn))
		if err != nil {
			panic(err)
		}
		fmt.Println("Member enrolled : " + string(memberNameIn) + " with hash: " + newMember.Hash.String())
		fmt.Println()

		index++
	}
}

func main() {

	ctx := context.Background()
	keyBag := &eos.KeyBag{}

	api := eos.New("https://testnet.telos.caleos.io")
	contract := eos.AccountName("dao.hypha")
	err := keyBag.ImportPrivateKey(ctx, "5HwnoWBuuRmNdcqwBzd1LABFRKnTk2RY2kUMYKkZfF8tKodubtK")

	// api := eos.New("http://localhost:8888")
	// contract := eos.AccountName("dao.hypha")
	// err := keyBag.ImportPrivateKey(ctx, eostest.DefaultKey())

	if err != nil {
		panic(err)
	}
	api.SetSigner(keyBag)

	scope := eos.Name("role")

	reset4test(ctx, api, contract)
	eraseobjsByScope(ctx, api, contract, scope)

	// enroll the test members
	//enrollMembers(ctx, api, contract)

	// // copy data from prod
	migrateMembers(ctx, api, contract, "https://api.telos.kitchen")
	copyPeriodsFromProd(api, contract)
	CopyObjects(ctx, api, contract, scope, "https://api.telos.kitchen")

	// t := testing.T{}
	// pause(&t, time.Second, "Building blocks...", "")

	// iterate list of objects and migrate each one
	// objects := dao.GetObjects(ctx, contract, scope, "https://testnet.telos.caleos.io")

	// for _, object := range objects {

	// 	fmt.Println("Running migration on scope: " + string(scope) + ", ID: " + strconv.Itoa(int(object.ID)))

	// 	actions := []*eos.Action{{
	// 		Account: contract,
	// 		Name:    eos.ActN("migrate"),
	// 		Authorization: []eos.PermissionLevel{
	// 			{Actor: contract, Permission: eos.PN("active")},
	// 		},
	// 		ActionData: eos.NewActionData(migrate{
	// 			Scope: scope,
	// 			ID:    object.ID,
	// 		}),
	// 	}}

	// 	trxID, err := eostest.ExecTrx(ctx, api, actions)
	// 	if err != nil {
	// 		panic(err)
	// 	}
	// 	fmt.Println("Migration completed. Transaction ID:  " + trxID)
	// 	fmt.Println()
	// }

	// mem2 := eos.AN("mem2.hypha")
	// roleFilename := "/Users/max/dev/hypha/daoctl/testing/role.json"
	// roleData, err := ioutil.ReadFile(roleFilename)
	// if err != nil {
	// 	fmt.Println("Unable to read file: ", roleFilename)
	// 	return
	// }

	// role, err := createRole(ctx, api, contract, mem2, roleData)
	// if err != nil {
	// 	panic(err)
	// }
	// fmt.Println("Created role document	: ", role.Hash.String())

	// assignmentData, err := ioutil.ReadFile("/Users/max/dev/hypha/daoctl/testing/assignment.json")
	// if err != nil {
	// 	fmt.Println("Unable to read file: ", assignmentData)
	// 	return
	// }

	// roleAssignment, err := createAssignment(ctx, api, contract, mem2, eos.Name("role"), eos.Name("assignment"), assignmentData)
	// if err != nil {
	// 	panic(err)
	// }
	// fmt.Println("Created role assignment document	: ", roleAssignment.Hash.String())

	// payoutData, err := ioutil.ReadFile("/Users/max/dev/hypha/daoctl/testing/payout.json")
	// if err != nil {
	// 	fmt.Println("Unable to read file: ", payoutData)
	// 	return
	// }

	// payAmt, _ := eos.NewAssetFromString("1000.00 USD")
	// payout, err := createPayout(ctx, api, contract, mem2, mem2, payAmt, 50, payoutData)
	// if err != nil {
	// 	panic(err)
	// }
	// fmt.Println("Created payout document	: ", payout.Hash.String())

	// badgeData, err := ioutil.ReadFile("/Users/max/dev/hypha/daoctl/testing/badge.json")
	// if err != nil {
	// 	panic(err)
	// }

	// badge, err := createBadge(ctx, api, contract, mem2, badgeData)
	// if err != nil {
	// 	panic(err)
	// }
	// fmt.Println("Created badge document	: ", badge.Hash.String())

	// badgeAssignmentData, err := ioutil.ReadFile("/Users/max/dev/hypha/daoctl/testing/badge-assignment.json")
	// if err != nil {
	// 	panic(err)
	// }

	// badgeAssignment, err := createAssignment(ctx, api, contract, mem2, eos.Name("badge"), eos.Name("assignbadge"), badgeAssignmentData)
	// if err != nil {
	// 	panic(err)
	// }
	// fmt.Println("Created badge assignment document	: ", badgeAssignment.Hash.String())
}

func getProdPeriods() []dao.Period {
	endpointAPI := *eos.New("https://api.telos.kitchen")

	var objects []dao.Period
	var request eos.GetTableRowsRequest
	request.Code = "dao.hypha"
	request.Scope = "dao.hypha"
	request.Table = "periods"
	request.Limit = 10000
	request.JSON = true
	response, _ := endpointAPI.GetTableRows(context.Background(), request)
	response.JSONToStructs(&objects)
	return objects
}

type addPeriodBTS struct {
	Predecessor eos.Checksum256 `json:"predecessor"`
	StartTime   eos.TimePoint   `json:"start_time"`
	Label       string          `json:"label"`
}

func copyPeriodsFromProd(api *eos.API, contract eos.AccountName) {

	periods := getProdPeriods()

	rootDoc, err := docgraph.LoadDocument(context.Background(), api, contract, "0f374e7a9d8ab17f172f8c478744cdd4016497e15229616f2ffd04d8002ef64a")
	if err != nil {
		panic(err)
	}

	predecessor := rootDoc.Hash
	var lastPeriod docgraph.Document

	for _, period := range periods {

		seconds := period.StartTime
		// fmt.Println("seconds (nano)		: ", strconv.Itoa(int(seconds.UnixNano())))

		microSeconds := seconds.UnixNano() / 1000
		// fmt.Println("seconds (micro)	: ", strconv.Itoa(int(microSeconds)))

		startTime := eos.TimePoint(microSeconds)

		addPeriodAction := eos.Action{
			Account: contract,
			Name:    eos.ActN("addperiod"),
			Authorization: []eos.PermissionLevel{
				{Actor: contract, Permission: eos.PN("active")},
			},
			ActionData: eos.NewActionData(addPeriodBTS{
				Predecessor: predecessor,
				StartTime:   startTime,
				Label:       period.Phase,
			}),
		}

		trxID, err := eostest.ExecTrx(context.Background(), api, []*eos.Action{&addPeriodAction})
		if err != nil {
			fmt.Printf("Error adding period: %v", err)
		} else {
			fmt.Println(" Added period: " + trxID)
			t := testing.T{}
			pause(&t, time.Second, "Build block...", "")

			lastPeriod, err = docgraph.GetLastDocument(context.Background(), api, contract)
			if err != nil {
				panic(err)
			}
		}

		predecessor = lastPeriod.Hash
	}
}

func enrollMember(ctx context.Context, api *eos.API, contract, member eos.AccountName) (docgraph.Document, error) {
	fmt.Println("Enrolling " + member + " in DAO: " + contract)

	trxID, err := dao.Apply(ctx, api, contract, member, "apply to DAO")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error applying %v", err)
	}
	fmt.Println("Completed the apply transaction: " + trxID)

	t := testing.T{}
	pause(&t, time.Second, "Build block...", "")

	trxID, err = dao.Enroll(ctx, api, contract, contract, member)
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error enrolling %v", err)
	}
	fmt.Println("Completed the enroll transaction: " + trxID)

	pause(&t, time.Second, "Build block...", "")

	memberDoc, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, "member")
	if err != nil {
		return docgraph.Document{}, fmt.Errorf("error enrolling %v", err)
	}

	return memberDoc, nil
}

func pause(t *testing.T, seconds time.Duration, headline, prefix string) {
	if headline != "" {
		t.Log(headline)
	}

	bar := progressbar.NewOptions(100,
		progressbar.OptionSetWriter(ansi.NewAnsiStdout()),
		progressbar.OptionEnableColorCodes(true),
		progressbar.OptionSetWidth(90),
		// progressbar.OptionShowIts(),
		progressbar.OptionSetDescription("[cyan]"+fmt.Sprintf("%20v", prefix)),
		progressbar.OptionSetTheme(progressbar.Theme{
			Saucer:        "[green]=[reset]",
			SaucerHead:    "[green]>[reset]",
			SaucerPadding: " ",
			BarStart:      "[",
			BarEnd:        "]",
		}))

	chunk := seconds / 100
	for i := 0; i < 100; i++ {
		bar.Add(1)
		time.Sleep(chunk)
	}
	fmt.Println()
	fmt.Println()
}

func getDefaultPeriod(api *eos.API, contract eos.AccountName) docgraph.Document {

	ctx := context.Background()

	root, err := docgraph.LoadDocument(ctx, api, contract, "0f374e7a9d8ab17f172f8c478744cdd4016497e15229616f2ffd04d8002ef64a")
	if err != nil {
		panic(err)
	}

	edges, err := docgraph.GetEdgesFromDocumentWithEdge(ctx, api, contract, root, eos.Name("start"))
	if err != nil || len(edges) <= 0 {
		panic("There are no next edges")
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
	contract, proposer eos.AccountName, proposal proposal) (docgraph.Document, error) {
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
		panic(err)
	}
	fmt.Println("Proposed: Transaction ID: " + trxID)

	return closeLastProposal(ctx, api, contract, proposer)
}

func closeLastProposal(ctx context.Context, api *eos.API, contract, member eos.AccountName) (docgraph.Document, error) {

	t := testing.T{}
	pause(&t, 5*time.Second, "Building blocks...", "")

	// retrieve the last proposal
	proposal, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, eos.Name("proposal"))
	if err != nil {
		panic(err)
	}

	ballot, err := proposal.GetContent("ballot_id")
	if err != nil {
		panic(err)
	}

	index := 1
	for index < 6 {

		memberNameIn := "mem" + strconv.Itoa(index) + ".hypha"
		//memberNameIn := "member" + strconv.Itoa(index)

		_, err := dao.TelosDecideVote(ctx, api, eos.AN("td1.hypha"), eos.AN(memberNameIn), ballot.Impl.(eos.Name), eos.Name("pass"))
		if err != nil {
			panic(err)
		}
		fmt.Println("Member voted : " + string(memberNameIn))
		index++
	}

	pause(&t, time.Second*65, "Waiting on voting period to lapse", "")

	_, err = dao.CloseProposal(ctx, api, contract, member, proposal.Hash)
	if err != nil {
		panic(err)
	}
	return proposal, nil
}

func createParent(ctx context.Context, api *eos.API, contract, member eos.AccountName, parentType eos.Name, data []byte) (docgraph.Document, error) {
	var doc docgraph.Document
	err := json.Unmarshal([]byte(data), &doc)
	if err != nil {
		panic(err)
	}

	return proposeAndPass(ctx, api, contract, member, proposal{
		Proposer:      member,
		ProposalType:  parentType,
		ContentGroups: doc.ContentGroups,
	})
}

func createRole(ctx context.Context, api *eos.API, contract, member eos.AccountName, data []byte) (docgraph.Document, error) {
	return createParent(ctx, api, contract, member, eos.Name("role"), data)
}

func createBadge(ctx context.Context, api *eos.API, contract, member eos.AccountName, data []byte) (docgraph.Document, error) {
	return createParent(ctx, api, contract, member, eos.Name("badge"), data)
}

func createAssignment(ctx context.Context, api *eos.API, contract, member eos.AccountName, parentType, assignmentType eos.Name, data []byte) (docgraph.Document, error) {
	var proposalDoc docgraph.Document
	err := json.Unmarshal([]byte(data), &proposalDoc)
	if err != nil {
		panic(err)
	}

	// e.g. a "role" is parent to a "role assignment"
	// e.g. a "badge" is parent to a "badge assignment"
	var parent docgraph.Document
	parent, err = docgraph.GetLastDocumentOfEdge(ctx, api, contract, parentType)
	if err != nil {
		panic(err)
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

	return proposeAndPass(ctx, api, contract, member, proposal{
		Proposer:      member,
		ProposalType:  assignmentType,
		ContentGroups: proposalDoc.ContentGroups,
	})
}

func createPayout(ctx context.Context, api *eos.API,
	contract, proposer, recipient eos.AccountName,
	usdAmount eos.Asset, deferred int64, data []byte) (docgraph.Document, error) {

	var payoutDoc docgraph.Document
	err := json.Unmarshal([]byte(data), &payoutDoc)
	if err != nil {
		panic("cannot unmarshal payout doc")
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

	return proposeAndPass(ctx, api, contract, proposer, proposal{
		Proposer:      proposer,
		ProposalType:  eos.Name("payout"),
		ContentGroups: payoutDoc.ContentGroups,
	})
}

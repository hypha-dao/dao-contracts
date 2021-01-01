package main

import (
	"context"
	"fmt"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-contracts/dao-go"
)

var mainnet = "https://api.telos.kitchen"
var testnet = "https://testnet.telos.caleos.io"

type notes struct {
	Notes string `json:"notes"`
}

type eraseobjs struct {
	Scope eos.Name `json:"scope"`
}

func reset4test(ctx context.Context, api *eos.API, contract eos.AccountName) {

	fmt.Println("\nRunning contract reset action on: " + string(contract))
	bar := dao.DefaultProgressBar(1)

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
	_, err := eostest.ExecTrx(ctx, api, actions)
	if err != nil {
		panic(err)
	}

	bar.Add(1)
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

func main() {

	ctx := context.Background()
	keyBag := &eos.KeyBag{}

	// api := eos.New(testnet)
	contract := eos.AccountName("dao.hypha")
	err := keyBag.ImportPrivateKey(ctx, "5KCZ9VBJMMiLaAY24Ro66mhx4vU1VcJELZVGrJbkUBATyqxyYmj")
	err = keyBag.ImportPrivateKey(ctx, "5HwnoWBuuRmNdcqwBzd1LABFRKnTk2RY2kUMYKkZfF8tKodubtK")
	err = keyBag.ImportPrivateKey(ctx, eostest.DefaultKey())

	//new test key: 5KCZ9VBJMMiLaAY24Ro66mhx4vU1VcJELZVGrJbkUBATyqxyYmj
	// old test key: 5HwnoWBuuRmNdcqwBzd1LABFRKnTk2RY2kUMYKkZfF8tKodubtK
	api := eos.New("http://localhost:8888")

	if err != nil {
		panic(err)
	}
	api.SetSigner(keyBag)

	// erase environment except for settings
	dao.EraseAllDocuments(ctx, api, contract)

	reset4test(ctx, api, contract)

	dao.CopyMembers(ctx, api, contract, "https://api.telos.kitchen")
	dao.MigrateMembers(ctx, api, contract)

	dao.CopyPeriods(ctx, api, contract, "https://api.telos.kitchen")
	dao.MigratePeriods(ctx, api, contract)

	scopes := []eos.Name{"role", "assignment"}

	for _, scope := range scopes {
		dao.CopyObjects(ctx, api, contract, scope, "https://api.telos.kitchen")
		dao.MigrateObjects(ctx, api, contract, scope)
	}

	// dao.CopyAssPayouts(ctx, api, contract, "https://api.telos.kitchen")
	// dao.MigrateAssPayouts(ctx, api, contract)

	// **********************************************************************
	// *******************
	// test data and use cases

	// enroll the test members
	// enrollMembers(ctx, api, contract)

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

// func enrollMembers(ctx context.Context, api *eos.API, contract eos.AccountName) {

// 	// re-enroll members
// 	index := 1
// 	for index < 6 {

// 		memberNameIn := "mem" + strconv.Itoa(index) + ".hypha"
// 		//memberNameIn := "member" + strconv.Itoa(index)

// 		newMember, err := enrollMember(ctx, api, contract, eos.AN(memberNameIn))
// 		if err != nil {
// 			panic(err)
// 		}
// 		fmt.Println("Member enrolled : " + string(memberNameIn) + " with hash: " + newMember.Hash.String())
// 		fmt.Println()

// 		index++
// 	}
// }

// func enrollMember(ctx context.Context, api *eos.API, contract, member eos.AccountName) (docgraph.Document, error) {
// 	fmt.Println("Enrolling " + member + " in DAO: " + contract)

// 	trxID, err := dao.Apply(ctx, api, contract, member, "apply to DAO")
// 	if err != nil {
// 		return docgraph.Document{}, fmt.Errorf("error applying %v", err)
// 	}
// 	fmt.Println("Completed the apply transaction: " + trxID)

// 	t := testing.T{}
// 	pause(&t, time.Second, "Build block...", "")

// 	trxID, err = dao.Enroll(ctx, api, contract, contract, member)
// 	if err != nil {
// 		return docgraph.Document{}, fmt.Errorf("error enrolling %v", err)
// 	}
// 	fmt.Println("Completed the enroll transaction: " + trxID)

// 	pause(&t, time.Second, "Build block...", "")

// 	memberDoc, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, "member")
// 	if err != nil {
// 		return docgraph.Document{}, fmt.Errorf("error enrolling %v", err)
// 	}

// 	return memberDoc, nil
// }

// func getDefaultPeriod(api *eos.API, contract eos.AccountName) docgraph.Document {

// 	ctx := context.Background()

// 	root, err := docgraph.LoadDocument(ctx, api, contract, "52a7ff82bd6f53b31285e97d6806d886eefb650e79754784e9d923d3df347c91")
// 	if err != nil {
// 		panic(err)
// 	}

// 	edges, err := docgraph.GetEdgesFromDocumentWithEdge(ctx, api, contract, root, eos.Name("start"))
// 	if err != nil || len(edges) <= 0 {
// 		panic("There are no next edges")
// 	}

// 	lastDocument, err := docgraph.LoadDocument(ctx, api, contract, edges[0].ToNode.String())
// 	if err != nil {
// 		panic("Next document not found: " + edges[0].ToNode.String())
// 	}

// 	index := 1
// 	for index < 6 {

// 		edges, err := docgraph.GetEdgesFromDocumentWithEdge(ctx, api, contract, lastDocument, eos.Name("next"))
// 		if err != nil || len(edges) <= 0 {
// 			panic("There are no next edges")
// 		}

// 		lastDocument, err = docgraph.LoadDocument(ctx, api, contract, edges[0].ToNode.String())
// 		if err != nil {
// 			panic("Next document not found: " + edges[0].ToNode.String())
// 		}

// 		index++
// 	}
// 	return lastDocument
// }

// type proposal struct {
// 	Proposer      eos.AccountName         `json:"proposer"`
// 	ProposalType  eos.Name                `json:"proposal_type"`
// 	ContentGroups []docgraph.ContentGroup `json:"content_groups"`
// }

// func proposeAndPass(ctx context.Context, api *eos.API,
// 	contract, proposer eos.AccountName, proposal proposal) (docgraph.Document, error) {
// 	action := eos.ActN("propose")
// 	actions := []*eos.Action{{
// 		Account: contract,
// 		Name:    action,
// 		Authorization: []eos.PermissionLevel{
// 			{Actor: proposer, Permission: eos.PN("active")},
// 		},
// 		ActionData: eos.NewActionData(proposal)}}

// 	trxID, err := eostest.ExecTrx(ctx, api, actions)
// 	if err != nil {
// 		panic(err)
// 	}
// 	fmt.Println("Proposed: Transaction ID: " + trxID)

// 	return closeLastProposal(ctx, api, contract, proposer)
// }

// func closeLastProposal(ctx context.Context, api *eos.API, contract, member eos.AccountName) (docgraph.Document, error) {

// 	t := testing.T{}
// 	pause(&t, 5*time.Second, "Building blocks...", "")

// 	// retrieve the last proposal
// 	proposal, err := docgraph.GetLastDocumentOfEdge(ctx, api, contract, eos.Name("proposal"))
// 	if err != nil {
// 		panic(err)
// 	}

// 	ballot, err := proposal.GetContent("ballot_id")
// 	if err != nil {
// 		panic(err)
// 	}

// 	index := 1
// 	for index < 6 {

// 		memberNameIn := "mem" + strconv.Itoa(index) + ".hypha"
// 		//memberNameIn := "member" + strconv.Itoa(index)

// 		_, err := dao.TelosDecideVote(ctx, api, eos.AN("td1.hypha"), eos.AN(memberNameIn), ballot.Impl.(eos.Name), eos.Name("pass"))
// 		if err != nil {
// 			panic(err)
// 		}
// 		fmt.Println("Member voted : " + string(memberNameIn))
// 		index++
// 	}

// 	pause(&t, time.Second*65, "Waiting on voting period to lapse", "")

// 	_, err = dao.CloseProposal(ctx, api, contract, member, proposal.Hash)
// 	if err != nil {
// 		panic(err)
// 	}
// 	return proposal, nil
// }

// func createParent(ctx context.Context, api *eos.API, contract, member eos.AccountName, parentType eos.Name, data []byte) (docgraph.Document, error) {
// 	var doc docgraph.Document
// 	err := json.Unmarshal([]byte(data), &doc)
// 	if err != nil {
// 		panic(err)
// 	}

// 	return proposeAndPass(ctx, api, contract, member, proposal{
// 		Proposer:      member,
// 		ProposalType:  parentType,
// 		ContentGroups: doc.ContentGroups,
// 	})
// }

// func createRole(ctx context.Context, api *eos.API, contract, member eos.AccountName, data []byte) (docgraph.Document, error) {
// 	return createParent(ctx, api, contract, member, eos.Name("role"), data)
// }

// func createBadge(ctx context.Context, api *eos.API, contract, member eos.AccountName, data []byte) (docgraph.Document, error) {
// 	return createParent(ctx, api, contract, member, eos.Name("badge"), data)
// }

// func createAssignment(ctx context.Context, api *eos.API, contract, member eos.AccountName, parentType, assignmentType eos.Name, data []byte) (docgraph.Document, error) {
// 	var proposalDoc docgraph.Document
// 	err := json.Unmarshal([]byte(data), &proposalDoc)
// 	if err != nil {
// 		panic(err)
// 	}

// 	// e.g. a "role" is parent to a "role assignment"
// 	// e.g. a "badge" is parent to a "badge assignment"
// 	var parent docgraph.Document
// 	parent, err = docgraph.GetLastDocumentOfEdge(ctx, api, contract, parentType)
// 	if err != nil {
// 		panic(err)
// 	}

// 	// inject the parent hash in the first content group of the document
// 	// TODO: use content_group_label to find details group instead of just the first one
// 	proposalDoc.ContentGroups[0] = append(proposalDoc.ContentGroups[0], docgraph.ContentItem{
// 		Label: string(parentType),
// 		Value: &docgraph.FlexValue{
// 			BaseVariant: eos.BaseVariant{
// 				TypeID: docgraph.GetVariants().TypeID("checksum256"),
// 				Impl:   parent.Hash,
// 			}},
// 	})

// 	// inject the assignee in the first content group of the document
// 	// TODO: use content_group_label to find details group instead of just the first one
// 	proposalDoc.ContentGroups[0] = append(proposalDoc.ContentGroups[0], docgraph.ContentItem{
// 		Label: "assignee",
// 		Value: &docgraph.FlexValue{
// 			BaseVariant: eos.BaseVariant{
// 				TypeID: docgraph.GetVariants().TypeID("name"),
// 				Impl:   member,
// 			}},
// 	})

// 	return proposeAndPass(ctx, api, contract, member, proposal{
// 		Proposer:      member,
// 		ProposalType:  assignmentType,
// 		ContentGroups: proposalDoc.ContentGroups,
// 	})
// }

// func createPayout(ctx context.Context, api *eos.API,
// 	contract, proposer, recipient eos.AccountName,
// 	usdAmount eos.Asset, deferred int64, data []byte) (docgraph.Document, error) {

// 	var payoutDoc docgraph.Document
// 	err := json.Unmarshal([]byte(data), &payoutDoc)
// 	if err != nil {
// 		panic("cannot unmarshal payout doc")
// 	}

// 	// inject the recipient in the first content group of the document
// 	// TODO: use content_group_label to find details group instead of just the first one
// 	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
// 		Label: "recipient",
// 		Value: &docgraph.FlexValue{
// 			BaseVariant: eos.BaseVariant{
// 				TypeID: docgraph.GetVariants().TypeID("name"),
// 				Impl:   recipient,
// 			}},
// 	})

// 	// TODO: use content_group_label to find details group instead of just the first one
// 	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
// 		Label: "usd_amount",
// 		Value: &docgraph.FlexValue{
// 			BaseVariant: eos.BaseVariant{
// 				TypeID: docgraph.GetVariants().TypeID("asset"),
// 				Impl:   usdAmount,
// 			}},
// 	})

// 	// TODO: use content_group_label to find details group instead of just the first one
// 	payoutDoc.ContentGroups[0] = append(payoutDoc.ContentGroups[0], docgraph.ContentItem{
// 		Label: "deferred_perc_x100",
// 		Value: &docgraph.FlexValue{
// 			BaseVariant: eos.BaseVariant{
// 				TypeID: docgraph.GetVariants().TypeID("int64"),
// 				Impl:   deferred,
// 			}},
// 	})

// 	return proposeAndPass(ctx, api, contract, proposer, proposal{
// 		Proposer:      proposer,
// 		ProposalType:  eos.Name("payout"),
// 		ContentGroups: payoutDoc.ContentGroups,
// 	})
// }

// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"paused", "value":["int64","0"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"seeds_token_contract", "value":["name","token.seeds"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"voting_duration_sec", "value":["int64",3600]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"seeds_deferral_factor_x100", "value":["int64","100"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"telos_decide_contract", "value":["name","trailservice"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"husd_token_contract", "value":["name","husd.hypha"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"hypha_token_contract", "value":["name","token.hypha"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"seeds_escrow_contract", "value":["name","escrow.seeds"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"publisher_contract", "value":["name","publsh.hypha"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"treasury_contract", "value":["name","bank.hypha"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"last_ballot_id", "value":["name","hypha1....1ce"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"hypha_deferral_factor_x100", "value":["int64","25"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"seeds_deferral_factor_x100", "value":["int64","100"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"client_version", "value":["string","0.2.0 pre-release"]}' -p dao.hypha
// eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha setsetting '{"key":"contract_version", "value":["string","0.2.0 pre-release"]}' -p dao.hypha

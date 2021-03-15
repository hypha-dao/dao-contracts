package dao_test

import (
	"strconv"
	"strings"
	"testing"
	"time"

	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/dao-contracts/dao-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"gotest.tools/assert"
)

func GetAdjustInfo(assignment eos.Checksum256, timeShare int64, startDate eos.TimePoint) []docgraph.ContentGroup {

	return []docgraph.ContentGroup{
		{
			{
				Label: "assignment",
				Value: &docgraph.FlexValue{
					BaseVariant: eos.BaseVariant{
						TypeID: docgraph.GetVariants().TypeID("checksum256"),
						Impl:   assignment,
					}},
			},
			{
				Label: "new_time_share_x100",
				Value: &docgraph.FlexValue{
					BaseVariant: eos.BaseVariant{
						TypeID: docgraph.GetVariants().TypeID("int64"),
						Impl:   timeShare,
					}},
			},
			{
				Label: "start_date",
				Value: &docgraph.FlexValue{
					BaseVariant: eos.BaseVariant{
						TypeID: docgraph.GetVariants().TypeID("time_point"),
						Impl:   startDate,
					}},
			},
		}}
}

//Used to calculate token compensation with 3 adjustments over the same
//period
func CalculateTotalCompensation(alfa, beta, gamma, totalByPeriod float32) float32 {
	//Or factorized totalByPeriod*1.0/3.0*(alfa+beta+gamma)
	return totalByPeriod*1.0/3.0*alfa +
		totalByPeriod*1.0/3.0*beta +
		totalByPeriod*1.0/3.0*gamma
}

func CreateAdjustmentAfter(commitment, startSecs int64, offsetSecs time.Duration, assignment *docgraph.Document, assignee *Member, env *Environment, t *testing.T) error {

	time := time.Unix(startSecs, 0)

	adjustStartDate := eos.TimePoint(time.Add(offsetSecs).UnixNano() / 1000)

	var adjustInfo = GetAdjustInfo(assignment.Hash, commitment, adjustStartDate)

	_, err := AdjustCommitment(env, assignee.Member, adjustInfo)

	assert.NilError(t, err)

	timeShare, err := docgraph.GetLastDocument(env.ctx, &env.api, env.DAO)

	ts, err := timeShare.GetContent("time_share_x100")
	assert.NilError(t, err)
	assert.Equal(t, ts.String(), strconv.FormatInt(commitment, 10))

	sd, err := timeShare.GetContent("start_date")
	assert.NilError(t, err)
	assert.Equal(t, sd.Impl.(eos.TimePoint)/1000, adjustStartDate/1000)

	return err
}

func AssetToInt(asset *docgraph.FlexValue) (int64, string, error) {
	assetStr := asset.Impl.(*eos.Asset).String()
	stringItms := strings.Split(assetStr, " ")
	parsed, err := strconv.ParseFloat(stringItms[0], 32)

	return int64(parsed), stringItms[1], err
}

func ValidateLastReceipt(targetHUSD, targetHYPHA, targetHVOICE, targetSEEDS int64, env *Environment, t *testing.T) {

	period, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("claimed"))
	assert.NilError(t, err)

	paymentEdges, err := docgraph.GetEdgesFromDocumentWithEdge(env.ctx, &env.api, env.DAO, period, eos.Name("payment"))
	assert.NilError(t, err)

	var paymentMap map[string]int64

	paymentMap = make(map[string]int64)

	for _, edge := range paymentEdges {
		payment, err := docgraph.LoadDocument(env.ctx, &env.api, env.DAO, edge.ToNode.String())
		assert.NilError(t, err)
		asset, err := payment.GetContent("amount")
		assert.NilError(t, err)
		amount, symbol, err := AssetToInt(asset)
		assert.NilError(t, err)
		paymentMap[symbol] = amount
	}

	if targetHUSD != 0 {
		assert.Equal(t, targetHUSD, paymentMap["HUSD"])
	}

	if targetHYPHA != 0 {
		assert.Equal(t, targetHYPHA, paymentMap["HYPHA"])
	}

	if targetHVOICE != 0 {
		assert.Equal(t, targetHVOICE, paymentMap["HVOICE"])
	}

	if targetSEEDS != 0 {
		assert.Equal(t, targetSEEDS, paymentMap["SEEDS"])
	}

	assert.NilError(t, err)
}

func TestAdjustCommitment(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	// roles
	proposer := env.Members[0]
	assignee := env.Members[1]
	closer := env.Members[2]

	role1Doc := CreateRole(t, env, proposer, closer, role1)

	t.Run("Test Adjust assignment commitment", func(t *testing.T) {

		tests := []struct {
			name       string
			roleTitle  string
			title      string
			role       docgraph.Document
			assignment string
			husd       string
			hypha      string
			hvoice     string
			usd        string
		}{
			{
				name:       "role1 - 100% 100%",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role1Doc,
				assignment: assignment1,
				husd:       "0.00 HUSD",
				hypha:      "759.75 HYPHA",
				hvoice:     "6078.02 HVOICE",
				usd:        "3039.01 USD",
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)

			_, err := dao.ProposeAssignment(env.ctx, &env.api, env.DAO, proposer.Member, assignee.Member, test.role.Hash, env.Periods[0].Hash, test.assignment)
			assert.NilError(t, err)

			// retrieve the document we just created
			proposal, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)

			voteToPassTD(t, env, proposal)

			//Wait Half Period to close the proposal and test the special
			//case when approved time overlaps in the first period
			t.Log("Waiting for a period to lapse...")
			pause(t, env.PeriodPause/2, "", "Waiting...")

			_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, proposal.Hash)
			assert.NilError(t, err)

			assignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("assignment"))
			assert.NilError(t, err)

			var periodDuration float32
			var firstPeriodStartSecs int64
			var firstPeriodEndSecs int64
			//Get starting period to calculate half period duration
			{
				periodHash, err := assignment.GetContent("start_period")
				assert.NilError(t, err)
				period, err := docgraph.LoadDocument(env.ctx, &env.api, env.DAO, periodHash.String())
				assert.NilError(t, err)
				startTime, err := period.GetContent("start_time")
				assert.NilError(t, err)
				firstPeriodStartSecs = int64(startTime.Impl.(eos.TimePoint)) / 1000000
				nextPeriods, err := docgraph.GetEdgesFromDocumentWithEdge(env.ctx, &env.api, env.DAO, period, eos.Name("next"))
				assert.NilError(t, err)
				nextPeriod, err := docgraph.LoadDocument(env.ctx, &env.api, env.DAO, nextPeriods[0].ToNode.String())
				assert.NilError(t, err)
				startTime, err = nextPeriod.GetContent("start_time")
				assert.NilError(t, err)
				firstPeriodEndSecs = int64(startTime.Impl.(eos.TimePoint)) / 1000000
			}

			//Create Adjustment 2.5 Periods after start period
			CreateAdjustmentAfter(int64(50), firstPeriodStartSecs,
				env.PeriodDuration*5/2,
				&assignment,
				&assignee, env, t)

			//Create Adjustment 3.33 Periods after start period
			CreateAdjustmentAfter(int64(100),
				firstPeriodStartSecs,
				env.PeriodDuration*10/3,
				&assignment,
				&assignee, env, t)

			//Create Adjustment 3.66 Periods after start period
			CreateAdjustmentAfter(int64(75),
				firstPeriodStartSecs,
				env.PeriodDuration*11/3,
				&assignment,
				&assignee, env, t)

			//TODO: Calculate seeds_per_usd using tlosto.seeds table
			//Set to 0 to temporaly disable  calculating SEEDS
			hardcodedSeedsPerUsd := float32(0) /*float32(39.0840)*/
			var seedsDeferralFactor float32
			{
				settings, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("settings"))

				seedsDeferral, err := settings.GetContent("seeds_deferral_factor_x100")
				assert.NilError(t, err)

				seedsDeferralFactor = float32(seedsDeferral.Impl.(int64)) / 100.0
			}

			usdSalaryPerPhase := float32(3039.01)

			//Number of HYPHA tokens received in 1 full period
			totalHYPHA := float32(759.75)

			//Number of HUSD tokens received in 1 full period
			totalHUSD := float32(0)

			//Number of HVOICE tokens received in 1 full period
			totalHVOICE := float32(usdSalaryPerPhase * 2.0)

			//Number of seeds received in 1 full period
			totalSEEDS := usdSalaryPerPhase * hardcodedSeedsPerUsd * seedsDeferralFactor

			//Claim first period
			t.Log("Waiting for a period to lapse...")
			pause(t, env.PeriodPause, "", "Waiting...")

			//This should get partial payment since the approved time should be > than
			//the start time of the period
			_, err = ClaimNextPeriod(t, env, assignee.Member, assignment)
			assert.NilError(t, err)

			{
				initTimeShare, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("initimeshare"))
				assert.NilError(t, err)
				approvedTime, err := initTimeShare.GetContent("start_date")
				approvedSecs := int64(approvedTime.Impl.(eos.TimePoint)) / 1000000
				periodDuration = float32(firstPeriodEndSecs - firstPeriodStartSecs)
				timeFactor := float32(firstPeriodEndSecs-approvedSecs) / periodDuration
				ValidateLastReceipt(int64(totalHUSD*timeFactor),
					int64(totalHYPHA*timeFactor),
					int64(totalHVOICE*timeFactor),
					int64(totalSEEDS*timeFactor), env, t)
			}

			//Claim second period
			t.Log("Waiting for a period to lapse...")
			pause(t, env.PeriodPause, "", "Waiting...")

			//This should get full payment since the first 50% adjustment takes
			//place on the next period
			_, err = ClaimNextPeriod(t, env, assignee.Member, assignment)
			assert.NilError(t, err)

			//Ignore decimal precision.
			ValidateLastReceipt(int64(totalHUSD), int64(totalHYPHA), int64(totalHVOICE), int64(totalSEEDS), env, t)

			//Claim third period
			t.Log("Waiting for another period to lapse...")
			pause(t, env.PeriodPause, "", "Waiting...")

			//This should get full payment for the first half of the period
			//and then half payment for the last half of the period
			_, err = ClaimNextPeriod(t, env, assignee.Member, assignment)
			assert.NilError(t, err)

			{
				//Period Duration
				half := int64(periodDuration) / 2
				firstHalf := float32(half) / periodDuration
				secondHalf := (periodDuration - float32(half)) / periodDuration
				newTotalSEEDS := totalSEEDS*firstHalf + totalSEEDS*float32(0.5)*secondHalf
				newTotalHYPHA := totalHYPHA*firstHalf + totalHYPHA*float32(0.5)*secondHalf
				newTotalHVOICE := totalHVOICE*firstHalf + totalHVOICE*float32(0.5)*secondHalf
				newTotalHUSD := totalHUSD*firstHalf + totalHUSD*float32(0.5)*secondHalf
				ValidateLastReceipt(int64(newTotalHUSD), int64(newTotalHYPHA), int64(newTotalHVOICE), int64(newTotalSEEDS), env, t)
			}

			//Claim last period
			t.Log("Waiting for another period to lapse...")
			pause(t, env.PeriodPause, "", "Waiting...")

			//This should get half payment for the first third,
			//full payment on the second third &
			//75% of payment on the last third
			_, err = ClaimNextPeriod(t, env, assignee.Member, assignment)
			assert.NilError(t, err)

			{
				newTotalSEEDS := CalculateTotalCompensation(0.5, 1.0, 0.75, totalSEEDS)
				newTotalHYPHA := CalculateTotalCompensation(0.5, 1.0, 0.75, totalHYPHA)
				newTotalHVOICE := CalculateTotalCompensation(0.5, 1.0, 0.75, totalHVOICE)
				newTotalHUSD := CalculateTotalCompensation(0.5, 1.0, 0.75, totalHUSD)
				ValidateLastReceipt(int64(newTotalHUSD), int64(newTotalHYPHA), int64(newTotalHVOICE), int64(newTotalSEEDS), env, t)
			}
		}
	})
}

func TestAssignmentProposalDocument(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	// roles
	proposer := env.Members[0]
	assignee := env.Members[1]
	closer := env.Members[2]

	role1Doc := CreateRole(t, env, proposer, closer, role1)
	role2Doc := CreateRole(t, env, proposer, closer, role2)
	role3Doc := CreateRole(t, env, proposer, closer, role3)

	t.Run("Test Assignment Document Proposal", func(t *testing.T) {

		tests := []struct {
			name       string
			roleTitle  string
			title      string
			role       docgraph.Document
			assignment string
			husd       string
			hypha      string
			hvoice     string
			usd        string
		}{
			{
				name:       "role1 - 100% 100%",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role1Doc,
				assignment: assignment1,
				husd:       "0.00 HUSD",
				hypha:      "759.75 HYPHA",
				hvoice:     "6078.02 HVOICE",
				usd:        "3039.01 USD",
			},
			{
				name:       "role2 - 100% commit, 70% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role2Doc,
				assignment: assignment2,
				husd:       "455.85 HUSD",
				hypha:      "265.91 HYPHA",
				hvoice:     "3039.00 HVOICE",
				usd:        "1519.50 USD",
			},
			{
				name:       "role2 - 75% commit, 50% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role2Doc,
				assignment: assignment3,
				husd:       "569.81 HUSD",
				hypha:      "142.45 HYPHA",
				hvoice:     "2279.26 HVOICE",
				usd:        "1519.50 USD",
			},
			{
				name:       "role3 - 51% commit, 100% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role3Doc,
				assignment: assignment4,
				husd:       "0.00 HUSD",
				hypha:      "452.05 HYPHA",
				hvoice:     "3616.42 HVOICE",
				usd:        "3545.51 USD",
			},
			{
				name:       "role3 - 25% commit, 50% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role3Doc,
				assignment: assignment5,
				husd:       "443.18 HUSD",
				hypha:      "110.79 HYPHA",
				hvoice:     "1772.74 HVOICE",
				usd:        "3545.51 USD",
			},
			{
				name:       "role3 - 100% commit, 15% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role3Doc,
				assignment: assignment6,
				husd:       "3013.68 HUSD",
				hypha:      "132.95 HYPHA",
				hvoice:     "7091.02 HVOICE",
				usd:        "3545.51 USD",
			},
			{
				name:       "role3 - 10% commit, 0% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role3Doc,
				assignment: assignment7,
				husd:       "354.55 HUSD",
				hypha:      "0.00 HYPHA",
				hvoice:     "709.10 HVOICE",
				usd:        "3545.51 USD",
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)
			_, err := dao.ProposeAssignment(env.ctx, &env.api, env.DAO, proposer.Member, assignee.Member, test.role.Hash, env.Periods[0].Hash, test.assignment)
			assert.NilError(t, err)

			// retrieve the document we just created
			assignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, assignment.Creator, proposer.Member)

			fv, err := assignment.GetContent("title")
			assert.NilError(t, err)
			assert.Equal(t, fv.String(), test.title)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// root 		---proposal---> 	propDocument
			// member 		---owns-------> 	propDocument
			// propDocument ---ownedby----> 	member
			checkEdge(t, env, env.Root, assignment, eos.Name("proposal"))
			checkEdge(t, env, proposer.Doc, assignment, eos.Name("owns"))
			checkEdge(t, env, assignment, proposer.Doc, eos.Name("ownedby"))
			checkEdge(t, env, assignment, test.role, eos.Name("role"))

			// roleHash, err := assignment.GetContent("role")
			// assert.NilError(t, err)

			// roleDocument, err := docgraph.LoadDocument(env.ctx, &env.api, env.DAO, roleHash.String())
			// assert.NilError(t, err)

			// // check edge from assignment to role
			// exists, err := docgraph.EdgeExists(env.ctx, &env.api, env.DAO, assignment, roleDocument, eos.Name("role"))
			// assert.NilError(t, err)
			// if !exists {
			// 	t.Log("Edge does not exist	: ", assignment.Hash.String(), "	-- role	--> 	", roleDocument.Hash.String())
			// }
			// assert.Check(t, exists)

			voteToPassTD(t, env, assignment)

			t.Log("Member: ", closer.Member, " is closing assignment proposal	: ", assignment.Hash.String())
			_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, assignment.Hash)
			assert.NilError(t, err)

			assignment, err = docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("assignment"))
			assert.NilError(t, err)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// update graph edges:
			//  member          ---- assigned           ---->   role_assignment
			//  role_assignment ---- assignee           ---->   member
			//  role_assignment ---- role               ---->   role
			//  role            ---- role_assignment    ---->   role_assignment
			checkEdge(t, env, assignee.Doc, assignment, eos.Name("assigned"))
			checkEdge(t, env, assignment, assignee.Doc, eos.Name("assignee"))
			checkEdge(t, env, assignment, test.role, eos.Name("role"))
			checkEdge(t, env, test.role, assignment, eos.Name("assignment"))

			//  root ---- passedprops        ---->   role_assignment
			checkEdge(t, env, env.Root, assignment, eos.Name("passedprops"))

			fetchedAssignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("assignment"))

			testHusdAsset, err := eos.NewAssetFromString(test.husd)
			assert.NilError(t, err)
			testHyphaAsset, err := eos.NewAssetFromString(test.hypha)
			assert.NilError(t, err)
			testHvoiceAsset, err := eos.NewAssetFromString(test.hvoice)
			assert.NilError(t, err)
			testUsdAsset, err := eos.NewAssetFromString(test.usd)
			assert.NilError(t, err)

			husd, err := fetchedAssignment.GetContent("husd_salary_per_phase")
			if testHusdAsset.Amount != 0 {
				assert.NilError(t, err)
				t.Log("test: ", test.name, ": husd: "+husd.String())
				assert.Equal(t, husd.String(), test.husd)
			} else {
				assert.ErrorContains(t, err, "content label not found")
			}

			hypha, err := fetchedAssignment.GetContent("hypha_salary_per_phase")
			if testHyphaAsset.Amount != 0 {
				assert.NilError(t, err)
				t.Log("test: ", test.name, ": hypha: "+hypha.String())
				assert.Equal(t, hypha.String(), test.hypha)
			} else {
				assert.ErrorContains(t, err, "content label not found")
			}

			hvoice, err := fetchedAssignment.GetContent("hvoice_salary_per_phase")
			if testHvoiceAsset.Amount != 0 {
				assert.NilError(t, err)
				t.Log("test: ", test.name, ": hvoice: "+hvoice.String())
				assert.Equal(t, hvoice.String(), test.hvoice)
			} else {
				assert.ErrorContains(t, err, "content label not found")
			}

			usd, err := fetchedAssignment.GetContent("usd_salary_value_per_phase")
			if testUsdAsset.Amount != 0 {
				assert.NilError(t, err)
				t.Log("test: ", test.name, ": usd: "+usd.String())
				assert.Equal(t, usd.String(), test.usd)
			} else {
				assert.ErrorContains(t, err, "content label not found")
			}
		}
	})
}

func TestAssignmentDefaults(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	// roles
	proposer := env.Members[0]
	assignee := env.Members[1]
	closer := env.Members[2]

	role1Doc := CreateRole(t, env, proposer, closer, role1)

	t.Run("Test Assignment Document Proposal", func(t *testing.T) {

		tests := []struct {
			name               string
			roleTitle          string
			title              string
			role               docgraph.Document
			assignment         string
			defaultPeriodCount int64
			hypha              string
			hvoice             string
			usd                string
		}{
			{
				name:               "role1 - 100% 100%",
				roleTitle:          "Underwater Basketweaver",
				title:              "Underwater Basketweaver - Atlantic",
				role:               role1Doc,
				assignment:         assignment8,
				defaultPeriodCount: 13,
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)
			_, err := dao.ProposeAssignment(env.ctx, &env.api, env.DAO, proposer.Member, assignee.Member, test.role.Hash, env.Periods[0].Hash, test.assignment)
			assert.NilError(t, err)

			// retrieve the document we just created
			assignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, assignment.Creator, proposer.Member)

			fv, err := assignment.GetContent("period_count")
			assert.NilError(t, err)
			assert.Equal(t, fv.Impl.(int64), test.defaultPeriodCount)
		}
	})
}

func TestOldAssignmentsPayClaim(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)

	proposer := env.Members[0]

	var balances []Balance

	balances = append(balances, NewBalance())

	t.Run("Test Assignment Document Proposal", func(t *testing.T) {

		tests := []struct {
			name       string
			roleTitle  string
			title      string
			role       string
			assignment string
			husd       string
			hypha      string
			hvoice     string
			usd        string
		}{
			{
				name:       "role2 - 100% commit, 70% deferred",
				roleTitle:  "Alfa Omega",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role2,
				assignment: assignment2,
				// husd:       "455.85 HUSD",
				// hypha:      "265.91 HYPHA",
				// hvoice:     "3039.00 HVOICE",
				// usd:        "1519.50 USD",
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)
			role := CreateRole(t, env, proposer, proposer, test.role)

			var assignment docgraph.Document
			var err error

			assignment, err = dao.CreateOldAssignment(t, env.ctx, &env.api, env.DAO, proposer.Member, proposer.Doc.Hash, role.Hash, env.Periods[0].Hash, assignment2)

			assert.NilError(t, err)

			assert.Equal(t, assignment.Creator, proposer.Member)

			//Emulate voting period
			t.Log("Waiting for a period to lapse...")
			pause(t, env.PeriodPause, "", "Waiting...")

			//Manually create the edges for passed assignments
			ExecuteDocgraphCall(t, env, func() {
				//Create edges
				_, err = docgraph.CreateEdge(env.ctx, &env.api, env.DAO, env.DAO, proposer.Doc.Hash, assignment.Hash, eos.Name("assigned"))

				assert.NilError(t, err)

				_, err = docgraph.CreateEdge(env.ctx, &env.api, env.DAO, env.DAO, assignment.Hash, proposer.Doc.Hash, eos.Name("assignee"))

				assert.NilError(t, err)

				_, err = docgraph.CreateEdge(env.ctx, &env.api, env.DAO, env.DAO, role.Hash, assignment.Hash, eos.Name("assignment"))

				assert.NilError(t, err)
			})

			fv, err := assignment.GetContent("title")
			assert.NilError(t, err)
			assert.Equal(t, fv.String(), test.title)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// update graph edges:
			//  member          ---- assigned           ---->   role_assignment
			//  role_assignment ---- assignee           ---->   member
			//  role_assignment ---- role               ---->   role
			//  role            ---- role_assignment    ---->   role_assignment
			checkEdge(t, env, proposer.Doc, assignment, eos.Name("assigned"))
			checkEdge(t, env, assignment, proposer.Doc, eos.Name("assignee"))
			//checkEdge(t, env, assignment, role, eos.Name("role"))
			checkEdge(t, env, role, assignment, eos.Name("assignment"))

			//  root ---- passedprops        ---->   role_assignment
			//checkEdge(t, env, env.Root, assignment, eos.Name("passedprops"))

			t.Log("Waiting for a period to lapse...")
			pause(t, env.PeriodPause, "", "Waiting...")

			_, err = ClaimNextPeriod(t, env, proposer.Member, assignment)
			assert.NilError(t, err)

			// Assignment hash will change due origina_approved_date item begin added the first time
			// we claim with an old assignment
			assignment, err = docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("assignment"))
			assert.NilError(t, err)

			fetchedAssignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("assignment"))

			husd, err := fetchedAssignment.GetContent("husd_salary_per_phase")
			assert.NilError(t, err)
			hypha, err := fetchedAssignment.GetContent("hypha_salary_per_phase")

			assert.NilError(t, err)

			hvoice, err := fetchedAssignment.GetContent("hvoice_salary_per_phase")
			assert.NilError(t, err)

			var payments []Balance
			// first payment is a partial payment, so should be less than the amount on the assignment record
			payments = append(payments, CalcLastPayment(t, env, balances[len(balances)-1], proposer.Member))
			balances = append(balances, GetBalance(t, env, proposer.Member))
			assert.Assert(t, hypha.Impl.(*eos.Asset).Amount >= payments[len(payments)-1].Hypha.Amount)
			assert.Assert(t, husd.Impl.(*eos.Asset).Amount >= payments[len(payments)-1].Husd.Amount)
			t.Log("Hvoice from payment      : ", strconv.Itoa(int(payments[len(payments)-1].Hvoice.Amount)))
			t.Log("Hvoice from assignment   : ", strconv.Itoa(int(hvoice.Impl.(*eos.Asset).Amount)))
			assert.Assert(t, hvoice.Impl.(*eos.Asset).Amount+eos.Int64(env.GenesisHVOICE) >= payments[len(payments)-1].Hvoice.Amount)
			//No seeds payment anymore
			//assert.Assert(t, payments[len(payments)-1].SeedsEscrow.Amount > 0)

			t.Log("Waiting for a period to lapse...")
			pause(t, env.PeriodPause, "", "Waiting...")

			_, err = ClaimNextPeriod(t, env, proposer.Member, assignment)
			assert.NilError(t, err)

			// 2nd payment should be equal to the payment on the assignment record
			payments = append(payments, CalcLastPayment(t, env, balances[len(balances)-1], proposer.Member))
			balances = append(balances, GetBalance(t, env, proposer.Member))
			assert.Equal(t, hypha.Impl.(*eos.Asset).Amount, payments[len(payments)-1].Hypha.Amount)
			assert.Equal(t, husd.Impl.(*eos.Asset).Amount, payments[len(payments)-1].Husd.Amount)
			assert.Equal(t, hvoice.Impl.(*eos.Asset).Amount, payments[len(payments)-1].Hvoice.Amount)
			//assert.Assert(t, payments[len(payments)-1].SeedsEscrow.Amount >= payments[len(payments)-2].SeedsEscrow.Amount)
		}
	})
}

func TestAssignmentPayClaim(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	balances = append(balances, NewBalance())

	// roles
	proposer := env.Members[0]
	assignee := env.Members[1]
	closer := env.Members[2]

	t.Run("Test Assignment Pay Claim", func(t *testing.T) {

		tests := []struct {
			name       string
			roleTitle  string
			title      string
			role       string
			assignment string
			husd       string
			hypha      string
			hvoice     string
			usd        string
		}{
			{
				name:       "role2 - 100% commit, 70% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role2,
				assignment: assignment2,
				// husd:       "455.85 HUSD",
				// hypha:      "265.91 HYPHA",
				// hvoice:     "3039.00 HVOICE",
				// usd:        "1519.50 USD",
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)
			role := CreateRole(t, env, proposer, closer, test.role)

			trxID, err := dao.ProposeAssignment(env.ctx, &env.api, env.DAO, proposer.Member, assignee.Member, role.Hash, env.Periods[0].Hash, test.assignment)
			t.Log("Assignment proposed: ", trxID)
			assert.NilError(t, err)

			// retrieve the document we just created
			assignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, assignment.Creator, proposer.Member)

			fv, err := assignment.GetContent("title")
			assert.NilError(t, err)
			assert.Equal(t, fv.String(), test.title)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// root 		---proposal---> 	propDocument
			// member 		---owns-------> 	propDocument
			// propDocument ---ownedby----> 	member
			checkEdge(t, env, env.Root, assignment, eos.Name("proposal"))
			checkEdge(t, env, proposer.Doc, assignment, eos.Name("owns"))
			checkEdge(t, env, assignment, proposer.Doc, eos.Name("ownedby"))

			voteToPassTD(t, env, assignment)

			t.Log("Member: ", closer.Member, " is closing assignment proposal	: ", assignment.Hash.String())
			_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, assignment.Hash)
			assert.NilError(t, err)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// update graph edges:
			//  member          ---- assigned           ---->   role_assignment
			//  role_assignment ---- assignee           ---->   member
			//  role_assignment ---- role               ---->   role
			//  role            ---- role_assignment    ---->   role_assignment
			checkEdge(t, env, assignee.Doc, assignment, eos.Name("assigned"))
			checkEdge(t, env, assignment, assignee.Doc, eos.Name("assignee"))
			checkEdge(t, env, assignment, role, eos.Name("role"))
			checkEdge(t, env, role, assignment, eos.Name("assignment"))

			//  root ---- passedprops        ---->   role_assignment
			checkEdge(t, env, env.Root, assignment, eos.Name("passedprops"))

			t.Log("Waiting for a period to lapse...")
			pause(t, env.PeriodPause, "", "Waiting...")

			_, err = ClaimNextPeriod(t, env, assignee.Member, assignment)
			assert.NilError(t, err)

			fetchedAssignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("assignment"))

			husd, err := fetchedAssignment.GetContent("husd_salary_per_phase")
			assert.NilError(t, err)

			hypha, err := fetchedAssignment.GetContent("hypha_salary_per_phase")
			assert.NilError(t, err)

			hvoice, err := fetchedAssignment.GetContent("hvoice_salary_per_phase")
			assert.NilError(t, err)

			// first payment is a partial payment, so should be less than the amount on the assignment record
			// SEEDS escrow payment should be greater than zero
			payments = append(payments, CalcLastPayment(t, env, balances[len(balances)-1], assignee.Member))
			balances = append(balances, GetBalance(t, env, assignee.Member))
			assert.Assert(t, hypha.Impl.(*eos.Asset).Amount >= payments[len(payments)-1].Hypha.Amount)
			assert.Assert(t, husd.Impl.(*eos.Asset).Amount >= payments[len(payments)-1].Husd.Amount)
			t.Log("Hvoice from payment      : ", strconv.Itoa(int(payments[len(payments)-1].Hvoice.Amount)))
			t.Log("Hvoice from assignment   : ", strconv.Itoa(int(hvoice.Impl.(*eos.Asset).Amount)))
			assert.Assert(t, hvoice.Impl.(*eos.Asset).Amount+eos.Int64(env.GenesisHVOICE) >= payments[len(payments)-1].Hvoice.Amount)
			assert.Assert(t, payments[len(payments)-1].SeedsEscrow.Amount > 0)

			t.Log("Waiting for a period to lapse...")
			pause(t, env.PeriodPause, "", "Waiting...")

			_, err = ClaimNextPeriod(t, env, assignee.Member, assignment)
			assert.NilError(t, err)

			// 2nd payment should be equal to the payment on the assignment record
			// 2nd SEEDS escrow payment should be greater than the first one
			payments = append(payments, CalcLastPayment(t, env, balances[len(balances)-1], assignee.Member))
			balances = append(balances, GetBalance(t, env, assignee.Member))
			assert.Equal(t, hypha.Impl.(*eos.Asset).Amount, payments[len(payments)-1].Hypha.Amount)
			assert.Equal(t, husd.Impl.(*eos.Asset).Amount, payments[len(payments)-1].Husd.Amount)
			assert.Equal(t, hvoice.Impl.(*eos.Asset).Amount, payments[len(payments)-1].Hvoice.Amount)
			assert.Assert(t, payments[len(payments)-1].SeedsEscrow.Amount >= payments[len(payments)-2].SeedsEscrow.Amount)
		}
	})
}

func TestAssignmentExtensionProposal(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	env = SetupEnvironment(t)
	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	balances = append(balances, NewBalance())

	// roles
	proposer := env.Members[0]
	assignee := env.Members[0]
	closer := env.Members[0]

	t.Run("Test Assignment Extension", func(t *testing.T) {

		tests := []struct {
			name       string
			roleTitle  string
			title      string
			role       string
			assignment string
			husd       string
			hypha      string
			hvoice     string
			usd        string
		}{
			{
				name:       "role2 - 100% commit, 70% deferred",
				roleTitle:  "Underwater Basketweaver",
				title:      "Underwater Basketweaver - Atlantic",
				role:       role2,
				assignment: assignment2,
				// husd:       "455.85 HUSD",
				// hypha:      "265.91 HYPHA",
				// hvoice:     "3039.00 HVOICE",
				// usd:        "1519.50 USD",
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)
			role := CreateRole(t, env, proposer, closer, test.role)

			trxID, err := dao.ProposeAssignment(env.ctx, &env.api, env.DAO, proposer.Member, assignee.Member, role.Hash, env.Periods[0].Hash, test.assignment)
			t.Log("Assignment proposed: ", trxID)
			assert.NilError(t, err)

			// retrieve the document we just created
			assignment, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, assignment.Creator, proposer.Member)

			fv, err := assignment.GetContent("title")
			assert.NilError(t, err)
			assert.Equal(t, fv.String(), test.title)

			originalPeriodCount, err := assignment.GetContent("period_count")
			assert.NilError(t, err)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// root 		---proposal---> 	propDocument
			// member 		---owns-------> 	propDocument
			// propDocument ---ownedby----> 	member
			checkEdge(t, env, env.Root, assignment, eos.Name("proposal"))
			checkEdge(t, env, proposer.Doc, assignment, eos.Name("owns"))
			checkEdge(t, env, assignment, proposer.Doc, eos.Name("ownedby"))

			voteToPassTD(t, env, assignment)

			t.Log("Member: ", closer.Member, " is closing assignment proposal	: ", assignment.Hash.String())
			_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, assignment.Hash)
			assert.NilError(t, err)

			pause(t, 1000000000, "", "Waiting before fetching assignment again")

			// Assignment hash will change due origina_approved_date item begin added the first time
			// we claim with an old assignment
			assignment, err = docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("assignment"))
			assert.NilError(t, err)

			// verify that the edges are created correctly
			// Graph structure post creating proposal:
			// update graph edges:
			//  member          ---- assigned           ---->   role_assignment
			//  role_assignment ---- assignee           ---->   member
			//  role_assignment ---- role               ---->   role
			//  role            ---- role_assignment    ---->   role_assignment
			checkEdge(t, env, assignee.Doc, assignment, eos.Name("assigned"))
			checkEdge(t, env, assignment, assignee.Doc, eos.Name("assignee"))
			checkEdge(t, env, assignment, role, eos.Name("role"))
			checkEdge(t, env, role, assignment, eos.Name("assignment"))

			//  root ---- passedprops        ---->   role_assignment
			checkEdge(t, env, env.Root, assignment, eos.Name("passedprops"))

			trxID, err = dao.ProposeAssExtension(env.ctx, &env.api, env.DAO, assignee.Member, assignment.Hash.String(), 13)
			assert.NilError(t, err)

			// retrieve the document we just created
			extensionProp, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
			assert.NilError(t, err)
			assert.Equal(t, extensionProp.Creator, proposer.Member)

			voteToPassTD(t, env, extensionProp)

			t.Log("Member: ", closer.Member, " is closing extension proposal	: ", extensionProp.Hash.String())
			_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, extensionProp.Hash)
			assert.NilError(t, err)

			originalPeriodCountValue, err := originalPeriodCount.Int64()
			assert.NilError(t, err)

			newPeriodCount, err := extensionProp.GetContent("period_count")
			assert.NilError(t, err)

			newPeriodCountValue, err := newPeriodCount.Int64()
			assert.NilError(t, err)

			assert.Equal(t, originalPeriodCountValue+13, newPeriodCountValue)
		}
	})
}

const base_assigment_adjust_x1 = `
{
  "content_groups": [
      [
        {
          "label": "assignemnt_id",
          "value": [
              "checksum256",
              "c9937a11790ff74d9465a779138572f4312a76c2db487964820f0f4177e9c905"
          ]
        },
        {
          "label": "new_time_share_x100",
          "value": [
              "int64",
              50
          ]
        }
      ]
  ]
`

const assignment1 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
              "label": "url",
              "value": [
                  "string",
                  "https://dho.hypha.earth"
              ]
            },
            {
                "label": "annual_usd_salary",
                "value": [
                    "asset",
                    "150000.00 USD"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    9
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    100
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    100
                ]
            }
        ]
    ]
}`

const assignment2 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    25
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    100
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    70
                ]
            }
        ]
    ]
}`

const assignment3 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    9
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    75
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    50
                ]
            }
        ]
    ]
}`

const assignment4 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    9
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    51
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    100
                ]
            }
        ]
    ]
}`

const assignment5 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    9
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    25
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    50
                ]
            }
        ]
    ]
}`

const assignment6 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    9
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    100
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    15
                ]
            }
        ]
    ]
}`

const assignment7 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "period_count",
                "value": [
                    "int64",
                    9
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    10
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    0
                ]
            }
        ]
    ]
}`

const assignment8 = `{
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Underwater Basketweaver - Atlantic"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Weave baskets at the bottom of the sea - Atlantic Ocean"
                ]
            },
            {
                "label": "time_share_x100",
                "value": [
                    "int64",
                    100
                ]
            },
            {
                "label": "deferred_perc_x100",
                "value": [
                    "int64",
                    100
                ]
            }
        ]
    ]
}`

package dao_test

// func TestBadgeProposals(t *testing.T) {

// 	teardownTestCase := setupTestCase(t)
// 	defer teardownTestCase(t)

// 	// var env Environment
// 	env = SetupEnvironment(t)

// 	// roles
// 	proposer := env.Members[0]
// 	assignee := env.Members[1]
// 	closer := env.Members[2]

// 	// balances at the start are always zero
// 	balances = append(balances, NewBalance())

// 	t.Run("Configuring the DAO environment: ", func(t *testing.T) {
// 		t.Log(env.String())
// 		t.Log("\nDAO Environment Setup complete\n")
// 	})

// t.Run("Test Roles & Assignments", func(t *testing.T) {

// 	tests := []struct {
// 		name       string
// 		role       string
// 		assignment string
// 	}{
// 		{
// 			name:       "Basketweaver",
// 			role:       role1,
// 			assignment: assignment1,
// 		},
// 	}

// 	for _, test := range tests {
// 		t.Run(test.name, func(t *testing.T) {
// 			// Create the role and assignment
// 			role = createRole(t, env, proposer.Member, closer.Member, test.role)
// 			assignment = createAssignment(t, env, role, proposer.Member, assignee.Member, closer.Member, test.assignment)

// 			// Claim the first period's pay
// 			t.Log("Waiting for a period to lapse...")
// 			pause(t, env.PeriodPause, "", "Waiting...")
// 			// _, err := dao.ClaimPay(env.ctx, &env.api, env.DAO, assignee.Member, assignment.ID, claimedPeriods)
// 			_, err := ClaimNextPeriod(t, env, assignee.Member, assignment)
// 			assert.NilError(t, err)
// 			// first payment is a partial payment, so should be less than the amount on the assignment record
// 			// SEEDS escrow payment should be greater than zero
// 			payments = append(payments, CalcLastPayment(t, env, balances[len(balances)-1], assignee.Member))
// 			balances = append(balances, GetBalance(t, env, assignee.Member))
// 			assert.Assert(t, assignment.Assets["hypha_salary_per_phase"].Amount >= payments[len(payments)-1].Hypha.Amount)
// 			assert.Assert(t, assignment.Assets["husd_salary_per_phase"].Amount >= payments[len(payments)-1].Husd.Amount)
// 			// assert.Assert(t, assignment.Assets["hvoice_salary_per_phase"].Amount >= payments[len(payments)-1].Hvoice.Amount)
// 			assert.Assert(t, payments[len(payments)-1].SeedsEscrow.Amount > 0)

// 			// Claim the 2nd periods's pay
// 			t.Log("Waiting for a period to lapse...")
// 			pause(t, env.PeriodPause, "", "Waiting...")
// 			// _, err = dao.ClaimPay(env.ctx, &env.api, env.DAO, assignee.Member, assignment.ID, claimedPeriods)
// 			_, err = ClaimNextPeriod(t, env, assignee.Member, assignment)
// 			assert.NilError(t, err)
// 			// 2nd payment should be equal to the payment on the assignment record
// 			// 2nd SEEDS escrow payment should be greater than the first one
// 			payments = append(payments, CalcLastPayment(t, env, balances[len(balances)-1], assignee.Member))
// 			balances = append(balances, GetBalance(t, env, assignee.Member))
// 			assert.Equal(t, assignment.Assets["hypha_salary_per_phase"].Amount, payments[len(payments)-1].Hypha.Amount)
// 			assert.Equal(t, assignment.Assets["husd_salary_per_phase"].Amount, payments[len(payments)-1].Husd.Amount)
// 			// assert.Equal(t, assignment.Assets["hvoice_salary_per_phase"].Amount, payments[len(payments)-1].Hvoice.Amount)
// 			assert.Assert(t, payments[len(payments)-1].SeedsEscrow.Amount >= payments[len(payments)-2].SeedsEscrow.Amount)
// 		})
// 	}
// })

// 	t.Run("Badge proposals", func(t *testing.T) {

// 		tests := []struct {
// 			name             string
// 			title            string
// 			badge            string
// 			badge_assignment string
// 			percChange       float64
// 		}{
// 			{
// 				name:             "enroller",
// 				title:            "Enroller",
// 				badge:            enroller_badge,
// 				badge_assignment: enroller_badge_assignment,
// 				percChange:       1.00,
// 			},
// 			{
// 				name:             "librarian",
// 				title:            "Badge Librarian",
// 				badge:            librarian_badge_proposal,
// 				badge_assignment: librarian_badge_assignment,
// 				percChange:       6.00,
// 			},
// 		}

// 		for _, test := range tests {
// 			t.Run(test.name, func(t *testing.T) {

// 				t.Log("\n\nStarting test: ", test.name)
// 				_, err := dao.ProposeBadgeFromFile(env.ctx, &env.api, env.DAO, proposer.Member, test.badge)
// 				assert.NilError(t, err)

// 				// retrieve the document we just created
// 				badgeDoc, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("proposal"))
// 				assert.NilError(t, err)
// 				assert.Equal(t, badgeDoc.Creator, proposer.Member)

// 				fv, err := badgeDoc.GetContent("title")
// 				assert.NilError(t, err)
// 				assert.Equal(t, fv.String(), test.title)

// 				// verify that the edges are created correctly
// 				// Graph structure post creating proposal:
// 				// root 		---proposal---> 	propDocument
// 				// member 		---owns-------> 	propDocument
// 				// propDocument ---ownedby----> 	member
// 				checkEdge(t, env, env.Root, badgeDoc, eos.Name("proposal"))
// 				checkEdge(t, env, proposer.Doc, badgeDoc, eos.Name("owns"))
// 				checkEdge(t, env, badgeDoc, proposer.Doc, eos.Name("ownedby"))

// 				ballot, err := badgeDoc.GetContent("ballot_id")
// 				assert.NilError(t, err)
// 				voteToPassTD(t, env, ballot.Impl.(eos.Name))

// 				t.Log("Member: ", closer.Member, " is closing badge proposal	: ", badgeDoc.Hash.String())
// 				_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, badgeDoc.Hash)
// 				assert.NilError(t, err)

// 				// verify that the edges are created correctly
// 				// Graph structure post creating proposal:
// 				// root 		---badge---> 	badgeDoc
// 				checkEdge(t, env, env.Root, badgeDoc, eos.Name("badge"))

// 				t.Log("Member: ", proposer.Member, " is submitting badge assignment proposal for	: "+string(assignee.Member)+"; badge: "+badgeDoc.Hash.String())
// 				pause(t, env.ChainResponsePause, "", "")

// 				_, err = dao.ProposeBadgeAssignment(env.ctx, &env.api, env.DAO, proposer.Member, assignee.Member, badgeDoc.Hash, test.badge_assignment)
// 				assert.NilError(t, err)

// 				badgeAssignmentDoc, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("propopsal"))
// 				assert.NilError(t, err)

// 				assignmentBallot, err := badgeAssignmentDoc.GetContent("ballot_id")
// 				assert.NilError(t, err)
// 				voteToPassTD(t, env, assignmentBallot.Impl.(eos.Name))

// 				t.Log("Member: ", closer.Member, " is closing badge assignment proposal	: ", badgeAssignmentDoc.Hash.String())
// 				_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, badgeAssignmentDoc.Hash)
// 				assert.NilError(t, err)

// 				// verify that the edges are created correctly
// 				// Graph structure post creating proposal:
// 				// update graph edges:
// 				//    member    ---- holdsbadge     ---->   badge
// 				//    member    ---- assignbadge    ---->   badge_assignment
// 				//    badge     ---- heldby         ---->   member
// 				//    badge     ---- assignment     ---->   badge_assignment
// 				//    badge_assignment     ---- badge    ---->   badge

// 				checkEdge(t, env, assignee.Doc, badgeDoc, eos.Name("holdsbadge"))
// 				checkEdge(t, env, assignee.Doc, badgeAssignmentDoc, eos.Name("assignbadge"))
// 				checkEdge(t, env, badgeDoc, assignee.Doc, eos.Name("heldby"))
// 				checkEdge(t, env, badgeDoc, badgeAssignmentDoc, eos.Name("assignment"))
// 				checkEdge(t, env, badgeAssignmentDoc, badgeDoc, eos.Name("badge"))

// 				// Claiming the next period
// 				t.Log("Waiting for a period to lapse...")
// 				pause(t, env.PeriodPause, "", "Waiting...")
// 				// _, err = dao.ClaimPay(env.ctx, &env.api, env.DAO, assignee.Member, assignment.ID, claimedPeriods)
// 				_, err = ClaimNextPeriod(t, env, assignee.Member, assignment)
// 				assert.NilError(t, err)
// 				// last balances are greater than the prior balances
// 				payments = append(payments, CalcLastPayment(t, env, balances[len(balances)-1], assignee.Member))
// 				balances = append(balances, GetBalance(t, env, assignee.Member))

// 				// balances are greater than before
// 				assert.Assert(t, balances[len(balances)-1].Hypha.Amount > balances[len(balances)-2].Hypha.Amount)
// 				assert.Assert(t, balances[len(balances)-1].Husd.Amount > balances[len(balances)-2].Husd.Amount)
// 				assert.Assert(t, balances[len(balances)-1].SeedsEscrow.Amount > balances[len(balances)-2].SeedsEscrow.Amount)
// 				// assert.Assert(t, balances[len(balances)-1].Hvoice.Amount > balances[len(balances)-2].Hvoice.Amount)

// 				// payments after the badge assignment are greater than the prior payment
// 				assert.Assert(t, payments[len(payments)-1].Hypha.Amount > payments[len(payments)-2].Hypha.Amount)
// 				assert.Assert(t, payments[len(payments)-1].Husd.Amount > payments[len(payments)-2].Husd.Amount)
// 				assert.Assert(t, payments[len(payments)-1].SeedsEscrow.Amount > payments[len(payments)-2].SeedsEscrow.Amount)
// 				// assert.Assert(t, payments[len(payments)-1].Hvoice.Amount > payments[len(payments)-2].Hvoice.Amount)

// 				hyphaPercDiff := PercentageChange(int(payments[0].Hypha.Amount), int(payments[len(payments)-1].Hypha.Amount))
// 				t.Logf("HYPHA percentage change : %0.2f %% \n", hyphaPercDiff)
// 				testassert.InDelta(t, hyphaPercDiff, test.percChange, 0.01)

// 				husdPercDiff := PercentageChange(int(payments[0].Husd.Amount), int(payments[len(payments)-1].Husd.Amount))
// 				t.Logf("HUSD percentage change : %0.2f %% \n", husdPercDiff)
// 				testassert.InDelta(t, husdPercDiff, test.percChange, 0.01)

// 				seedsPercDiff := PercentageChange(int(payments[0].SeedsEscrow.Amount), int(payments[len(payments)-1].SeedsEscrow.Amount))
// 				t.Logf("SEEDS percentage change : %0.2f %% \n", seedsPercDiff)
// 				testassert.InDelta(t, seedsPercDiff, test.percChange, 0.01)

// 			})
// 		}
// 	})
// }

// func TestSingleTokenBadge(t *testing.T) {

// 	teardownTestCase := setupTestCase(t)
// 	defer teardownTestCase(t)

// 	// var env Environment
// 	env = SetupEnvironment(t)

// 	// roles
// 	proposer := env.Members[0]
// 	assignee := env.Members[1]
// 	closer := env.Members[2]

// 	var assignment, role dao.V1Object

// 	// balances at the start are always zero
// 	balances = append(balances, NewBalance())

// 	t.Log(env.String())
// 	t.Log("\nDAO Environment Setup complete\n")
// 	t.Log("\nSetting up role and assignment to test single coefficient badge calculation\n")

// 	// setup a role and assignment to test single coefficient badge
// 	// Create the role and assignment
// 	role = createRole(t, env, proposer.Member, closer.Member, role1)
// 	assignment = createAssignment(t, env, role, proposer.Member, assignee.Member, closer.Member, assignment1)

// 	// Claim the first period's pay
// 	t.Log("Waiting for a period to lapse...")
// 	pause(t, env.PeriodPause, "", "Waiting...")
// 	_, err := ClaimNextPeriod(t, env, assignee.Member, assignment)
// 	assert.NilError(t, err)
// 	// first payment is a partial payment, so should be less than the amount on the assignment record
// 	// SEEDS escrow payment should be greater than zero
// 	payments = append(payments, CalcLastPayment(t, env, balances[len(balances)-1], assignee.Member))
// 	balances = append(balances, GetBalance(t, env, assignee.Member))
// 	assert.Assert(t, assignment.Assets["hypha_salary_per_phase"].Amount >= payments[len(payments)-1].Hypha.Amount)
// 	assert.Assert(t, assignment.Assets["husd_salary_per_phase"].Amount >= payments[len(payments)-1].Husd.Amount)
// 	// assert.Assert(t, assignment.Assets["hvoice_salary_per_phase"].Amount >= payments[len(payments)-1].Hvoice.Amount)
// 	assert.Assert(t, payments[len(payments)-1].SeedsEscrow.Amount > 0)

// 	// Claim the 2nd periods's pay
// 	t.Log("Waiting for a period to lapse...")
// 	pause(t, env.PeriodPause, "", "Waiting...")
// 	_, err = ClaimNextPeriod(t, env, assignee.Member, assignment)
// 	assert.NilError(t, err)
// 	// 2nd payment should be equal to the payment on the assignment record
// 	// 2nd SEEDS escrow payment should be greater than the first one
// 	payments = append(payments, CalcLastPayment(t, env, balances[len(balances)-1], assignee.Member))
// 	balances = append(balances, GetBalance(t, env, assignee.Member))
// 	assert.Equal(t, assignment.Assets["hypha_salary_per_phase"].Amount, payments[len(payments)-1].Hypha.Amount)
// 	assert.Equal(t, assignment.Assets["husd_salary_per_phase"].Amount, payments[len(payments)-1].Husd.Amount)
// 	// assert.Equal(t, assignment.Assets["hvoice_salary_per_phase"].Amount, payments[len(payments)-1].Hvoice.Amount)
// 	assert.Assert(t, payments[len(payments)-1].SeedsEscrow.Amount >= payments[len(payments)-2].SeedsEscrow.Amount)

// 	t.Run("Single coefficient badge calculation", func(t *testing.T) {

// 		tests := []struct {
// 			name             string
// 			title            string
// 			badge            string
// 			badge_assignment string
// 			percChange       float64
// 		}{
// 			{
// 				name:             "single_coeff",
// 				title:            "Single Coefficient",
// 				badge:            single_token_coefficient,
// 				badge_assignment: single_coeff_assignment,
// 				percChange:       2.30,
// 			},
// 		}

// 		for _, test := range tests {
// 			t.Run(test.name, func(t *testing.T) {

// 				t.Log("\n\nStarting test: ", test.name)
// 				_, err := dao.ProposeBadgeFromFile(env.ctx, &env.api, env.DAO, proposer.Member, test.badge)
// 				assert.NilError(t, err)

// 				// retrieve the document we just created
// 				badgeDoc, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("propopsal"))
// 				assert.NilError(t, err)
// 				assert.Equal(t, badgeDoc.Creator, proposer.Member)

// 				fv, err := badgeDoc.GetContent("title")
// 				assert.NilError(t, err)
// 				assert.Equal(t, fv.String(), test.title)

// 				// verify that the edges are created correctly
// 				// Graph structure post creating proposal:
// 				// root 		---proposal---> 	propDocument
// 				// member 		---owns-------> 	propDocument
// 				// propDocument ---ownedby----> 	member
// 				checkEdge(t, env, env.Root, badgeDoc, eos.Name("proposal"))
// 				checkEdge(t, env, proposer.Doc, badgeDoc, eos.Name("owns"))
// 				checkEdge(t, env, badgeDoc, proposer.Doc, eos.Name("ownedby"))

// 				ballot, err := badgeDoc.GetContent("ballot_id")
// 				assert.NilError(t, err)
// 				voteToPassTD(t, env, ballot.Impl.(eos.Name))

// 				t.Log("Member: ", closer.Member, " is closing badge proposal	: ", badgeDoc.Hash.String())
// 				_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, badgeDoc.Hash)
// 				assert.NilError(t, err)

// 				// verify that the edges are created correctly
// 				// Graph structure post creating proposal:
// 				// root 		---badge---> 	badgeDoc
// 				checkEdge(t, env, env.Root, badgeDoc, eos.Name("badge"))

// 				t.Log("Member: ", proposer.Member, " is submitting badge assignment proposal for	: "+string(assignee.Member)+"; badge: "+badgeDoc.Hash.String())
// 				pause(t, env.ChainResponsePause, "", "")

// 				_, err = dao.ProposeBadgeAssignment(env.ctx, &env.api, env.DAO, proposer.Member, assignee.Member, badgeDoc.Hash, test.badge_assignment)
// 				assert.NilError(t, err)

// 				badgeAssignmentDoc, err := docgraph.GetLastDocumentOfEdge(env.ctx, &env.api, env.DAO, eos.Name("propopsal"))
// 				assert.NilError(t, err)

// 				assignmentBallot, err := badgeAssignmentDoc.GetContent("ballot_id")
// 				assert.NilError(t, err)
// 				voteToPassTD(t, env, assignmentBallot.Impl.(eos.Name))

// 				t.Log("Member: ", closer.Member, " is closing badge assignment proposal	: ", badgeAssignmentDoc.Hash.String())
// 				_, err = dao.CloseProposal(env.ctx, &env.api, env.DAO, closer.Member, badgeAssignmentDoc.Hash)
// 				assert.NilError(t, err)

// 				// verify that the edges are created correctly
// 				// Graph structure post creating proposal:
// 				// update graph edges:
// 				//    member    ---- holdsbadge     ---->   badge
// 				//    member    ---- assignbadge    ---->   badge_assignment
// 				//    badge     ---- heldby         ---->   member
// 				//    badge     ---- assignment     ---->   badge_assignment
// 				checkEdge(t, env, assignee.Doc, badgeDoc, eos.Name("holdsbadge"))
// 				checkEdge(t, env, assignee.Doc, badgeAssignmentDoc, eos.Name("assignbadge"))
// 				checkEdge(t, env, badgeDoc, assignee.Doc, eos.Name("heldby"))
// 				checkEdge(t, env, badgeDoc, badgeAssignmentDoc, eos.Name("assignment"))
// 				checkEdge(t, env, badgeAssignmentDoc, badgeDoc, eos.Name("badge"))

// 				// Claiming the next period
// 				t.Log("Waiting for a period to lapse...")
// 				pause(t, env.PeriodPause, "", "Waiting...")
// 				_, err = ClaimNextPeriod(t, env, assignee.Member, assignment)
// 				assert.NilError(t, err)
// 				// last balances are greater than the prior balances
// 				payments = append(payments, CalcLastPayment(t, env, balances[len(balances)-1], assignee.Member))
// 				balances = append(balances, GetBalance(t, env, assignee.Member))

// 				// balances are greater than before
// 				assert.Assert(t, balances[len(balances)-1].Hypha.Amount > balances[len(balances)-2].Hypha.Amount)
// 				assert.Assert(t, balances[len(balances)-1].Husd.Amount > balances[len(balances)-2].Husd.Amount)
// 				assert.Assert(t, balances[len(balances)-1].SeedsEscrow.Amount > balances[len(balances)-2].SeedsEscrow.Amount)
// 				// assert.Assert(t, balances[len(balances)-1].Hvoice.Amount > balances[len(balances)-2].Hvoice.Amount)

// 				// payments after the badge assignment are equal for HUSD and SEEDS but greater for HYPHA
// 				assert.Assert(t, payments[len(payments)-1].Hypha.Amount > payments[len(payments)-2].Hypha.Amount)
// 				assert.Equal(t, payments[len(payments)-1].Husd.Amount, payments[len(payments)-2].Husd.Amount)
// 				assert.Equal(t, payments[len(payments)-1].SeedsEscrow.Amount, payments[len(payments)-2].SeedsEscrow.Amount)
// 				// assert.Assert(t, payments[len(payments)-1].Hvoice.Amount > payments[len(payments)-2].Hvoice.Amount)

// 				hyphaPercDiff := PercentageChange(int(payments[0].Hypha.Amount), int(payments[len(payments)-1].Hypha.Amount))
// 				t.Logf("HYPHA percentage change : %0.2f %% \n", hyphaPercDiff)
// 				testassert.InDelta(t, hyphaPercDiff, test.percChange, 0.01)
// 			})
// 		}
// 	})
// }

const enroller_badge = `{
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
                    "Enroller"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Enrollers authorize new membership and hold administative duties of the Hypha organization"
                ]
            },
            {
                "label": "icon",
                "value": [
                    "string",
                    "https://gallery.yopriceville.com/var/resizes/Free-Clipart-Pictures/Badges-and-Labels-PNG/Gold_Badge_Transparent_PNG_Image.png"
                ]
            },
            {
                "label": "seeds_coefficient_x10000",
                "value": [
                    "int64",
                    10100
                ]
			},
			{
                "label": "hypha_coefficient_x10000",
                "value": [
                    "int64",
                    10100
                ]
			},
			{
                "label": "husd_coefficient_x10000",
                "value": [
                    "int64",
                    10100
                ]
			},
			{
                "label": "hvoice_coefficient_x10000",
                "value": [
                    "int64",
                    10100
                ]
            }
        ]
    ]
}`

const enroller_badge_assignment = `{
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
                    "Enroller Badge Assignment"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Enrollers authorize new membership and hold administative duties of the Hypha organization"
                ]
            },
            {
                "label": "start_period",
                "value": [
                    "int64",
                    2
                ]
            },
            {
                "label": "end_period",
                "value": [
                    "int64",
                    9
                ]
            }
        ]
    ]
}`

const librarian_badge_proposal = `{
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
                    "Badge Librarian"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Ensure that badges represent clear, clean, and useful distinctions for the organization"
                ]
            },
            {
                "label": "icon",
                "value": [
                    "string",
                    "https://gallery.yopriceville.com/var/resizes/Free-Clipart-Pictures/Badges-and-Labels-PNG/Gold_Badge_Transparent_PNG_Image.png"
                ]
            },
            {
                "label": "seeds_coefficient_x10000",
                "value": [
                    "int64",
                    10500
                ]
			},
			{
                "label": "hypha_coefficient_x10000",
                "value": [
                    "int64",
                    10500
                ]
			},
			{
                "label": "hvoice_coefficient_x10000",
                "value": [
                    "int64",
                    10500
                ]
			},
			{
                "label": "husd_coefficient_x10000",
                "value": [
                    "int64",
                    10500
                ]
            }
        ]
    ]
}`

const librarian_badge_assignment = `{
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
                    "Librarian Badge Assignment"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Librarians are responsible for helping to construct new badges"
                ]
            },
            {
                "label": "start_period",
                "value": [
                    "int64",
                    2
                ]
            },
            {
                "label": "end_period",
                "value": [
                    "int64",
                    9
                ]
            }
        ]
    ]
}`

const single_token_coefficient = `{
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
                    "Single Coefficient"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Ensure that other coefficients default to 10000 if they are not provided"
                ]
            },
            {
                "label": "icon",
                "value": [
                    "string",
                    "https://gallery.yopriceville.com/var/resizes/Free-Clipart-Pictures/Badges-and-Labels-PNG/Gold_Badge_Transparent_PNG_Image.png"
                ]
            },
            {
                "label": "hypha_coefficient_x10000",
                "value": [
                    "int64",
                    10230
                ]
			}
        ]
    ]
}`

const single_coeff_assignment = `{
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
                    "Single Coefficient Assignment"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Only HYPHA token should have an increase on this badge"
                ]
            },
            {
                "label": "start_period",
                "value": [
                    "int64",
                    2
                ]
            },
            {
                "label": "end_period",
                "value": [
                    "int64",
                    9
                ]
            }
        ]
    ]
}`

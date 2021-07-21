project(dao)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

# Activate Logging
add_compile_definitions(USE_LOGGING)

add_contract( dao dao 
                dao.cpp
                typed_document.cpp
                util.cpp 
                period.cpp
                member.cpp
                assignment.cpp
                time_share.cpp
                payers/payer.cpp
                payers/escrow_payer.cpp
                payers/husd_payer.cpp
                payers/voice_payer.cpp
                payers/reward_payer.cpp
                payers/seeds_payer.cpp 
                payers/payer_factory.cpp
                proposals/proposal.cpp
                proposals/role_proposal.cpp
                proposals/badge_proposal.cpp
                proposals/badge_assignment_proposal.cpp
                proposals/ass_extend_proposal.cpp
                proposals/assignment_proposal.cpp
                proposals/payout_proposal.cpp
                proposals/attestation_proposal.cpp
                proposals/edit_proposal.cpp
                proposals/suspend_proposal.cpp
                proposals/proposal_factory.cpp
                ballots/vote.cpp
                ballots/vote_tally.cpp
                ../document-graph/src/document_graph/document_graph.cpp 
                ../document-graph/src/document_graph/document.cpp 
                ../document-graph/src/document_graph/content_wrapper.cpp
                ../document-graph/src/document_graph/content.cpp
                ../document-graph/src/document_graph/edge.cpp
                ../document-graph/src/document_graph/util.cpp 
                ../document-graph/src/logger/logger.cpp 
            )

target_include_directories( dao PUBLIC ${CMAKE_SOURCE_DIR}/../include ${CMAKE_SOURCE_DIR}/../document-graph/include )
target_ricardian_directory( dao ${CMAKE_SOURCE_DIR}/../ricardian )
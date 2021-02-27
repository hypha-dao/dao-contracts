project(dao)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( dao dao 
                dao.cpp
                util.cpp 
                period.cpp
                member.cpp
                assignment.cpp
                time_share.cpp
                payers/payer.cpp
                payers/escrow_payer.cpp
                payers/husd_payer.cpp
                payers/hvoice_payer.cpp
                payers/hypha_payer.cpp
                payers/seeds_payer.cpp 
                payers/payer_factory.cpp
                proposals/proposal.cpp
                proposals/role_proposal.cpp
                proposals/badge_proposal.cpp
                proposals/badge_assignment_proposal.cpp
                proposals/assignment_proposal.cpp
                proposals/payout_proposal.cpp
                proposals/attestation_proposal.cpp
                proposals/edit_proposal.cpp
                proposals/proposal_factory.cpp
                ../document-graph/src/document_graph/document_graph.cpp 
                ../document-graph/src/document_graph/document.cpp 
                ../document-graph/src/document_graph/content_wrapper.cpp
                ../document-graph/src/document_graph/content.cpp
                ../document-graph/src/document_graph/edge.cpp
                ../document-graph/src/document_graph/util.cpp 
            )

target_include_directories( dao PUBLIC ${CMAKE_SOURCE_DIR}/../include ${CMAKE_SOURCE_DIR}/../document-graph/include )
target_ricardian_directory( dao ${CMAKE_SOURCE_DIR}/../ricardian )
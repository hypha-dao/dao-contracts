module github.com/hypha-dao/dao-contracts

go 1.15

require (
	github.com/digital-scarcity/eos-go-test v0.0.0-20201030135239-784ff05708c0
	github.com/eoscanada/eos-go v0.9.1-0.20200805141443-a9d5402a7bc5
	github.com/hypha-dao/dao-contracts/dao-go v0.0.0-20201229192602-b03263aebb57
)

replace github.com/hypha-dao/dao-contracts/dao-go => ./dao-go

replace github.com/hypha-dao/document-graph/docgraph => ../document-graph/docgraph

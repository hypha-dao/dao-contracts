module github.com/hypha-dao/dao-contracts/dao-go

go 1.16

require (
	github.com/alexeyco/simpletable v0.0.0-20200730140406-5bb24159ccfb
	github.com/digital-scarcity/eos-go-test v0.0.0-20201030135239-784ff05708c0
	github.com/eoscanada/eos-go v0.9.1-0.20200805141443-a9d5402a7bc5
	github.com/hypha-dao/document-graph/docgraph v0.0.0-20201229193929-e09f4b1c9e47
	github.com/k0kubun/go-ansi v0.0.0-20180517002512-3bf9e2903213
	github.com/schollz/progressbar/v3 v3.7.4
	github.com/spf13/viper v1.3.1
	github.com/stretchr/testify v1.7.0
	gotest.tools v2.2.0+incompatible
)

replace github.com/hypha-dao/document-graph/docgraph => ../document-graph/docgraph

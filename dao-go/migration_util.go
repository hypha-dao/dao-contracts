package dao

import (
	"context"
	"fmt"
	"strconv"
	"time"

	eostest "github.com/digital-scarcity/eos-go-test"
	"github.com/eoscanada/eos-go"
	"github.com/hypha-dao/document-graph/docgraph"
	"github.com/schollz/progressbar/v3"
	"github.com/spf13/viper"
)

func defaultPause() time.Duration {
	if viper.IsSet("pause") {
		return viper.GetDuration("pause")
	}
	return time.Second
}

func defaultPeriodDuration() time.Duration {
	if viper.IsSet("periodDuration") {
		return viper.GetDuration("periodDuration")
	}
	return time.Second * 130
}

// DefaultProgressBar ...
func DefaultProgressBar(counter int) *progressbar.ProgressBar {
	return progressbar.Default(int64(counter))
	// ,
	// 	progressbar.OptionSetWriter(ansi.NewAnsiStdout()),
	// 	progressbar.OptionEnableColorCodes(true),
	// 	progressbar.OptionSetWidth(90),
	// 	// progressbar.OptionShowIts(),
	// 	progressbar.OptionSetDescription("[cyan]"+fmt.Sprintf("%20v", "")),
	// 	progressbar.OptionSetTheme(progressbar.Theme{
	// 		Saucer:        "[green]=[reset]",
	// 		SaucerHead:    "[green]>[reset]",
	// 		SaucerPadding: " ",
	// 		BarStart:      "[",
	// 		BarEnd:        "]",
	// 	}))
}

func pause(seconds time.Duration, headline, prefix string) {
	if headline != "" {
		fmt.Println(headline)
	}

	bar := DefaultProgressBar(100)

	chunk := seconds / 100
	for i := 0; i < 100; i++ {
		bar.Add(1)
		time.Sleep(chunk)
	}
	fmt.Println()
}

type eraseDoc struct {
	Hash eos.Checksum256 `json:"hash"`
}

// EraseAllDocuments ...
func EraseAllDocuments(ctx context.Context, api *eos.API, contract eos.AccountName) {

	documents, err := docgraph.GetAllDocuments(ctx, api, contract)
	if err != nil {
		fmt.Println(err)
	}

	fmt.Println("\nErasing documents: " + strconv.Itoa(len(documents)))
	bar := DefaultProgressBar(len(documents))

	for _, document := range documents {

		typeFV, err := document.GetContent("type")
		docType := typeFV.Impl.(eos.Name)

		if err != nil ||
			docType == eos.Name("settings") ||
			docType == eos.Name("dho") {
			// do not erase
		} else {
			actions := []*eos.Action{{
				Account: contract,
				Name:    eos.ActN("erasedoc"),
				Authorization: []eos.PermissionLevel{
					{Actor: contract, Permission: eos.PN("active")},
				},
				ActionData: eos.NewActionData(eraseDoc{
					Hash: document.Hash,
				}),
			}}

			_, err := eostest.ExecWithRetry(ctx, api, actions)
			if err != nil {
				// too many false positives
				fmt.Println("\nFailed to erase : ", document.Hash.String())
				fmt.Println(err)
			} else {
				time.Sleep(defaultPause())
			}
		}
		bar.Add(1)
	}
}

package dao

import (
	"time"

	eos "github.com/eoscanada/eos-go"
)

// QrAction ...
type QrAction struct {
	Timestamp      time.Time `json:"@timestamp"`
	TrxID          string    `json:"trx_id"`
	ActionContract string
	ActionName     string
	Data           string
}

// Scope ...
type Scope struct {
	Code  eos.Name `json:"code"`
	Scope eos.Name `json:"scope"`
	Table eos.Name `json:"table"`
	Payer eos.Name `json:"payer"`
	Count uint64   `json:"count"`
}

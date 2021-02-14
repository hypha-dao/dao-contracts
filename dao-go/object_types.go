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

// NameKV struct
type NameKV struct {
	Key   string   `json:"key"`
	Value eos.Name `json:"value"`
}

// StringKV struct
type StringKV struct {
	Key   string `json:"key"`
	Value string `json:"value"`
}

// AssetKV struct
type AssetKV struct {
	Key   string    `json:"key"`
	Value eos.Asset `json:"value"`
}

// TimePointKV struct
type TimePointKV struct {
	Key   string        `json:"key"`
	Value eos.TimePoint `json:"value"`
}

// IntKV struct
type IntKV struct {
	Key   string `json:"key"`
	Value uint64 `json:"value"`
}

// Scope ...
type Scope struct {
	Code  eos.Name `json:"code"`
	Scope eos.Name `json:"scope"`
	Table eos.Name `json:"table"`
	Payer eos.Name `json:"payer"`
	Count uint64   `json:"count"`
}

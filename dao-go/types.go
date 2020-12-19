package dao

import (
	eos "github.com/eoscanada/eos-go"
)

// QrAction ...
// type QrAction struct {
// 	Timestamp      time.Time `json:"@timestamp"`
// 	TrxID          string    `json:"trx_id"`
// 	ActionContract string
// 	ActionName     string
// 	Data           string
// }

// Scope ...
// type Scope struct {
// 	Code  eos.Name `json:"code"`
// 	Scope eos.Name `json:"scope"`
// 	Table eos.Name `json:"table"`
// 	Payer eos.Name `json:"payer"`
// 	Count uint64   `json:"count"`
// }

// SeedsExchConfigTable ...
type SeedsExchConfigTable struct {
	SeedsPerUsd   eos.Asset `json:"seeds_per_usd"`
	TlosPerUsd    eos.Asset `json:"tlos_per_usd"`
	CitizenLimit  eos.Asset `json:"citizen_limit"`
	ResidentLimit eos.Asset `json:"resident_limit"`
	VisitorLimit  eos.Asset `json:"visitor_limit"`
}

// SeedsPriceHistory ...
type SeedsPriceHistory struct {
	ID       uint64        `json:"id"`
	SeedsUSD eos.Asset     `json:"seeds_usd"`
	Date     eos.TimePoint `json:"date"`
}

// Period represents a period of time aligning to a payroll period, typically a week
type Period struct {
	PeriodID  uint64        `json:"period_id"`
	StartTime eos.TimePoint `json:"start_time"`
	EndTime   eos.TimePoint `json:"end_time"`
	Phase     string        `json:"phase"`
}

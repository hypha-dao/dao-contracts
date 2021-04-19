package dao_test

import (
	"os"
	"os/exec"
	"testing"
	"time"

	"gotest.tools/assert"
	eostest "github.com/digital-scarcity/eos-go-test"
)

var env *Environment

var payments []Balance
var balances []Balance

// func TestMain(m *testing.M) {
// 	log.SetOutput(ansi.NewAnsiStdout())
// 	os.Exit(m.Run())
// }

func setupTestCase(t *testing.T) func(t *testing.T) {
	t.Log("Bootstrapping testing environment ...")

	_, err := exec.Command("sh", "-c", "pkill -SIGINT nodeos").Output()
	if err == nil {
		eostest.Pause(time.Second, "Killing nodeos ...", "")
	}

	t.Log("Starting nodeos from 'nodeos.sh' script ...")
	cmd := exec.Command("./nodeos.sh")

    outfile, err := os.Create("./nodeos-go.log")
    assert.NilError(t, err)

    defer outfile.Close()
    cmd.Stdout = outfile
    cmd.Stderr = outfile
	err = cmd.Start()
	assert.NilError(t, err)

	t.Log("nodeos PID: ", cmd.Process.Pid)

	eostest.Pause(time.Second, "", "")

	return func(t *testing.T) {
		t.Log(" **********    Payments   **********")
		for _, p := range payments {
			t.Log(p.String())
		}

		t.Log(" **********    Balances   **********")
		for _, b := range balances {
			t.Log(b.String())
		}

		folderName := "test_results"
		t.Log("Saving graph to : ", folderName)
		err := SaveGraph(env.ctx, &env.api, env.DAO, folderName)
		assert.NilError(t, err)
	}
}

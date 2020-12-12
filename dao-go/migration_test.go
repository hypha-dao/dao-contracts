package dao_test

import (
	"testing"
)

func TestMigration(t *testing.T) {
	teardownTestCase := setupTestCase(t)
	defer teardownTestCase(t)

	// var env Environment
	env = SetupEnvironment(t)

	t.Log(env.String())
	t.Log("\nDAO Environment Setup complete\n")

	t.Run("Test Object Migration", func(t *testing.T) {

		tests := []struct {
			name  string
			title string
			role  string
		}{
			{
				name:  "Basketweaver",
				title: "Underwater Basketweaver",
				role:  role1,
			},
		}

		for _, test := range tests {

			t.Log("\n\nStarting test: ", test.name)

		}
	})
}

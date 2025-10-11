package main

import (
	"context"
	"log"
	"os"

	"github.com/urfave/cli/v3"
	"e-paper-cli/cmd"
)

func main() {
	app := &cli.Command{
		Name:  "e-paper-cli",
		Usage: "CLI tool for managing e-paper display firmware updates",
		Commands: []*cli.Command{
			cmd.NewUpdateDisplayCommand(),
		},
	}

	if err := app.Run(context.Background(), os.Args); err != nil {
		log.Fatal(err)
	}
}

package cmd

import (
	"github.com/spf13/cobra"
)

var rootCmd = &cobra.Command{
	Use:   "vm-orchestrator",
	Short: "VM Orchestrator for DPDK Traffic Testing",
}

func Execute() error {
	return rootCmd.Execute()
}

func init() {
	rootCmd.AddCommand(startCmd)
	rootCmd.AddCommand(stopCmd)
	rootCmd.AddCommand(topologyCmd)
	rootCmd.AddCommand(ipsCmd)
	rootCmd.AddCommand(runTrafficCmd)
	rootCmd.AddCommand(helpCmd)
} 
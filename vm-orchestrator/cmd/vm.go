package cmd

import (
	"fmt"
	"os/exec"

	"github.com/spf13/cobra"
)

var startCmd = &cobra.Command{
	Use:   "start [vm-name]",
	Short: "Start a VM",
	Long: `Start a VM using virsh start command.
This will power on the specified VM.`,
	Example: `  vm-orchestrator start <vm-name>`,
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		vmName := args[0]
		exec.Command("virsh", "start", vmName).Run()
		fmt.Printf("Started VM: %s\n", vmName)
	},
}

var stopCmd = &cobra.Command{
	Use:   "stop [vm-name]",
	Short: "Stop a VM",
	Long: `Stop a VM using virsh destroy command.
This will forcefully shut down the specified VM.`,
	Example: `  vm-orchestrator stop <vm-name>`,
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		vmName := args[0]
		exec.Command("virsh", "destroy", vmName).Run()
		fmt.Printf("Stopped VM: %s\n", vmName)
	},
}

var topologyCmd = &cobra.Command{
	Use:   "topology",
	Short: "Show VM topology",
	Long: `Show VM topology using virsh list --all.
This displays all VMs and their current status.`,
	Example: `  vm-orchestrator topology`,
	Run: func(cmd *cobra.Command, args []string) {
		output, _ := exec.Command("virsh", "list", "--all").Output()
		fmt.Println(string(output))
	},
}

var ipsCmd = &cobra.Command{
	Use:   "ips [vm-name]",
	Short: "Show VM IP addresses",
	Long: `Show VM IP addresses using virsh domifaddr --source agent.
This requires the VM to be running and have the qemu-guest-agent installed.`,
	Example: `  vm-orchestrator ips <vm-name>`,
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		vmName := args[0]
		output, _ := exec.Command("virsh", "domifaddr", "--source", "agent", vmName).Output()
		fmt.Println(string(output))
	},
} 
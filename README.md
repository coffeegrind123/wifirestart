# Wi-Fi Interface Monitor

A Windows utility designed to monitor and automatically re-enable Wi-Fi interfaces, particularly useful for remote management scenarios where Wi-Fi might be accidentally disabled.

## Features

- Monitors the state of Intel AX200 Wi-Fi adapter
- Automatically attempts to re-enable the interface if it becomes disabled
- Runs with administrator privileges (required for WLAN operations)
- Provides real-time status updates in the console
- Handles various Wi-Fi interface states including disconnected, disabled, and connected states

## Requirements

- Windows 10/11
- Visual Studio 2022 Build Tools (or full Visual Studio 2022)
- Administrator privileges
- Intel AX200 Wi-Fi adapter (code can be modified for other adapters)

## Building

1. Ensure you have Visual Studio 2022 Build Tools installed
2. Run `compile.cmd` to build the executable
3. The compiled executable will be named `wifi_monitor.exe`

## Usage

1. Run `wifi_monitor.exe` as administrator
   - If not run as administrator, the program will request elevation
2. The program will:
   - Display the current Wi-Fi interface state
   - Monitor for state changes
   - Automatically attempt to re-enable the interface if disabled
   - Show a running status every 10 checks

## Common States

- `DISABLED`: Interface is turned off
- `CONNECTED`: Successfully connected to a network
- `DISCONNECTED`: Not connected to any network
- `DISCONNECTING`: In the process of disconnecting
- `CONNECTING`: In the process of connecting
- `DISCOVERING`: Scanning for networks
- `AUTHENTICATING`: Authenticating with a network

## Error Codes

Common Windows error codes you might see:
- 50 (ERROR_NOT_SUPPORTED): Operation not supported in current state
- 87 (ERROR_INVALID_PARAMETER): Invalid parameter passed to function

## Use Case

This utility is particularly useful for:
- Remote management scenarios where Wi-Fi might be accidentally disabled
- Preventing loss of remote access due to Wi-Fi being turned off
- Automated recovery of Wi-Fi connectivity

## Building from Source

The project consists of two main files:
- `wifi_restart.c`: The main source code
- `compile.cmd`: Build script for Visual Studio tools

To modify for a different Wi-Fi adapter, update the interface description string in the code:
```c
if (wcsstr(pIfInfo->strInterfaceDescription, L"Intel(R) Wi-Fi 6 AX200") != NULL)
```

## Notes

- The program requires administrator privileges due to Windows security requirements for WLAN operations
- The monitoring loop runs indefinitely until terminated with Ctrl+C
- Console window refreshes every 10 status checks to prevent overflow
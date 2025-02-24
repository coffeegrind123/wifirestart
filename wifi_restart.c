#include <windows.h>
#include <wlanapi.h>
#include <stdio.h>
#include <stdbool.h>
#include <shellapi.h>

#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

// Check if the program is running with admin rights
BOOL IsElevated() {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    return fRet;
}

// Function to restart itself with admin rights
void RestartAsAdmin() {
    wchar_t szPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, szPath, MAX_PATH)) {
        // Execute new process with admin rights
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = szPath;
        sei.hwnd = NULL;
        sei.nShow = SW_NORMAL;

        if (!ShellExecuteExW(&sei)) {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_CANCELLED) {
                printf("Operation cancelled by user. Please run as administrator.\n");
            } else {
                printf("Failed to restart with admin rights. Error: %lu\n", dwError);
            }
        }
    }
}

const char* getStateString(WLAN_INTERFACE_STATE state) {
    switch (state) {
        case wlan_interface_state_not_ready: return "DISABLED";
        case wlan_interface_state_connected: return "CONNECTED";
        case wlan_interface_state_ad_hoc_network_formed: return "AD_HOC";
        case wlan_interface_state_disconnecting: return "DISCONNECTING";
        case wlan_interface_state_disconnected: return "DISCONNECTED";
        case wlan_interface_state_associating: return "CONNECTING";
        case wlan_interface_state_discovering: return "DISCOVERING";
        case wlan_interface_state_authenticating: return "AUTHENTICATING";
        default: return "UNKNOWN";
    }
}

void restartInterface(HANDLE hClient, PWLAN_INTERFACE_INFO pIfInfo) {
    DWORD dwResult;
    printf("[%s] Current State: %s\n", __TIME__, getStateString(pIfInfo->isState));

    // Try to enable using radio state control with proper struct
    printf("[%s] Enabling Wi-Fi radio...\n", __TIME__);
    WLAN_PHY_RADIO_STATE radioState;
    ZeroMemory(&radioState, sizeof(WLAN_PHY_RADIO_STATE));
    radioState.dwPhyIndex = 0;  // First radio
    radioState.dot11SoftwareRadioState = dot11_radio_state_on;
    radioState.dot11HardwareRadioState = dot11_radio_state_on;

    dwResult = WlanSetInterface(
        hClient,
        &pIfInfo->InterfaceGuid,
        wlan_intf_opcode_radio_state,
        sizeof(WLAN_PHY_RADIO_STATE),
        &radioState,
        NULL
    );

    if (dwResult != ERROR_SUCCESS) {
        printf("[%s] Error: Failed to enable radio: %lu\n", __TIME__, dwResult);
        
        // If radio state fails, try enabling at the interface level
        printf("[%s] Trying interface level enable...\n", __TIME__);
        BOOL bEnabled = TRUE;
        dwResult = WlanSetInterface(
            hClient,
            &pIfInfo->InterfaceGuid,
            wlan_intf_opcode_autoconf_enabled,
            sizeof(BOOL),
            &bEnabled,
            NULL
        );
        
        if (dwResult != ERROR_SUCCESS) {
            printf("[%s] Error: Failed to enable autoconf: %lu\n", __TIME__, dwResult);
        } else {
            printf("[%s] Successfully enabled interface autoconf\n", __TIME__);
        }
    } else {
        printf("[%s] Successfully enabled Wi-Fi radio\n", __TIME__);
    }

    // Additional wait after enabling to allow interface to stabilize
    Sleep(3000);
}

int main() {
    // Check if running as admin
    if (!IsElevated()) {
        printf("This program requires administrator rights.\n");
        printf("Requesting elevation...\n");
        RestartAsAdmin();
        return 0;
    }

    HANDLE hClient = NULL;
    DWORD dwMaxClient = 2;      
    DWORD dwCurVersion = 0;
    DWORD dwResult = 0;
    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    PWLAN_INTERFACE_INFO pIfInfo = NULL;
    bool interfaceFound = false;
    GUID targetInterfaceGuid;
    int checkCount = 0;
    WLAN_INTERFACE_STATE lastState = wlan_interface_state_not_ready;
    bool needsRestart = false;

    SetConsoleTitle(L"Wi-Fi Interface Monitor [ADMIN]");
    printf("Wi-Fi Interface Monitor Starting (Administrator Mode)...\n");
    printf("Press Ctrl+C to exit\n\n");

    // Open handle to WLAN API
    dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
    if (dwResult != ERROR_SUCCESS) {
        printf("Error: WlanOpenHandle failed with error: %lu\n", dwResult);
        system("pause");
        return 1;
    }

    // First, find our target interface and store its GUID
    dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
    if (dwResult != ERROR_SUCCESS) {
        printf("Error: WlanEnumInterfaces failed with error: %lu\n", dwResult);
        WlanCloseHandle(hClient, NULL);
        system("pause");
        return 1;
    }

    // Find the Intel AX200 interface
    for (int i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
        pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];
        if (wcsstr(pIfInfo->strInterfaceDescription, L"Intel(R) Wi-Fi 6 AX200") != NULL) {
            targetInterfaceGuid = pIfInfo->InterfaceGuid;
            interfaceFound = true;
            printf("[%s] Found target interface: %ls\n", __TIME__, pIfInfo->strInterfaceDescription);
            break;
        }
    }

    if (!interfaceFound) {
        printf("Error: Could not find Intel AX200 interface\n");
        if (pIfList != NULL) WlanFreeMemory(pIfList);
        WlanCloseHandle(hClient, NULL);
        system("pause");
        return 1;
    }

    // Main monitoring loop
    while (1) {
        checkCount++;
        
        // Free the previous list if it exists
        if (pIfList != NULL) {
            WlanFreeMemory(pIfList);
            pIfList = NULL;
        }

        // Get fresh interface list
        dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
        if (dwResult != ERROR_SUCCESS) {
            printf("[%s] Error: WlanEnumInterfaces failed with error: %lu\n", __TIME__, dwResult);
            Sleep(5000);
            continue;
        }

        // Clear console and show header every 10 checks
        if (checkCount % 10 == 0) {
            system("cls");
            printf("Wi-Fi Interface Monitor Running (Administrator Mode)...\n");
            printf("Monitoring: Intel(R) Wi-Fi 6 AX200\n");
            printf("Check count: %d\n", checkCount);
            printf("Press Ctrl+C to exit\n\n");
        }

        // Find our target interface in the fresh list
        for (int i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
            pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];
            if (IsEqualGUID(&pIfInfo->InterfaceGuid, &targetInterfaceGuid)) {
                // Check if interface state has changed to disconnected
                if (lastState != pIfInfo->isState) {
                    printf("[%s] State changed from %s to %s\n", 
                           __TIME__, 
                           getStateString(lastState), 
                           getStateString(pIfInfo->isState));
                    
                    if (pIfInfo->isState == wlan_interface_state_disconnected ||
                        pIfInfo->isState == wlan_interface_state_not_ready) {
                        needsRestart = true;
                    }
                }

                if (needsRestart) {
                    restartInterface(hClient, pIfInfo);
                    needsRestart = false;
                }

                lastState = pIfInfo->isState;
                break;
            }
        }

        // Wait between checks
        Sleep(5000);
    }

    // Cleanup (this part won't be reached unless you add a break condition)
    if (pIfList != NULL) {
        WlanFreeMemory(pIfList);
    }
    WlanCloseHandle(hClient, NULL);
    return 0;
}
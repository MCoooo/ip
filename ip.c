#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

// ANSI escape codes for color
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

int main() {
    WSADATA wsaData;
    char hostname[256];
    struct addrinfo hints, *res, *ptr;
    char ipstr[INET_ADDRSTRLEN];

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf(RED "WSAStartup failed.\n" RESET);
        return 1;
    }

    // Get the local hostname
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        printf(RED "gethostname failed.\n" RESET);
        WSACleanup();
        return 1;
    }
    printf(CYAN "Hostname: " RESET "%s\n", hostname);

    // Set up the hints address info structure
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    // Get the list of IP addresses associated with the hostname
    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        printf(RED "getaddrinfo failed.\n" RESET);
        WSACleanup();
        return 1;
    }

    // Loop through the results and print each IP address
    printf(CYAN "IP Addresses:\n" RESET);
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
        struct sockaddr_in *sockaddr_ipv4 = (struct sockaddr_in *) ptr->ai_addr;
        inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ipstr, sizeof(ipstr));
        printf(GREEN "  %s\n" RESET, ipstr);
    }

    // Clean up
    freeaddrinfo(res);

    // Get a list of all adapters
    PIP_ADAPTER_INFO adapterInfo;
    DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);
    adapterInfo = (IP_ADAPTER_INFO *) malloc(dwBufLen);

    if (GetAdaptersInfo(adapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW) {
        adapterInfo = (IP_ADAPTER_INFO *) malloc(dwBufLen);
    }

    if (GetAdaptersInfo(adapterInfo, &dwBufLen) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapterInfo = adapterInfo;
        while (pAdapterInfo) {
            printf(CYAN "\nAdapter Name: " RESET "%s\n", pAdapterInfo->AdapterName);
            printf(CYAN "  Description: " RESET "%s\n", pAdapterInfo->Description);
            printf(CYAN "  IP Address: " RESET "%s\n", pAdapterInfo->IpAddressList.IpAddress.String);
            printf(CYAN "  IP Mask: " RESET "%s\n", pAdapterInfo->IpAddressList.IpMask.String);
            printf(CYAN "  Gateway: " RESET "%s\n", pAdapterInfo->GatewayList.IpAddress.String);
            pAdapterInfo = pAdapterInfo->Next;
        }
    } else {
        printf(RED "GetAdaptersInfo failed.\n" RESET);
    }

    free(adapterInfo);
    WSACleanup();

    return 0;
}


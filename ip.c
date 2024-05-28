#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct {
    bool printHostname;
    bool printActiveIP;
    bool printIfPartial;
    bool printIfFull;
    bool incInactiveAdapaters;
} Config;

int get_local_hostName(char *hostname, size_t size ) {
    // char hostname[256];

    if (gethostname(hostname, size) == SOCKET_ERROR) {
        return SOCKET_ERROR;
    }
    return 0;
}

void print_ip_addresses(char *hostname) {
    struct addrinfo hints, *res, *ptr;
    char ipstr[INET_ADDRSTRLEN];

        // Set up the hints address info structure
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    // Get the list of IP addresses associated with the hostname
    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        printf(RED "getaddrinfo failed.\n" RESET);
        WSACleanup();
        return ;
    }

    printf(CYAN "IP Addresses:\n" RESET);
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
        struct sockaddr_in *sockaddr_ipv4 = (struct sockaddr_in *) ptr->ai_addr;
        inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ipstr, sizeof(ipstr));
        printf("  %s\n" RESET, ipstr);
    }
    printf("\n");

    freeaddrinfo(res);
}

void print_adapters_info(bool all, bool full) {
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    ULONG ulOutBufLen = 0;
    DWORD dwRetVal = 0;

    // Make an initial call to GetAdaptersInfo to get the necessary buffer size
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);
    }

    // Call GetAdaptersInfo to retrieve adapter information
    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter) {
            if (all || pAdapter->IpAddressList.IpAddress.String[0] != '0') {
                printf(CYAN "%s:\n" RESET, pAdapter->Description);
                if (pAdapter->IpAddressList.IpAddress.String[0] != '0') {
                    printf(CYAN "  IP Address: " RESET "%s\n", pAdapter->IpAddressList.IpAddress.String);
                    if (full) {
                        printf(CYAN "  Subnet Mask: " RESET "%s\n", pAdapter->IpAddressList.IpMask.String);
                        printf(CYAN "  Gateway: " RESET "%s\n", pAdapter->GatewayList.IpAddress.String);
                        printf(CYAN "  DHCP Server: " RESET "%s\n", pAdapter->DhcpServer.IpAddress.String);
                        printf(CYAN "  Lease Obtained: " RESET "%s", ctime((time_t*)&pAdapter->LeaseObtained));
                        printf(CYAN "  Lease Expires: " RESET "%s", ctime((time_t*)&pAdapter->LeaseExpires));
                        printf(CYAN "  Adapter Name: " RESET "%s\n", pAdapter->AdapterName);
                    }
                }
                printf("\n");
            }
            pAdapter = pAdapter->Next;
        }
    } else {
        printf(RED "GetAdaptersInfo failed.\n" RESET);
    }

    if (pAdapterInfo != NULL) {
        free(pAdapterInfo);
    }
}

int main(int argc, char *argv[]) {

    Config config = {false, false, false, false, false};

    // Parse command-line arguments and update config
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--name") == 0) {
            config.printHostname = true;
        }
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--active") == 0) {
            config.printActiveIP = true;
        }
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--if") == 0) {
            config.printIfPartial = true;
        }
        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--full") == 0) {
            config.printIfFull = true;
        }
        if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--inactive") == 0) {
            config.incInactiveAdapaters = true;
        }
    }
    if (config.printIfPartial == false && config.printActiveIP == false && config.printHostname == false) {
        config.printIfPartial = true;
    }

    printf("\n");

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf(RED "WSAStartup failed.\n" RESET);
        return 1;
    }

    char hostname[256];
    if (config.printHostname || config.printActiveIP){
        if (get_local_hostName(hostname, sizeof(hostname)) !=0){
            printf(RED "Failed to get hostname.\n" RESET);
            return 1;
        }
    }

    if (config.printHostname){
        printf(CYAN "Hostname: " RESET "\n  %s\n\n", hostname);
    }
   
    if (config.printActiveIP){
        print_ip_addresses(hostname);
    }

    if (config.printIfPartial){
        print_adapters_info(config.incInactiveAdapaters, config.printIfFull);
    }

    WSACleanup();

    return 0;
}

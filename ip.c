// #include <iptypes.h>
#include <stdio.h>
#include <time.h>
#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <dhcpsapi.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Dhcpcsvc.lib")

#define PROGRAM_VERSION "1.0.0"

// ANSI escape codes for color
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

#define MEGABITS_PER_SECOND 1000000

typedef struct {
    bool printHostname;
    bool printActiveIP;
    bool printIfPartial;
    bool printIfFull;
    bool incInactiveAdapaters;
} Config;

int get_local_hostName(char *hostname, size_t size ) {
    if (gethostname(hostname, size) == SOCKET_ERROR) {
        return SOCKET_ERROR;
    }
    return 0;
}

void print_ip_addresses(char *hostname) {
    struct addrinfo hints, *res, *ptr;
    char ipstr[INET_ADDRSTRLEN];

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

const char* getInterfaceTypeDescription(UINT ifType) {
    switch (ifType) {
        case IF_TYPE_OTHER:
            return "Other";
        case IF_TYPE_ETHERNET_CSMACD:
            return "Ethernet";
        case IF_TYPE_ISO88025_TOKENRING:
            return "Token Ring";
        case IF_TYPE_IEEE80211:
            return "Wireless";
        case IF_TYPE_PPP:
            return "Point-to-Point Protocol";
        // Add more cases as needed
        default:
            return "Unknown";
    }
}



void print_adapters_addresses(bool all, bool full) {
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;

    ULONG outBufLen = 0;
    DWORD dwRetVal = 0;

    // Make an initial call to GetAdaptersAddresses to get the necessary buffer size
    dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, NULL, pAddresses, &outBufLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
        dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, NULL, pAddresses, &outBufLen);
    }


    if (dwRetVal == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses; pCurrAddresses != NULL; pCurrAddresses = pCurrAddresses->Next) {

             if (pCurrAddresses->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
                 continue;
             }

            if(all || pCurrAddresses->OperStatus == 1){
                printf("\n");                
                printf (WHITE "///////////////////////////////////////////////////////////////////\n" RESET);
                // printf (WHITE "-------------------------------------------------------------------\n" RESET);
                printf(CYAN "  Adapter: " RESET "%ld %S (%S):\n", pCurrAddresses->IfIndex, pCurrAddresses->FriendlyName, pCurrAddresses->Description);
                printf (GREEN "---------------------------------------" RESET "\n");

                // printf (WHITE "-------------------------------------------------------------------\n" RESET);
                // Ip address
                char ipAddr[INET_ADDRSTRLEN];
                struct sockaddr_in* ipv4 = (struct sockaddr_in*)pCurrAddresses->FirstUnicastAddress->Address.lpSockaddr;
                inet_ntop(AF_INET, &(ipv4->sin_addr), ipAddr, INET_ADDRSTRLEN);
                printf(CYAN "  IP Address: " RESET "%s\n", ipAddr);
                

                if (full){

                    if (pCurrAddresses->FirstDnsServerAddress != NULL){
                    printf (GREEN "---------------------------------------" RESET "\n");
                    
                        // DNS servers
                        printf(CYAN "  DNS Servers:\n" RESET);
                        for (PIP_ADAPTER_DNS_SERVER_ADDRESS dnsServer = pCurrAddresses->FirstDnsServerAddress; dnsServer != NULL; dnsServer = dnsServer->Next) {
                            char dnsIpAddr[INET6_ADDRSTRLEN];
                            struct sockaddr* dnsSockAddr = dnsServer->Address.lpSockaddr;
                            if (dnsSockAddr->sa_family == AF_INET) {
                                struct sockaddr_in* dnsIpv4 = (struct sockaddr_in*)dnsSockAddr;
                                inet_ntop(AF_INET, &(dnsIpv4->sin_addr), dnsIpAddr, INET6_ADDRSTRLEN);
                                printf("    %s\n", dnsIpAddr);
                            } 
                        }
                    }

                    // DHCP server
                    char dhcpIpAddr[INET_ADDRSTRLEN];
                    if ( pCurrAddresses->Dhcpv4Server.lpSockaddr != NULL ) {
                        struct sockaddr_in* dhcpIpv4 = (struct sockaddr_in*)pCurrAddresses->Dhcpv4Server.lpSockaddr;
                        inet_ntop(AF_INET, &(dhcpIpv4->sin_addr), dhcpIpAddr, INET_ADDRSTRLEN);
                        printf(CYAN "  DHCP Server: " RESET "%s\n", dhcpIpAddr);
                    }

                    // Gateway adrress
                    char gIpAddr[INET_ADDRSTRLEN];
                    if (pCurrAddresses->FirstGatewayAddress != NULL) {
                        struct sockaddr_in* gIpv4 = (struct sockaddr_in*)pCurrAddresses->FirstGatewayAddress->Address.lpSockaddr;
                        inet_ntop(AF_INET, &(gIpv4->sin_addr), gIpAddr, INET_ADDRSTRLEN);
                        printf(CYAN "  Gateway: " RESET "%s\n", gIpAddr);
                    }

                    if (pCurrAddresses->DnsSuffix != NULL) {
                        printf (GREEN "---------------------------------------" RESET "\n");
                        // DNS Suffix
                        printf(CYAN "  DNS Suffix: " RESET "%S\n", pCurrAddresses->DnsSuffix);
                        

                        if (pCurrAddresses->FirstDnsSuffix != NULL) {
                            printf(CYAN "  DNS Suffixes:\n" RESET);
                            PIP_ADAPTER_DNS_SUFFIX dnsSuffix = pCurrAddresses->FirstDnsSuffix;
                            while (dnsSuffix != NULL) {
                                printf("    %S\n", dnsSuffix->String);
                                dnsSuffix = dnsSuffix->Next;
                            }
                        }
                    }

                    printf (GREEN "---------------------------------------" RESET "\n");

                    // Physical address
                    printf(CYAN "  Physical Address: " RESET);
                    for (UINT i = 0; i < pCurrAddresses->PhysicalAddressLength; i++) {
                        if (i == (pCurrAddresses->PhysicalAddressLength - 1)) {
                            printf("%.2X\n", (int)pCurrAddresses->PhysicalAddress[i]);
                        } else {
                            printf("%.2X-", (int)pCurrAddresses->PhysicalAddress[i]);
                        }
                    }
                    printf(CYAN "  Interface Type: " RESET "%s\n", getInterfaceTypeDescription(pCurrAddresses->IfType));

                    int transmitSpeedMbps = (double)pCurrAddresses->TransmitLinkSpeed / MEGABITS_PER_SECOND;
                    int receiveSpeedMbps = (double)pCurrAddresses->ReceiveLinkSpeed / MEGABITS_PER_SECOND;

                    if (transmitSpeedMbps > 0 && receiveSpeedMbps > 0) {
                        printf (GREEN "---------------------------------------" RESET "\n");
                        printf(CYAN "  Transmit Link Speed: " RESET "%d Mbps\n", transmitSpeedMbps);
                        printf(CYAN "  Receive Link Speed: " RESET "%d Mbps\n", receiveSpeedMbps);
                    }



                }
                if (all) {
                    if (pCurrAddresses->OperStatus == 1) {
                        printf(CYAN "  Status: Up\n" RESET);
                    } else {
                        printf(CYAN "  Status: Down\n" RESET);
                    }

                }
                printf (WHITE "///////////////////////////////////////////////////////////////////\n" RESET);

            }

        }
    } else {
       printf(RED "GetAdaptersAddresses failed.\n" RESET);
    }

    if (pAddresses != NULL) {
        free(pAddresses);
    }


}


void displayHelp() {
    printf("\n");
    printf("Usage: ip [options]\n");
    printf("Options:\n");
    printf("  -n, --name     : Print hostname\n");
    printf("  -a, --active   : Print active IP addresses\n");
    printf("  -i, --if       : Print partial interface information\n");
    printf("  -f, --full     : Print full interface information\n");
    printf("  -x, --inactive : Include inactive adapters\n");
    printf("  -v, --version  : Get version information\n");
    printf("\n");
    printf("NB: Defaults to --if if not selection made");
    printf("\n");
}

void display_version() {
    printf("\n");
    printf(" Welcome to ip v%s\n", PROGRAM_VERSION);
    printf("\n");

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
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "/?") == 0 || strcmp(argv[i], "-?") == 0) {
            displayHelp();
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            display_version();
            return 0;
        }
    }

    // Make partial if default
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
        // print_adapters_info(config.incInactiveAdapaters, config.printIfFull);
        print_adapters_addresses(config.incInactiveAdapaters, config.printIfFull);
    }

    WSACleanup();

    return 0;
}

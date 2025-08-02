#include <iostream>
#include <winsock2.h>
//access to the Windows Sockets API—specifically version 2, which is the modern standard
#include <ws2tcpip.h>
#include <windows.h>
#include <csignal>
// for signal handling
#pragma comment(lib, "ws2_32.lib")
/*link your program with the ws2_32.lib library which contains all the networking
functions like socket(), bind(), recv(), WSAStartup(), etc.
 Without linking it, your program would compile but fail to link, because the linker wouldn’t
 know where to find the actual implementations of those functions.
*/
// IP header structure
struct iphdr {
    unsigned char ihl : 4; //Needed to calculate where the IP header ends and where the payload (e.g., TCP/UDP) begins
    unsigned char version : 4; //To confirm it's IPv4 (usually always 4)
    unsigned char tos;
    unsigned short tot_len;
    unsigned short id;
    unsigned short frag_off;
    unsigned char ttl;
    unsigned char protocol; //tells you if it’s TCP, UDP, ICMP, etc
    unsigned short check;
    unsigned int saddr; //Source IP — very useful for identifying sender
    unsigned int daddr; //Destination IP — very useful for identifying receiver
};

bool running = true;
void signalHandler(int signum) {
    running = false;
}

int main() {
    WSADATA wsaData; //stores info about the WinSock version.
    SOCKET sock; //the raw socket we'll use to capture packets.
    char buffer[65536]; //where incoming packet data will be stored. Large enough to hold most packets
    int bytesReceived = 0; //how many bytes we got from each packet.

    // Handle Ctrl+C gracefully
    signal(SIGINT, signalHandler);

    // Step 1: Start WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "WinSock initialization failed!" << std::endl;
        return 1;
    }
    // If it fails, clean up and exit.


    // Step 2: Create raw socket
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
    //AF_INET: IPv4, SOCK_RAW: raw packets, IPPROTO_IP: capture all IP packets.

    if (sock == INVALID_SOCKET) {
        std::cout << "Socket creation failed!" << std::endl;
        WSACleanup();
        return 1;
    }
    //If it fails, clean up and exit.

    // Step 3: Bind to your IP (change this to your actual IP)
    sockaddr_in local;//structure to describe where the socket should listen.
    local.sin_family = AF_INET;//We’re using IPv4 addresses
    local.sin_port = htons(0); // 0 means "any port", sin_port is the port number.

    char hostname[256];
    gethostname(hostname, sizeof(hostname));//Gets your computer’s hostname (e.g., "MyLaptop").
    hostent* host = gethostbyname(hostname);//Resolves that hostname to an IP address using gethostbyname.
    if (!host) {
        std::cout << "Failed to get local IP!" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }//f it fails, it cleans up and exits.

    // Assign resolved IP address to local.sin_addr
    in_addr addr;//create a temporary in_addr variable called addr.
    memcpy(&addr, host->h_addr_list[0], sizeof(in_addr));// Copy the IP bytes into addr.
    local.sin_addr = addr;// Then assign addr to local.sin_addr.


    //Binds the socket to your local IP and a random port.
    if (bind(sock, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
        std::cout << "Bind failed!" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }//If it fails, it cleans up and exits

    std::cout << "Bound to IP: " << inet_ntoa(local.sin_addr) << std::endl;
    //Prints the IP address you just bound to in human-readable form (e.g., "192.168.1.5").

    // Step 4: Enable promiscuous mode- it lets you capture all packets, not just those meant for your computer.

    DWORD flag = 1; //- This sets the flag to 1, which means "turn on promiscuous mode".
    if (ioctlsocket(sock, SIO_RCVALL, &flag) != 0) {
        std::cout << "Failed to enable promiscuous mode!" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    /* ioctlsocket(sock, SIO_RCVALL, &flag) is the key function. It tells Windows:
"Hey, for this socket (sock), I want to receive all packets on the network interface — not just the ones meant for me."
  . sock is your raw socket
  .SIO_RCVALL is a special control code that activates promiscuous mode
  . &flag is a pointer to the flag (1 to enable, 0 to disable)
 If ioctlsocket returns anything other than 0, it means failure.
- Inside the if block:
If enabling promiscuous mode fails:
  . Print an error
  . Close the socket
  . Clean up Winsock
  . Exit with error code 1
*/
    // Step 5: Receive packets
    std::cout << "Listening for packets...(press Ctrl+C to stop)" << std::endl;

    // Infinite loop: continuously capture packets
    while (running) {
        // Receive raw packet data into buffer
        bytesReceived = recv(sock, buffer, sizeof(buffer), 0);

        // If data was received successfully
        if (bytesReceived > 0) {
            std::cout << "Packet received: " << bytesReceived << " bytes" << std::endl;

            /*
             * Skip the Ethernet header (first 14 bytes).
             * Ethernet headers contain MAC addresses and other link-layer info,
             * but we're interested in the IP layer, which starts after that.
             */
            iphdr* ip = (iphdr*)buffer; // 



            /*
             * Read the protocol field from the IP header.
             * This tells us what kind of transport-layer data the packet contains:
             * - 1 = ICMP (used for ping and diagnostics)
             * - 6 = TCP (used for reliable connections like HTTP, FTP)
             * - 17 = UDP (used for fast, connectionless services like DNS, streaming)
             */
            std::cout << "Protocol: ";
            switch (ip->protocol) {
                case 1:
                    std::cout << "ICMP" << std::endl;
                    break;
                case 6:
                    std::cout << "TCP" << std::endl;
                    break;
                case 17:
                    std::cout << "UDP" << std::endl;
                    break;
                default:
                    // If it's not one of the common protocols, print its numeric value
                    std::cout << "Other (" << (int)ip->protocol << ")" << std::endl;
                    break;
            }
            
         //Outputing the source nad destination IP
         in_addr src, dest;
            src.s_addr = ip->saddr;
            dest.s_addr = ip->daddr;
            std::cout << "From: " << inet_ntoa(src) << ", To: " << inet_ntoa(dest) << std::endl;

            // add a separator for readability
            std::cout << "-----------------------------" << std::endl;
        }
    }
    // Step 6: Cleanup
    std::cout << "Stopping packet capture and cleaning up." << std::endl;
    closesocket(sock);
    WSACleanup();
    return 0;

}

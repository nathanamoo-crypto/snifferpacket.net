#include <iostream>
#include <winsock2.h>

using namespace std;

int main() {
    // Step 1: Start Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Winsock startup failed!" << endl;
        return 1;
    }

    // Step 2: Create a raw socket
    SOCKET rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
    if (rawSocket == INVALID_SOCKET) {
        cout << "Socket creation failed!" << endl;
        WSACleanup();
        return 1;
    }

    // Step 3: Get local host name
    char host[256];
    if (gethostname(host, sizeof(host)) == SOCKET_ERROR) {
        cout << "Failed to get host name!" << endl;
        closesocket(rawSocket);
        WSACleanup();
        return 1;
    }

    // Step 4: Get IP address of the host
    hostent* host_entry = gethostbyname(host);
    if (!host_entry) {
        cout << "Failed to get IP address!" << endl;
        closesocket(rawSocket);
        WSACleanup();
        return 1;
    }

    // Prepare socket address
    sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(0); // Any port
    local.sin_addr.s_addr = *((u_long*)host_entry->h_addr);

    // Step 5: Bind socket to IP
    if (bind(rawSocket, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
        cout << "Bind failed!" << endl;
        closesocket(rawSocket);
        WSACleanup();
        return 1;
    }

    // Step 6: Enable promiscuous mode to capture all packets
    u_long flag = 1;
    if (ioctlsocket(rawSocket, SIO_RCVALL , &flag) != 0) {
        cout << "Failed to set promiscuous mode!" << endl;
        closesocket(rawSocket);
        WSACleanup();
        return 1;
    }

    cout << "Listening for packets on " << inet_ntoa(local.sin_addr) << "..." << endl;

    // Step 7: Receive packets
    char buffer[65536];
    while (true) {
        int bytesReceived = recv(rawSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            cout << "Packet received: " << bytesReceived << " bytes" << endl;
        } else {
            cout << "Error receiving packet" << endl;
            break;
        }
    }

    // Step 8: Cleanup
    closesocket(rawSocket);
    WSACleanup();
    return 0;
}

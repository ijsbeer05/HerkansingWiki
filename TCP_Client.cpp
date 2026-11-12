#include <arpa/inet.h> 
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> 
#include <chrono> 
#include <vector>
#include <iostream>

#define PORT     8080
#define MAXLINE  65536
#define SA struct sockaddr
#define AANTAL_METINGEN 200 

//meet de RTT voor een specifieke pakketgrootte
long long meet_rtt(int sockfd, size_t pakket_grootte) {
    std::vector<char> send_buffer(pakket_grootte, 'B');
    std::vector<char> recv_buffer(pakket_grootte);

    ssize_t bytes_sent, bytes_received;

    // start timer voor de RTT
    auto start = std::chrono::high_resolution_clock::now();

    // pakket versturen (pakket komt gegarandeerd aan)
    size_t total_sent = 0;
    while (total_sent < pakket_grootte) {
        bytes_sent = send(sockfd, send_buffer.data() + total_sent, pakket_grootte - total_sent, 0);
        if (bytes_sent <= 0) { std::cerr << "TCP send fout." << std::endl; return -1; }
        total_sent += bytes_sent;
    }

    // antwoord van de server wachten (pakket wordt gegarandeerd ontvangen)
    size_t total_received = 0;
    while (total_received < pakket_grootte) {
        bytes_received = recv(sockfd, recv_buffer.data() + total_received, pakket_grootte - total_received, 0);

        if (bytes_received <= 0) { std::cerr << "TCP ontvangst fout of server verbrak verbinding." << std::endl; return -1; }
        total_received += bytes_received;
    }

    // stop timer en bereken RTT
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count();
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    // Array van pakketgroottes die getest worden
    std::vector<size_t> test_groottes = {
        64, 256, 1024, 4096, 8192, 16384, 32768, 65000
    };

    std::cout << "-- TCP -- RTT metingen (" << AANTAL_METINGEN << " metingen per grootte) --" << std::endl;

    // fro loop gaat door alle pakketgroottes in de array test_groottes heen
    for (size_t grootte : test_groottes) {

        // socket aanmaken
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { perror("Socket creation failed"); continue; }
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        servaddr.sin_port = htons(PORT);

        // verbinding maken (Three-way handshake)
        if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
            std::cerr << "Fout: Connection failed for size " << grootte << ". Server not running?" << std::endl;
            close(sockfd);
            continue;
        }

        long long totale_rtt = 0;
        int mislukte_metingen = 0;

        // meting AANTAL_METINGEN keer uitvoeren
        for (int i = 0; i < AANTAL_METINGEN; ++i) {
            long long rtt = meet_rtt(sockfd, grootte);
            if (rtt > 0) { totale_rtt += rtt; }
            else { mislukte_metingen++; }
        }

        // toont het resultaat
        if ((AANTAL_METINGEN - mislukte_metingen) > 0) {
            long long gemiddelde_rtt = totale_rtt / (AANTAL_METINGEN - mislukte_metingen);

            std::cout << "Pakketgrootte: " << grootte << " bytes. Gemiddelde RTT: "
                << gemiddelde_rtt << " microseconden. (" << mislukte_metingen << " mislukt)" << std::endl;
        }
        else {
            std::cerr << "WAARSCHUWING: Geen geldige metingen voor grootte: " << grootte << " bytes." << std::endl;
        }

        // verbinding na de meting sluiten(Four-way handshake)
        close(sockfd);
    }

    return 0;
}

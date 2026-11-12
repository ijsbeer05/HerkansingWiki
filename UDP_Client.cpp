#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono> 
#include <vector>
#include <numeric>
#include <algorithm> 

//8081 gebruikt om ervoor te zorgen dat het direct werkt en niet eerst moet wachten op afsluiten 8080 van TCP als je snel wilt testen
#define PORT     8081 
#define MAXLINE  65536 
#define AANTAL_METINGEN 200 // Aantal metingen per pakketgrootte

// teller om later te kijken of alle pakketten zijn aangekomen
static unsigned int packet_counter = 0;

// meet rtt per pakketgrootte
long long meet_rtt(int sockfd, struct sockaddr_in servaddr, size_t data_grootte) {
    
    // totale grootte = 4 bytes (voor ID) + data_grootte
    size_t totale_grootte = data_grootte + sizeof(unsigned int);

    // zorg ervoor dat de totale grootte de MAXLINE niet overschrijdt
    if (totale_grootte > MAXLINE) {
        std::cerr << "Fout: Totale pakketgrootte overschrijdt MAXLINE." << std::endl;
        return -3;
    }

    // maakt een buffer van de vereiste totale grootte
    std::vector<char> send_buffer(totale_grootte);
    char recv_buffer[MAXLINE];
    socklen_t len = sizeof(servaddr);
    ssize_t n;

    // id van pakket instellen
    packet_counter++;
    unsigned int expected_id = packet_counter;

    // kopieer het id naar het begin van de buffer (de eerste 4 bytes)
    std::memcpy(send_buffer.data(), &expected_id, sizeof(unsigned int));
    // Vul de rest van de buffer met willekeurige data (hier de 'A')
    std::fill(send_buffer.data() + sizeof(unsigned int), send_buffer.data() + totale_grootte, 'A');

    // start timer voor rtt meten
    auto start = std::chrono::high_resolution_clock::now();

    // stuurt het pakket
    sendto(sockfd, send_buffer.data(), totale_grootte,
        MSG_CONFIRM, (const struct sockaddr*)&servaddr,
        sizeof(servaddr));

    // wacht op antwoord van de server
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000; // 500ms timeout als er geen reactie is
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    n = recvfrom(sockfd, recv_buffer, MAXLINE,
        MSG_WAITALL, (struct sockaddr*)&servaddr,
        &len);

    // stop timer voor rtt meten
    auto end = std::chrono::high_resolution_clock::now();

    // controleert op fouten, timeout of verlies
    if (n != totale_grootte || n <= 0) {
        // bericht niet volledig ontvangen of fout opgetreden (incl. timeout)
        return -1; // retourneer foutwaarde
    }

    // kijk of equentie id klopt
    unsigned int received_id;
    std::memcpy(&received_id, recv_buffer, sizeof(unsigned int));

    if (received_id != expected_id) {
        // als er wel iets is ontvangen, maar eht is niet wat er verwacht werd, return -2.
        return -2;
    }

    // rtt is geldig (geen gekke dingen erin)
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count();
}


int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    // array van pakketgroottes die getest worden
    std::vector<size_t> test_groottes = {
        64, 256, 1024, 4096, 8192, 16384, 32768, 65000
    };

    // maak UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    std::memset(&servaddr, 0, sizeof(servaddr));

    // server informatie invullen
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    std::cout << "-- UPD (" << AANTAL_METINGEN << " metingen per grootte) --" << std::endl;

    // for loop gaat door alle gedefinieerde pakketgroottes in de array test_groottes
    for (size_t data_grootte : test_groottes) {
        std::vector<long long> rtt_resultaten;
        long long totale_rtt = 0;
        int mislukte_metingen = 0;

        // meting worden AANTAL_METINGEN keer uitgevoerd 
        for (int i = 0; i < AANTAL_METINGEN; ++i) {
            long long rtt = meet_rtt(sockfd, servaddr, data_grootte);

            if (rtt > 0) { // Alleen geldige metingen deon mee (rtt is positief)
                rtt_resultaten.push_back(rtt);
                totale_rtt += rtt;
            }
            else {
                mislukte_metingen++;
            }
        }

        if (!rtt_resultaten.empty()) {
            // het gemiddelde berekenen van de rtt per pakktrgrootte
            long long gemiddelde_rtt = totale_rtt / rtt_resultaten.size();

            std::cout << "Pakketgrootte: " << data_grootte << " bytes. Gemiddelde RTT: "
                << gemiddelde_rtt << " microseconden. (" << mislukte_metingen << " verlies)" << std::endl;
        }
        else {
            std::cerr << "WAARSCHUWING: Geen geldige metingen voor grootte: " << data_grootte << " bytes." << std::endl;
        }
    }

    // sluit socket
    close(sockfd);
    return 0;
}

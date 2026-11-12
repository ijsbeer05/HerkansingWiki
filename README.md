# RTT-metingen TCP vs. UDP

Dit project bevat de C++ prototype-code voor het meten en vergelijken van de reactietijd (round-trip time, RTT) tussen **TCP** en **UDP** bij communicatie tussen robots. De metingen worden uitgevoerd in microseconden over 8 verschillende pakketgroottes (van 64 tot 65.000 bytes), met 200 metingen per pakketgrootte.

## Bestanden in deze repository

| Bestand | Protocol | Poort | Beschrijving |
| :--- | :--- | :--- | :--- |
| `tcp_server.cpp` | TCP | `8080` | Ontvangt TCP-pakketten en stuurt deze direct terug (echo). |
| `tcp_client.cpp` | TCP | `8080` | Verstuurt 200 pakketten per grootte en berekent de gemiddelde RTT in microseconden. |
| `udp_server.cpp` | UDP | `8081` | Ontvangt UDP-datagrammen en stuurt deze direct terug naar de client. |
| `udp_client.cpp` | UDP | `8081` | Verstuurt 200 pakketten (met pakket-ID) per grootte, meet de RTT en telt pakketverlies. |

## Testinstellingen

- **Pakketgroottes:** `64`, `256`, `1024`, `4096`, `8192`, `16384`, `32768` en `65000` bytes
- **Aantal metingen:** `200` per pakketgrootte
- **Tijdseenheid:** Microseconden (`µs`)

---

## Stappenplan: Code uitvoeren in Docker

De code wordt uitgevoerd binnen een Docker-container via Windows PowerShell of Command Prompt.

### 1. Docker opstarten en container openen
Start eerst de **Docker Desktop** applicatie op je computer. Open daarna PowerShell of Command Prompt en ga naar de map met de code in de container:

```bash
docker exec -it determined_lederberg /bin/bash
cd /tmp/build
```

*(Optioneel)* Bestanden in de container bekijken of nieuwe code erin plakken (sluit plakken af met `Ctrl + D`):
```bash
cat tcp_client.cpp        # Bestand bekijken
cat > tcp_client.cpp      # Bestand aanmaken of overschrijven
```

### 2. Code compileren
Compileer alle vier de C++-bestanden met `g++` (doe dit opnieuw na elke aanpassing in de code):

```bash
g++ tcp_client.cpp -o tcp_client
g++ tcp_server.cpp -o tcp_server
g++ udp_client.cpp -o udp_client
g++ udp_server.cpp -o udp_server
```

### 3. Metingen uitvoeren
Start eerst de server op de achtergrond (`&`) en voer daarna de bijbehorende client uit:

**TCP-test:**
```bash
./tcp_server &
./tcp_client
```

**UDP-test:**
```bash
./udp_server &
./udp_client
```

### 4. Achtergrondservers stoppen
Omdat de servers met `&` op de achtergrond draaien, kun je ze na het testen weer afsluiten met:

```bash
pkill tcp_server
pkill udp_server
```

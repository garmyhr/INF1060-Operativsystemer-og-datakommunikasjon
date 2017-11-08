README.txt

Hjemmeeksamen INF1060, H2016

Oppgavetekst:
https://github.uio.no/INF1060/Hjemmeeksamen/blob/master/Hjemmeeksamen.pdf

NB! ANTAKELSER:
Denne serveren tar bare imot én klient
og avslutter naar denne klienten er terminert.
Antar at dette er greit da det ikke staar
spesifisert i oppgaveteksten.


MAKEFILE:
make server - kompilerer server.c
make client - kompilerer client.c
make runs - kjør server.c
make runc - kjør client.c
checks - kjør server.c med valgrind
checkc - kjør client.c med valgrind


INPUT I MAKEFILE:
4444 for portnummer
127.0.0.1 for IP-adresse
prog.job for jobbfil

Kjoer med annen input:
./server <Jobbfil> <Portnummer>
./client <IP/Maskinnavn> <Portnummer>


  KILDER - KODE LAANT/MODIFISERT OG BRUKT FRA NETT:
    1) I child 1 og child 2:
    Lese fra read-end av pipe, printe ut til stderr og stdout:
    http://stackoverflow.com/questions/15196784/how-to-wait-till-data-is-written-on-the-other-end-of-pipe

    2) I opprett_socket i klient og lytt_tilkobling + opprett_socket i server:
    Opprettelse av sockets og kommunikasjon server og klient:
    https://screencast.uninett.no/relay/ansatt/hpkragseuio.no/2016/01.11/6678800/Plenumstime_10_-_20161101_164617_39.html

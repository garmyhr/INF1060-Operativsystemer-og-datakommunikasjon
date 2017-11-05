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


SVAKHETER VED OPPGAVEN, IKKE RUKKET TO DO-LIST:
  1) Programmet sender én og én jobbmelding. Rakk ikke se på en bedre
  loesning av dette, men kan se at det burde vært høyere prioritert.

  2) Svarer man med bokstaver når man skal angi hvor mange jobber man
  vil lese inn i 2) hent flere (X antall) jobber, sendes jobber likevel.
  Har ikke rukket å fikse dette.

  3) Prøvde meg på ctrl + c for suksessfull terminering med signal handler.
  (void intHandler(int temp){...} signal(SIGINT, intHandler) osv.)
  Klarte ikke finne ut av hvor det skulle implementeres for aa fungere hele tiden.

  4) Om serveren terminerer før klienten, sier den ikke ifra om dette til klient.

  5) Lagde funksjonen error som automatisk skriver ut perror. Ser at dette
  kan være unødvendig å implementere inne i funksjonen som jeg også bruker steder
  der errno ikke blir satt, og "success" blir dermed printet ut ved enkelte feiltermineringer.

Obs! Maskinnavn i stedet for IP-adresse i argv[1] til server skal fungere,
men fikk ikke testet dette.


  KILDER - KODE LAANT/MODIFISERT OG BRUKT FRA NETT:
    1) I child 1 og child 2:
    Lese fra read-end av pipe, printe ut til stderr og stdout:
    http://stackoverflow.com/questions/15196784/how-to-wait-till-data-is-written-on-the-other-end-of-pipe

    2) I opprett_socket i klient og lytt_tilkobling + opprett_socket i server:
    Opprettelse av sockets og kommunikasjon server og klient:
    https://screencast.uninett.no/relay/ansatt/hpkragseuio.no/2016/01.11/6678800/Plenumstime_10_-_20161101_164617_39.html

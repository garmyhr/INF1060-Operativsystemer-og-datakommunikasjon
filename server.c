//Hjemmeeksamen INF1060, H2016

/*
Info om filer og makefile:
Les README.txt
*/

//Socket prog
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

//Handler, signal
#include <signal.h>

//Pipe og fork
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

//Div, generelt
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// Deklarerer funksjoner
int opprett_socket(int portnr);
int lytt_tilkobling(int request_socket);
int send_jobb();
int avslutt();

// Deklarerer hjelpefunksjoner
void error(char *msg);

/*Typedefinerer int som bool slik at
lesing av "boolean"-verdier blir enklere*/
typedef int bool;
#define true 1
#define false 0

//Deklarerer globale variabler og structer
bool jobber_igjen = true;
int sock, request_socket;
struct sockaddr_in serveradr;
struct sockaddr_in klientadr;

//buffer for innlesing av melding fra klient (Q, T eller E)
unsigned char hent_jobb_buf[2];

FILE *jobbfil;

/*
Main-funksjon aapner joblist-fil, setter opp socket,
kobler til én (den foerste) klient og behandler menyvalg
sendt fra klient ved utsending av jobber fra fil.
  argc: antall argumenter fra terminal
  argv: argumenter fra terminal
Returnerer 0 ved suksessfull terminering, -1 ved error.
*/
int main(int argc, char* argv[]){
  int portnr;

  if(argc < 3){
    fprintf(stderr, "Feil i argumenter.\nBruk: <%s>, <Jobbfil>, <Portnummer>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  //Aapner jobbfil
  if ((jobbfil = fopen(argv[1], "r")) == 0){
    error("fopen()");
  }

  //portnr = atoi(argv[2]);
  portnr = strtol(argv[2], NULL, 10);
  if(errno == ERANGE){
    error("strtol()");
  } else if(sizeof(portnr) != 4){
    printf("Portnummer oppgitt maa vaere firesiffret.\nAvslutter...\n");
    EXIT_FAILURE;
  }

  //Oppretter servers lytte-socket
  if((request_socket = opprett_socket(portnr)) == EXIT_FAILURE){
    if((close(request_socket)) == -1){
      error("close()");
    }
  error("opprett_socket()");
  }

  //oppretter klient-socket
  sock = lytt_tilkobling(request_socket);

  while(jobber_igjen == true){
    //Det er flere jobber tilgjengelig i filen

    //Tar imot jobbforespoersel fra klient's hent_jobb()
    if((recv(sock, &hent_jobb_buf[0], 1, 0)) == -1){
      error("recv()");
    }
    hent_jobb_buf[1] = '\0';
    char retur = hent_jobb_buf[0];
    if(retur == 'G'){
      send_jobb();
    } else if (retur == 'T'){
      printf("\nKlient terminert suksessfult.\nAvslutter...\n");
      if(avslutt() != 0){
        fprintf(stderr, "\nFeil i terminering av program med avslutt()\n");
        exit(EXIT_FAILURE);
      }
      exit(EXIT_SUCCESS);
    } else if (retur == 'E'){
      fprintf(stderr, "\nKlient terminert med error. \nAvslutter...\n");
      if(avslutt() != 0){
        fprintf(stderr, "\nFeil i terminering av program med avslutt()\n");
        exit(EXIT_FAILURE);
      }
      exit(EXIT_SUCCESS);
    } else {
      fprintf(stderr, "\nError hos klient: hent_jobb()\nAvslutter...\n");
      if(avslutt() != 0){
        fprintf(stderr, "\nFeil i terminering av program med avslutt()\n");
        exit(EXIT_FAILURE);
      }
      exit(EXIT_SUCCESS);
    }
  }

  printf("\nAvslutter...\n");
  if(avslutt() != 0){
    fprintf(stderr, "Feil i terminering av program med avslutt()\n");
    exit(EXIT_FAILURE);
  }
  return EXIT_SUCCESS;
}

/*
Oppretter en request_sd (socket descriptor) som skal
lytte paa klienter som proever aa koble til samme port.
Setter adresse paa servers adresse-struct.
Binder deretter adressen til lytte-socket.
  portnr: portnummer sendt inn fra terminal
Returnerer ny lyttesocket ved suksess, -1 ved error.
*/
int opprett_socket(int portnr){
  int request_sd;

  //oppretter socket med adressefamilie, stroem-kommunikasjon og protokoll (TCP)
  if((request_sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
    error("socket()");
  }
  //nullstiller servers adressestruct og setter nye verdier
  memset(&serveradr, 0, sizeof(struct sockaddr_in));
  serveradr.sin_family = AF_INET;
  //Alle tilgjengelige IP-adresser aksepteres
  serveradr.sin_addr.s_addr = INADDR_ANY;
  //konverterer portnummer til big-endian som nettverk krever
  serveradr.sin_port = htons(portnr);

  //Unngaar "adress already in use"-error, setter SO_REUSEADDR i socket options
  int activate = 1;
  if ((setsockopt(request_sd, SOL_SOCKET, SO_REUSEADDR, &activate, sizeof(int))) < 0){
    error("setsockopt(SO_REUSEADDR)");
  }
  //binder lyttesocket til serveradresse
  if((bind(request_sd, (struct sockaddr*)&serveradr, sizeof(struct sockaddr_in))) == -1){
    error("bind()");
  }
  return request_sd;
}

/*
lytte-socket lytter etter tilkoblingsforespoersel fra klient.
Aksepterer 1 tilkobling og danner deretter en ny socket for klienten.
  request_socket: Lyttesocket
Return: Ny socket ved suksess, -1 ved error.
*/
int lytt_tilkobling(int request_socket){
  int clientsock, klientadrlengde;

  printf("\nLytter etter tilkoblingsforespoersler...\n");

  //Bruker lytte-socket til aa lytte etter innkommende tilkoblingsforespoersler
  if((listen(request_socket, SOMAXCONN)) == -1){
    error("listen()");
  }
  klientadrlengde = sizeof(struct sockaddr_in);
  //Servers lytte-socket aksepterer tilkobling og oppretter ny socket for klient
  if((clientsock = accept(request_socket, (struct sockaddr*)&klientadr, &klientadrlengde)) == -1){
    error("accept()");
  }
  printf("Tilkobling akseptert!\n");

  return clientsock;
}

/*
Send_jobb kalles paa naar klient oensker aa hente jobb
(sender 'G') via hent_jobb(). Funksjonen leser én jobb fra
innsendt jobbfil og sender med én jobb til klient.
Meldingen skal bestaa av jobbtype 'O' eller 'E' char,
lengde_jobbtekst unsigned char og jobbtekst char[].
Er det ikke fler jobber igjen, sender Funksjonen
jobbfil_buf[2] = {'Q', '\0'} og setter jobber_igjen til false
  Ingen argumenter
Returnerer 1 ved slutt paa jobber, 0 ved suksess og -1 ved error
*/
int send_jobb(){
char jobbtype;
unsigned char lengde_jobbtekst;

 jobbtype = fgetc(jobbfil);

 lengde_jobbtekst = fgetc(jobbfil);
 if(lengde_jobbtekst == EOF){
   fprintf(stderr, "Naadd end of file: fgetc()");
 }

 if(jobbtype == EOF){
   //Naadd slutt paa fil
   printf("\nIngen jobber igjen, sender sluttmelding.\n");
   //Oppretter og sender buffer for sluttmelding til klient
   unsigned char jobbfil_buf[2];
   jobbfil_buf[0] = 'Q';
   jobbfil_buf[1] = '\0';

   if((send(sock, jobbfil_buf, sizeof(jobbfil_buf), 0)) == -1){
     error("send()");
   }
   //Signaliserer at det ikke er fler jobber igjen
   return 1;
 } else if (jobbtype != 'O' && jobbtype != 'E'){
   error("Innsendt char ikke kompitabel: fgetc()");
 }

 //Oppretter buffer for sending av én jobb til klient
 unsigned char jobbfil_buf[lengde_jobbtekst+1];

 jobbfil_buf[0] = jobbtype;
 jobbfil_buf[1] = lengde_jobbtekst;
 //Henter jobbtekst og legger inn f.o.m. indexplass 2 i jobbfil_buf
 if((fgets(jobbfil_buf+2, lengde_jobbtekst+1, jobbfil)) == NULL){
   error("NULL-verdi: fgets()");
 }

 if(jobbfil_buf[lengde_jobbtekst+1] == '\0'){
   //Buffer er ikke fylt opp med char
   printf("\nSetning i jobbfil kortere enn oppgitt, eller tekstlengde ikke satt.\nAvslutter...\n");
   error("send_jobb()");
 }
 //Sender buffer med type, lengde og jobbtekst til klient gjennom socket
 if((send(sock, jobbfil_buf, sizeof(jobbfil_buf)+1, 0)) == -1){
   error("send()");
 }

 //nullstiller buffer
 memset(jobbfil_buf, 0, sizeof(jobbfil_buf));

 return 0;
}

/*
Funksjonen avslutter programmet etter at klien har
avsluttet gjennom error eller suksessfull terminering
Sockets blir lukket og minne frigjort
  Ingen argumenter
Returnerer 0 ved suksess
*/
int avslutt(){
  //om vi har faatt T eller E av klient
  if((fclose(jobbfil)) == EOF){
    error("fclose()");
  }
  if((close(sock)) == -1){
    error("close()");
  }
  if((close(request_socket)) == -1){
    error("close()");
  }
  return 0;
}

/*
Hjelpefunksjon brukt til haandtering av error.
  msg: melding som skal skrives til output-stream
Returnerer ingenting.
*/
void error(char *msg){
  if(jobbfil != 0){
    //lukker jobbfil
    if((fclose(jobbfil)) == EOF){
      perror("fclose()");
    }
  }
  if(sock != 0){
    //lukker klientsocket
    if((close(sock)) == -1){
      perror("close()");
    }
  }
  if(request_socket != 0){
    //lukker lyttesocket
    if((close(request_socket)) == -1){
      perror("close()");
    }
  }
  perror(msg);
  exit(EXIT_FAILURE);
}

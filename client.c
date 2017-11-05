//Hjemmeeksamen INF1060, H2016

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
#include <sys/ioctl.h>

//Div, generelt
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

//Deklarerer funksjoner
int opprett_socket(char *ip, int Portnr);
void ordreloekke();
void hent_jobb();
void hent_flere_jobber();
void hent_alle_jobber();
void avslutt();

//Deklarerer hjelpefunksjoner
void error(char *msg);
void print_meny();

/*Typedefinerer bool int slik at
lesing av "boolean"-verdier blir enklere*/
typedef int bool;
#define true 1
#define false 0

//Deklarerer globale variabler og structer
bool jobber_igjen = true;

//struct brukt til servers adresse
struct sockaddr_in serveradr;
//struct brukt for aa finne IP til server via host name
struct hostent *hostadr;
struct in_addr **addr_list;

int sock;
pid_t child_out, child_err;

/*Buffer for meldinger til server.
G = hent jobb. T = Klient terminert suksessfult. E = Klient terminert med error*/
unsigned char buf[3] = {'G', 'T', 'E'};
//Buffer for lagring av data sendt fra server
unsigned char jobbliste_buf[257];
//pipes
int stdout_pipe1[2], stderr_pipe2[2];
int retur1, retur2;

/*
Main-funksjon. Oppretter 2 pipes, 2 barneprosesser,
kobler til server, skriver ut brukermeny og behandler menyvalg.
  argc: antall argumenter fra terminal
  argv: argumenter fra terminal
Returnerer 0 ved suksessfull terminering, -1 ved error.
*/
int main(int argc, char *argv[]){
  int portnr;
  char *ip;
  char *sjekk_ip;

  if(argc < 3){
    //For faa argumenter fra terminal
    fprintf(stderr, "Feil i argumenter.\nBruk: <%s>, <IP-adresse/Maskinnavn>, <Portnummer>\n", argv[0]);
    exit(EXIT_SUCCESS);
  }

  ip = argv[1];

  sjekk_ip = ip;
  if((hostadr = gethostbyname(sjekk_ip)) != NULL){
    //innsendt argument er hostname

    //Caster og legger host-adresseliste inn i addr_list struct
    addr_list = (struct in_addr **) hostadr->h_addr_list;
    //Velger foerste IP-adresse i listen som oensket server-IP-adresse
    ip = strcpy(ip , inet_ntoa(*addr_list[0]));
  }

  //Konverterer portnummer til int og sjekker for feil input
  portnr = strtol(argv[2], NULL, 10);
  if(errno == ERANGE){
    error("strtol()");
  } else if(sizeof(portnr) != 4){
    printf("Portnummer oppgitt maa vaere firesiffret.\nAvslutter...\n");
    EXIT_FAILURE;
  }

  //Opprett 2 pipes (index 0 for read, 1 for write)
  if((retur1 = pipe(stdout_pipe1)) == -1){
    error("pipe()");
  }
  if((retur2 = pipe(stderr_pipe2)) == -1){
    error("pipe()");
  }

  //Oppretter foerste barneprosess m. fork()
  if ((child_out = fork()) == -1){
    error("fork()");
  }

  if(child_out == 0){
    //Vi er i barneprosess 1 - stdout
    char *buff = NULL;
    char byte = 0;
    int count = 0;

    for(;;){
      //vi er i en "evig" loekke
      while(read(stdout_pipe1[0], &byte, 1) == 1){
        //vi har minst én byte aa lese
        //Sjekker read-stoerrelsen av pipen
        if(ioctl(stdout_pipe1[0], FIONREAD, &count) != -1){
          //fprintf(stdout, "\nData i pipe: %d\n", count);
          //allokerer plass til data i pipe + byten
          buff = malloc(count+1);
          buff[0] = byte;
          if(read(stdout_pipe1[0], buff+1, count) == count)
            //Skriver ut resten av dataen
            fprintf(stdout, "%s\n\n", buff);
          free(buff);
        } else {
          perror("Feilet i innlesing av input-strls.\n");
        }
      }
    }
    //Terminerer barneprosess 1
    printf("\nTerminerer barneprosess.\n");
    _Exit(2);

  } else {
    //Oppretter barneprosess 2
    if ((child_err = fork()) == -1){
      error("fork()");
    }
    if(child_err == 0){
      /*Vi er i barneprosess 2 - stderr;
      (Likt som i barneprosess 1, men fra annen pipe og sendes
      til stderr i stedet)*/
      char *buff = NULL;
      char byte = 0;
      int count = 0;

      for(;;){
        while(read(stderr_pipe2[0], &byte, 1) == 1){
          //vi har minst én byte aa lese
          //Sjekker read-stoerrelsen av pipen
          if(ioctl(stderr_pipe2[0], FIONREAD, &count) != -1){

            buff = malloc(count+1);
            buff[0] = byte;
            if(read(stderr_pipe2[0], buff+1, count) == count)
              //Skriver ut resten av dataen
              fprintf(stderr, "%s\n\n", buff);
            free(buff);
          } else {
            perror("Feilet i innlesing av input-strls.\n");
          }
        }
      }
      //Terminerer barneprosess 2
      printf("\nTerminerer barneprosess.\n");
      _Exit(2);

    } else {
      //Vi er i Parent
      //Kobler til server
      if((sock = opprett_socket(ip, portnr)) == -1){
        //Fikk ikke opprettet socket
        error("socket()");
      }

      //Printer ut brukermeny
      ordreloekke();

      //venter til alle barn har terminert
      wait(NULL);
      avslutt();
    }
    return EXIT_SUCCESS;
  }
}

/*
Oppretter Berkeley-socket, setter adresse og sender tilkoblingsforespoersel til server.
  IP: IP-adresse til server
  Portnr: Portnummer brukt
Return: returnerer socket ved suksess, -1 ellers
*/
int opprett_socket(char *ip, int Portnr){
  int sd;
  int ip_retur;

  //Oppretter socket med adressefamilie INET og stroem-kommunikasjon - TCP-protokoll
  if((sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
    error("socket()");
  }

  //nullstiller minneadresse
  memset(&serveradr, 0, sizeof(struct sockaddr_in));

  //setter serveradresse
  serveradr.sin_family = AF_INET;

  ip_retur = inet_pton(AF_INET, ip, &serveradr.sin_addr);
  if(ip_retur != 1){
  //feiler
    if(ip_retur == 0){
      fprintf(stderr, "Ugyldig IP-adresse: %s\n", ip);
    } else {
      perror("inet_pton()");
    }
    close(sd);
    exit(EXIT_FAILURE);
  }

  //Translater portnummer til big-endian (host-to-network)
  serveradr.sin_port = htons(Portnr);

  //Kobler til server (samt caster sockaddr_in struct til sockaddr struct)
  printf("\nKobler til server...\n");
  if((connect(sd, (struct sockaddr*)&serveradr, sizeof(struct sockaddr_in))) == -1){
    error("connect()");
  }
  printf("Tilkobling akseptert!\n");

  return sd;
}

/*
Printer ut brukermeny til terminal. Tar imot brukerinput
med switch case. Gaar til bruker avslutter eller klient terminerer.
  Tar ikke imot argumenter.
Returnerer ingenting.
*/
void ordreloekke(){
  int svar;
  int temp;

  do{
    /*Venter nok til at meny ikke blir printet ut foer stderr/stdout i barneprosesser
    NB! 0.5 holder til normal kjoering paa egen maskin, valgrind krever mer tid. Vet dette ser stygt ut*/
    sleep(1);
    print_meny();

    printf("\nVelg operasjon du vil utfoere (1-4): ");
    svar = fgetc(stdin);

    while ((temp = getchar()) != '\n') {
      /*
      Dropper eventuelle resterende tegn med mindre
      det er linjeskift
      */
    }
    printf("\n");

    switch(svar){
      case '1':
        hent_jobb();
        break;
      case '2':
        hent_flere_jobber();
        break;
      case '3':
        hent_alle_jobber();
        break;
      case '4':
        avslutt();
      default:
        printf("Vennligst oppgi et tall fra 1-3. 4 for aa avslutte program.\n");
    }
  } while(svar!='4' || svar!='3' || svar!='2' || svar!='1');
}

/*
Funksjon som sender 1-byte 'G' til server for aa signalisere oenske om
aa faa tilbake én jobb fra jobbfil i server. Venter paa jobbmelding fra server
og sender dette videre til barneprosess 1 eller 2 gjennom pipes for printing.
  Ingen argumenter.
Returnerer ingenting.
*/
void hent_jobb(){
  int jobbtekstlengde;
  FILE* savestdin;
  //Buffer som skal sendes til barneprosesser gjennom pipes
  char send_child_buf[255];

  //Sender 'G' (get-job-signal) til server
  send(sock, &buf[0], 1, 0);

  //Nullstiller buffere
  memset(jobbliste_buf, 0, sizeof(jobbliste_buf));
  memset(send_child_buf, 0, sizeof(send_child_buf));

  //mottar jobb fra server
  if((recv(sock, &jobbliste_buf, sizeof(jobbliste_buf), 0)) == -1){
    error("recv()");
  }

  if(jobbliste_buf[0] == 'O'){
    //teksten skal sendes til child 1 - stdout
    int lengde = jobbliste_buf[1];

    //Kopierer alt unntatt T og L (kun jobbtekst) til nytt buffer
    memcpy(&send_child_buf[0], &jobbliste_buf[2], lengde);

    //Sender til child 1 for printing til stdout, gjennom write-ende av pipe
    close(stdout_pipe1[0]);
    write(stdout_pipe1[1], send_child_buf, 255);

  } else if (jobbliste_buf[0] == 'E'){
    //Teksten skal sendes til child 2 - stderr
    int lengde = jobbliste_buf[1];

    memcpy(&send_child_buf[0], &jobbliste_buf[2], lengde);

    close(stderr_pipe2[0]);
    write(stderr_pipe2[1], send_child_buf, 255);

  } else if(jobbliste_buf[0] == 'Q'){
    printf("\nIngen flere jobber.\n");
    jobber_igjen = false;
    avslutt();
  } else {
    //Server har sendt inn buffer med feil informasjon
    fprintf(stderr, "Feil i server: send_jobb()\n");
    avslutt();
  }
}

/*
Funksjon som henter oensket antall jobber i innsendt jobbfil.
Vil bruker hente for mange, hentes resterende antall og klient termineres
paa vanlig maate.
NB! Bokstaver som input fra terminal gir her feil i antall kall paa hent_jobb().
  Ingen parametre.
Returnerer ingenting.
*/
void hent_flere_jobber(){
  int svar, temp, i;

  printf("Vennligst oppgi antall jobber du oensker aa hente: ");
  svar = getchar();

  //dropper resterende tegn
  while ((temp = getchar()) != '\n'){}

  printf("\n");

  for(i = '0'; i < svar && jobber_igjen == true; i++)
      hent_jobb();
}

/*
Funksjon som henter alle resterende jobber i innsendt jobbfil.
  Ingen argumenter.
Returnerer ingenting.
*/
void hent_alle_jobber(){
  while(jobber_igjen == true)
    hent_jobb();
}

/*
Funksjon for frigjoering av minne, lukking av pipes, sockets,
barnemord og suksessfull terminering med melding 'T' til server.
  Ingen argumenter.
Returnerer 0 som suksessfull terminering av prosess.
*/
void avslutt(){
  errno = 0;
  printf("Avslutter...\n");
  //Lukker eventuelle pipes mellom parent og barneprosesser
  if(retur1 != 0){
    close(stdout_pipe1[1]);
    close(stdout_pipe1[0]);
  }
  if(retur2 != 0){
    close(stderr_pipe2[1]);
    close(stderr_pipe2[0]);
  }
  //Sender termineringssignal til server ('T')
  if(sock != 0){
    send(sock, &buf[1], 1, 0);
    //Lukker socket
    close(sock);
  }
  //Terminerer eventuelle barneprosesser
  if(child_out != 0)
    kill(child_out, SIGTERM);
  if(child_err != 0)
    kill(child_err, SIGTERM);
  if(errno != 0){
    error("Error ved terminering av program: avslutt()");
  }
  exit(EXIT_SUCCESS);
}

/*
Hjelpefunksjon brukt til aa skrive ut brukermeny til terminal.
  Tar ikke imot argumenter.
Returnerer ingenting
*/
void print_meny(){
  printf("\nJ O B B L I S T E M E N Y\n");
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  printf("1. Hent én jobb\n");
  printf("2. Hent flere (X antall) jobber\n");
  printf("3. Hent alle jobber\n");
  printf("4. Avslutt program\n");
}

/*
Hjelpefunksjon brukt til haandtering av error.
Lukker eventuell socket
  msg: melding som skal skrives til output-stream
Returnerer ingenting.
*/
void error(char *msg){
  perror(msg);
  errno = 0;
  //Lukker eventuelle pipes mellom parent og barneprosesser
  if(retur1 != 0){
    close(stdout_pipe1[1]);
    close(stdout_pipe1[0]);
  }
  if(retur2 != 0){
    close(stderr_pipe2[1]);
    close(stderr_pipe2[0]);
  }
  //send errormelding til server - 'E'
  if(sock != 0){
    send(sock, &buf[2], 1, 0);
    //Lukker socket
    close(sock);
  }
  //Terminerer eventuelle barneprosesser
  if(child_out != 0)
    kill(child_out, SIGTERM);
  if(child_err != 0)
    kill(child_err, SIGTERM);

  if(perror != 0){
    fprintf(stderr, "Feil under terminering: error()");
  }

  exit(EXIT_FAILURE);
}

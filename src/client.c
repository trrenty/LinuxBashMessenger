#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/select.h>
#include <string.h>
#include <strings.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

int main (int argc, char *argv[])
{
  int sd;			// descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
  		// mesajul trimis
  int nr=0;
  char buf[256];

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  /* stabilim portul */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);
  
  /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }

               fd_set rfds;
           struct timeval tv;
           int retval;
  while (1)
  {
      FD_ZERO(&rfds);
      FD_SET(sd, &rfds);
      FD_SET(0, &rfds);
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      retval = select(sd+1, &rfds, NULL, NULL, &tv);

      fflush(stdout);

      if(retval == -1)
        {
            perror("select()");
            break;
        }
    else if (FD_ISSET(sd, &rfds))
    {
        if (read(sd, buf, sizeof(buf)) < 0) {
            perror("[client] Eroare la read() de la server. \n");
            return errno;
        }
        printf("%s", buf);
        bzero(buf, sizeof(buf));
    }
    else if (FD_ISSET(0, &rfds))
    {
        if (read(0, buf, sizeof(buf))< 0)
        {
            perror("[client] Eroare la read() de la tastatura. \n");
            return errno;
        }
        if (write(sd, buf, sizeof(buf)) < 0)
        {
            perror("[client] Eroare la read() de la tastatura. \n");
            return errno;
        }
        bzero(buf, sizeof(buf));
    }
    else if (retval<0)
        break;
  }
  /* inchidem conexiunea, am terminat */
  close (sd);
}

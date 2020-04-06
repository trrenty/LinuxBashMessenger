#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>

/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData{
  int idThread; //id-ul thread-ului tinut in evidenta de acest program
  int cl; //descriptorul intors de accept
  short isOnline;
  char usrName[30];
  char offlineMsg[100];
  short offlineMsgNo;
}thData;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *, int);
void InitUsers();
char* getFileName(int, int);
thData utilizatori[100];
int numarUtilizatori;

int main ()
{
  struct sockaddr_in server;  // structura folosita de server
  struct sockaddr_in from;  
  int sd;   //descriptorul de socket 
  int pid;
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
  int i=0;
  
  InitUsers();

  /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  

  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));
  

    server.sin_family = AF_INET;  
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    server.sin_port = htons (PORT);
  

  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }


  if (listen (sd, 2) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }
  
  while (1)
    {
      int client;
      thData * td; //parametru functia executata de thread     
      int length = sizeof (from);

      printf ("[server]Asteptam la portul %d...\n",PORT);
      fflush (stdout);
      /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
      if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
      {
        perror ("[server]Eroare la accept().\n");
        continue;
      }
  
  td=(struct thData*)malloc(sizeof(struct thData)); 
  td->idThread=i;
  td->cl=client;
  i++;
  pthread_create(&th[i], NULL, &treat, td);       
        
  }   
};   

static void *treat(void * arg)
{   
    struct thData tdL; 
    tdL= *((struct thData*)arg);  
    char login[256];
    printf ("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
    fflush (stdout);
 login:
    if (read(tdL.cl, login, sizeof(login)) < 0)
    {
      printf("[Thread %d]\n",tdL.idThread);
      perror ("Eroare la read() de la client.\n");
      return (NULL);
    }     

   // printf("%s\n", login);
    int idUtilizator;
    if ((idUtilizator = Login(login, tdL.cl)) < 0)
    {
      printf("Nu-i bun\n");
      strcpy(login, "Eroare login - login username\n");
      if (write(tdL.cl, login, sizeof(login))<0)
      {
        printf("[Thread %d]\n",tdL.idThread);
        perror ("Eroare la write() catre client.\n");
      }
      goto login;
    }
    else {
      strcpy(login, "Ati fost conectat cu succes\n");
      if (write(tdL.cl, login, sizeof(login))<0)
      {
        printf("[Thread %d]\n",tdL.idThread);
        perror ("Eroare la write() catre client.\n");
      }
    }
    
    getNewMessages(idUtilizator);

    pthread_detach(pthread_self());

    raspunde((struct thData*)arg, idUtilizator);
    /* am terminat cu acest client, inchidem conexiunea */
    close ((intptr_t)arg);
    return(NULL); 
      
};



int Login(char * comanda, int descriptor) {
  char* username;
  char* login;
  login = strtok(comanda, " \n");
  if (strcmp("login", login) != 0) 
    return -1 ;
  username = strtok(NULL, " \n");

  //username[strlen(username) - 1] = '\0';

  for (int i = 0; i<numarUtilizatori ; i++)
  {
    if (!strcmp(username, utilizatori[i].usrName))
    {
      utilizatori[i].cl = descriptor;
      utilizatori[i].isOnline = 1;
      return i;
    }
  }
  return -1;
}


int activeUsers(int descriptor) {

  char text[256] = "Users:\n"; int j = 0;
  for (int i = 0; i<numarUtilizatori; i++)
  {
    if (utilizatori[i].isOnline!=0)
    {
      strcat(text, utilizatori[i].usrName);
      strcat(text, " is online! \n");
      j++;
    } 
  }
  if (!strcmp(text, "Users:\n"))
  {
    strcat(text, "No active users! \n");
    if (write(descriptor, text, sizeof(text)) < 0)
      {
        printf("[Thread %d]\n",descriptor);
        perror ("Eroare la write() catre client.\n");
        return -1;
      }
    return 0;
  }
  //printf("%s\n", text);
  if (write(descriptor, text, sizeof(text)) < 0)
  {
    printf("[Thread %d]\n",descriptor);
    perror ("Eroare la write() catre client.\n");
    return -1;
  }
  return j;
}

int showHistory(char * mesaj, int userId) {
  char aux[256];
  if (mesaj != NULL)
    sprintf(aux, "%s", mesaj);
  if (strcmp(strtok(aux, " "), "history")!= 0)
    return 0;

  char*username = strtok(NULL, " \n");
  printf("%s", username);
  for (int i = 0; i< numarUtilizatori;  i++) {
    if (!strcmp(username, utilizatori[i].usrName)) {
      char filename[60];
      char user1[30];
      char user2[30];
      sprintf(user1, "%s",utilizatori[userId].usrName );
      sprintf(user2, "%s",utilizatori[i].usrName );
  
       if (userId > utilizatori[i].idThread)
          sprintf(filename,"%s%s.txt", strtok(user2, "."),strtok(user1, "."));
        else 
          sprintf(filename,"%s%s.txt", strtok(user1, "."),strtok(user2, "."));
      int fd = open(filename, O_RDONLY);
      if (fd < 0)
      {
        sprintf(aux, "Nu exista istoric pentru aceasta conversatie\n");
         if (write(utilizatori[userId].cl, aux, sizeof(aux) )<0)
         {
            perror ("Eroare la write() catre client.\n");
            return -1;
         }
        return 1;
      }
      else {
        while(read(fd, aux, sizeof(aux) - 1) > 0)
        {
          if (write(utilizatori[userId].cl, aux, sizeof(aux))< 0 )
          {
             perror ("Eroare la write() catre client.\n");
            return -1;
          }
          bzero(aux, sizeof(aux));
        }
        close(fd);
        return 1;
      }
    }
  }
  sprintf(aux, "Nu exista acest utilizator\n");
  printf("%s\n", aux);
  if (write(utilizatori[userId].cl, aux, sizeof(aux) )<0)
  {
    perror ("Eroare la write() catre client.\n");
            return -1;
  }
  return 1;


}

void raspunde(void *arg, int idUser)
{
  char mesaj[256];
  struct thData tdL; 
  tdL= *((struct thData*)arg);
  int idReceiver;


  while(1)
  {
    bzero(mesaj, sizeof(mesaj));
    if (read (tdL.cl, mesaj,sizeof(mesaj)) <= 0)
      {
        printf("[Thread %d] Clientul %s s-a deconectat.\n", tdL.idThread, utilizatori[idUser].usrName);
        //perror ("Eroare la read() de la client.\n");
        utilizatori[idUser].isOnline = 0;
        return;
      }
    
    if (showHistory(mesaj, idUser) > 0)
    {
      continue;
    }
    
    if (!strcmp(mesaj, "onUsers\n"))
    {
      if (activeUsers(tdL.cl) < 0)
      {
        utilizatori[idUser].isOnline = 0;
        return;
      }
      
      else
      continue;
    }
    if (strstr(mesaj, "reply") != NULL)
      if (!strcmp(strstr(mesaj, "reply"), mesaj))
      {
        Reply(mesaj ,idUser);
        continue;
      }

    int idReceiver = strlen(mesaj);              
    printf("[Thread %d]Mesajul este: \"%s\". \n",tdL.idThread, mesaj);
    if (idReceiver == strlen(strtok(mesaj, ":")))
    {
    char tmp[] = "Nu ati precizat utilizatorul spre care vrei sa trimiti mesaj.";
    
    if (write(tdL.cl, tmp, sizeof(tmp)) < 0)
    {
       printf("[Thread %d]\n",tdL.idThread);
        perror ("Eroare la write() catre client.\n");
        utilizatori[idUser].isOnline = 0;
        return;
    }
    continue;
    }
  idReceiver = atoi(strtok(mesaj, ":"));
  int offlineToken = 0;
  if (!utilizatori[idReceiver].isOnline)
  {
    offlineToken = 1;
    goto offlineMessage;
  }
  char * preMsg[34];
  sprintf(preMsg, "%s : ", utilizatori[idUser].usrName);
  if (write(utilizatori[idReceiver].cl, preMsg, strlen(preMsg))<0)
  {
    printf("[Thread %d]\n",tdL.idThread);
    perror ("Eroare la write() catre client.\n");
    utilizatori[idUser].isOnline = 0;
    return;
  }
    
  if (write(utilizatori[idReceiver].cl, mesaj+strlen(mesaj)+1, sizeof(mesaj)-1) < 0)
  {
    printf("[Thread %d]\n",tdL.idThread);
    perror ("Eroare la write() catre client.\n");
    utilizatori[idUser].isOnline = 0;
    return;
  }
  offlineMessage: ;

  char user1[30];
  char user2[30];
  sprintf(user1, "%s",utilizatori[idUser].usrName );
  sprintf(user2, "%s",utilizatori[idReceiver].usrName );
  char fileName[60];
  if (idUser > idReceiver)
    sprintf(fileName,"%s%s.txt", strtok(user2, "."),strtok(user1, "."));
  else 
    sprintf(fileName,"%s%s.txt", strtok(user1, "."),strtok(user2, "."));
  
  FILE *fd = fopen(fileName, "a+");
  sprintf(user1, "%s : ", utilizatori[idUser].usrName);
  printf( "%s%s\n", user1, mesaj+strlen(mesaj)+1 );
  time_t currentTime = time(NULL);
  if (!offlineToken)
    printf("%d\n", fprintf(fd, "~%s%s%s\n",ctime(&currentTime),user1, mesaj+strlen(mesaj)+1 ));
  else 
  {
    printf("%d\n", fprintf(fd, "^%s%s%s\n",ctime(&currentTime),user1, mesaj+strlen(mesaj)+1 ));
    for (int i = 0; i<utilizatori[idReceiver].offlineMsgNo; i++)
    {
      if (utilizatori[idReceiver].offlineMsg[i] == idUser)
      {
        offlineToken = 2;
        break;
      }
    }
    if (offlineToken != 2)
    {
       utilizatori[idReceiver].offlineMsg[utilizatori[idReceiver].offlineMsgNo] = idUser;
       utilizatori[idReceiver].offlineMsgNo++;
    }
  }
  fclose(fd); 
  }         
}

void InitUsers() {
  int fd = open("conturi.txt", O_RDONLY);
  char c; char nume[30];
	int i = 0, j = 0;
	while (read(fd, &c, 1) > 0) {
		if (c == '\n' || c == EOF) {
	  	nume[i] = '\0';
      
      utilizatori[j].idThread = j;
      
      strcpy(utilizatori[j].usrName, nume);
      utilizatori[j].isOnline = 0;
      utilizatori[j].cl = -1;
      j++;
      utilizatori[j].offlineMsgNo = 0;
      numarUtilizatori++;
      i = 0;
			read(fd, &c, 1);
		}
    nume[i] = c;
	  i++;
	}
  close(fd);
}


void getNewMessages(int idUser) {
  for (int i = 0; i<utilizatori[idUser].offlineMsgNo; i++)
  {
    char user1[30];
    char user2[30];
    int idSender = (int)utilizatori[idUser].offlineMsg[i];
    sprintf(user1, "%s",utilizatori[idUser].usrName);
    sprintf(user2, "%s",utilizatori[idSender].usrName); 
    char fileName[60];
  if (idUser > idSender)
    sprintf(fileName,"%s%s.txt", strtok(user2, "."),strtok(user1, "."));
  else 
    sprintf(fileName,"%s%s.txt", strtok(user1, "."),strtok(user2, "."));
  
  char c;
  int fd = open(fileName, O_RDWR);
  int doIt = 1;
  while (read(fd, &c, 1)>0)
  {
    if (c == '^')
    {
      int search = lseek(fd, -1, SEEK_CUR);
      c = '~';
      write(fd,&c, 1);
      if (doIt)
      {
        doIt = 0;
        char mesaj[256];
        while (read(fd, mesaj, 255)>0)
        {
          write(utilizatori[idUser].cl, mesaj, 255);
        }
        lseek(fd, search, SEEK_SET);
      }
    }
  }
  }
}

int Reply (char * mesaj, int idSender) {
  strtok(mesaj, " ");
  char * receiverUserName = strtok(NULL, " ");
  int messageId = atoi(strtok(NULL, " "));
  short validUser = 0;
  int receiverId;

  printf("%s\n%d\n", receiverUserName, messageId);

  for (int i = 0; i<numarUtilizatori;i++)
  {
    if (!strcmp(receiverUserName, utilizatori[i].usrName))
    {
      validUser=1;
      receiverId = i;
      break;
    }
  }

  if (!validUser)
  {
    sprintf(mesaj, "Username utilizator invalid!\n");
    if (write(utilizatori[idSender].cl, mesaj, 30)<0)
    {
      printf("[Thread %d] Eroare trimitere mesaj catre client\n");
      return -1;
    }
    return 0;
  }
  messageId = (messageId-1)*3 + 1;
  printf("%d\n", messageId);
  printf("%d\n", NewLinesInFile(getFileName(idSender, receiverId)));
  if (messageId < 0 || messageId > NewLinesInFile(getFileName(idSender, receiverId))) {
    sprintf(mesaj, "Identificator mesaj gresit\n");
    if (write(utilizatori[idSender].cl, mesaj, 28)<0)
    {
      printf("[Thread %d] Eroare trimitere mesaj catre client\n");
      return -1;
    }
    return 0;
  }
  int i = 0, j = 0;
  while (i<3)
  {
    if (mesaj[j] == '\0')
    {
      i++;
    }
    j++;
  }
  char toSend[256];
  sprintf(toSend, "%s", mesaj + j);
  
  
  FILE* fd = fopen(getFileName(idSender, receiverId), "a+");
  char c;
  while (messageId > 0)
  {
    c = fgetc(fd);
    if (c == '\n')
    {
      messageId--;
    }
  }
  c = 'a';
  messageId = 0;
  
  fgets(mesaj, 256, fd);
  mesaj[strlen(mesaj)-1] = '\0';

 // printf("ReplyTo : %s - %s\n", mesaj, toSend);
  
  fseek(fd, 0, SEEK_END);
  long int position = ftell(fd);
  //printf("%s\n", ctime(&));
  time_t currentTime = time(NULL);
  fprintf(fd, "~%s%s: ReplyTo: <%s> - %s\n", ctime(&currentTime), utilizatori[idSender].usrName, mesaj, toSend);
  fseek(fd, position, SEEK_SET);
  while(fread(mesaj, 1, 256, fd) > 0)
  {
    write(utilizatori[receiverId].cl, mesaj, 256);
  }
  fclose(fd);
  return 0;

}

int NewLinesInFile(char * fileName)
{
  int descriptor = open(fileName, O_RDONLY);
  char c; int newLines = 0;
  while (read(descriptor, &c, 1) == 1)
  {
    if (c == '\n')
      newLines++;
  }
  close(descriptor);
  return newLines;
}

char * getFileName(int userOne, int userTwo) 
{
  char user1[30];
  char user2[30];
  sprintf(user1, "%s",utilizatori[userOne].usrName );
  sprintf(user2, "%s",utilizatori[userTwo].usrName );
  char *fileName = (char*)malloc(sizeof(char)*3);
  if (userOne > userTwo)
    sprintf(fileName,"%s%s.txt", strtok(user2, "."),strtok(user1, "."));
  else 
    sprintf(fileName,"%s%s.txt", strtok(user1, "."),strtok(user2, "."));
  return fileName;
}
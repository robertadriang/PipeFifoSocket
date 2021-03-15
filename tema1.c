/*
Design and implement the following communication protocol among processes:

- the communication is done by executing commands read from the keyboard in the parent process and executed in child processes
- the commands are strings bounded by a new line
- the responses are series of bytes prefixed by the length of the response
- the result obtained from the execution of any command will be displayed on screen by the parent process
- the minimal protocol includes the following commands:
    - "login: username" - whose existence is validated by using a configuration file
    - "myfind file" - a command that allows finding a file and displaying information associated with that file; the displayed information will contain the creation date, date of change, file size, file access rights, etc.
    - "mystat file" - a command that allows you to view the attributes of a file
    - "quit"
- no function in the "exec()" family (or other similar) will be used to implement "myfind" or "mystat" commands in order to execute some system commands that offer the same functionalities
- the communication among processes will be done using all of the following communication mechanisms: pipes, fifos, and socketpairs.
*/

/*1. login*/
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <dirent.h>

#define MAX_READ 256
#define bool _Bool
#define FIFO_PtoC "myFifo"
#define FIFO_CtoP "myFifo2"
#define HOMEDIR "/home"

char instruction[256];
bool logedIn=0;

int validUser(char* username)
{
    FILE* userlist;
    if((userlist=fopen("database.txt","r"))==NULL)
    {
        perror("[ERROR] Can't open the file.\n");
        exit(12);
    }
    if(fseek(userlist,0,SEEK_SET)==-1)
    {
        perror("[ERROR] Can't reposition to start of file\n");
        exit(13);
    }
    char dataline[256];
    while(fgets(dataline,256,userlist)!=NULL)
    {
        int compSize;
        if(strlen(dataline)>strlen(username))
            compSize=strlen(dataline)-1;
        else
            compSize=strlen(username);
        if(strncmp(username,dataline,compSize)==0)
        {
            fclose(userlist);
            return 1;
        }
    }    
    fclose(userlist);
    return 0;
}

int mystat(char* command,char* answer,int* answerLen)
{
    struct stat sb;
    if (stat(command, &sb) == -1)
    {
        //perror("[ERROR] Can't create stat structure ");
       // exit(28);
       return 0;
    }
    sprintf(answer+strlen(answer),"%s\n",command);
    sprintf(answer+strlen(answer),"File type: ");
    switch (sb.st_mode & S_IFMT) 
    {
        case S_IFBLK:  sprintf(answer+strlen(answer),"block device\n");            break;
        case S_IFCHR:  sprintf(answer+strlen(answer),"character device\n");        break;
        case S_IFDIR:  sprintf(answer+strlen(answer),"directory\n");               break;
        case S_IFIFO:  sprintf(answer+strlen(answer),"FIFO/pipe\n");               break;
        case S_IFLNK:  sprintf(answer+strlen(answer),"symlink\n");                 break;
        case S_IFREG:  sprintf(answer+strlen(answer),"regular file\n");            break;
        case S_IFSOCK: sprintf(answer+strlen(answer),"socket\n");                  break;
        default:       sprintf(answer+strlen(answer),"unknown?\n");                break;
    }//statman2 for file type 
    char permissions[10];
    permissions[0]=(sb.st_mode & S_IRUSR) ? 'r' : '-';
    permissions[1]=(sb.st_mode & S_IWUSR) ? 'w' : '-';
    permissions[2]=(sb.st_mode & S_IXUSR) ? 'x' : '-';
    permissions[3]=(sb.st_mode & S_IRGRP) ? 'r' : '-';
    permissions[4]=(sb.st_mode & S_IWGRP) ? 'w' : '-';
    permissions[5]=(sb.st_mode & S_IXGRP) ? 'x' : '-';
    permissions[6]=(sb.st_mode & S_IROTH) ? 'r' : '-';
    permissions[7]=(sb.st_mode & S_IWOTH) ? 'w' : '-';
    permissions[8]=(sb.st_mode & S_IXOTH) ? 'x' : '-';
    permissions[9]='\0'; 
    sprintf(answer+strlen(answer),"File permissions: %s \n",permissions);
    sprintf(answer+strlen(answer),"Ownership: UID=%ld GID=%ld\n",(long) sb.st_uid, (long) sb.st_gid);
    sprintf(answer+strlen(answer),"File size: %lld bytes\n",(long long)(sb.st_size));
    sprintf(answer+strlen(answer),"Last status change:       %s", ctime(&sb.st_ctime));
    sprintf(answer+strlen(answer),"Last file access:         %s", ctime(&sb.st_atime));
    sprintf(answer+strlen(answer),"Last file modification:   %s", ctime(&sb.st_mtime));
    //printf("Formatted answer:\n %s total size %d:",answer,(int)strlen(answer));
    *answerLen=strlen(answer);
    return 1;
}

void searchFileRecursively(char* fileName,char* basePath,char* answer)
{
    char path[256];
    struct dirent *read;
    DIR * dir;
    if((dir=opendir(basePath))==NULL)//not a directory
    {
        return;
    }
    while((read=readdir(dir))!=NULL)
    {
        if(strcmp(read->d_name,".")!=0&&strcmp(read->d_name,"..")!=0)
        {
            strcpy(path,basePath);
            strcat(path, "/");
            strcat(path,read->d_name);
            if(strstr(read->d_name,fileName)!=NULL)
            {
                sprintf(answer+strlen(answer),"%s\n",path);
              // mystat(path,answer,answerLen);
            }
            searchFileRecursively(fileName,path,answer);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv)
{
    pid_t pid;
    while(1)
    {
       // printf("[Console]: Introduceti o instructiune: ");
        fgets(instruction,MAX_READ,stdin); //fgets ia si caracterul end *FIX IT*
        //printf("%s , %d",instruction,strlen(instruction));
        if(strncmp(instruction,"login : ",8)==0&&logedIn==0)
        {
            printf("Comanda valida\n");
            int PtoC[2],CtoP[2];
            if(pipe(PtoC)==-1)
            {
                perror("[ERROR] Can't create pipe PtoC. \n");
                exit(1);
            }
            if(pipe(CtoP)==-1)
            {
                perror("[ERROR] Can't create pipe CtoP. \n");
                exit(2);
            }
            if((pid=fork())==-1)
            {
                perror("[ERROR] Can't fork\n");
                exit(3);
            }
            if(pid)//Parent
            {
                if(close(PtoC[0])==-1)
                {
                    perror("[ERROR] Can't close PtoC for READING\n");
                    exit(4);
                }
                if(close(CtoP[1])==-1)
                {
                    perror("[ERROR] Can't close CtoP for WRITING\n");
                    exit(5);
                }
                int instructionLength=strlen(instruction)+1;
                if(write(PtoC[1],&instructionLength,sizeof(instructionLength))==-1)
                {
                    perror("[ERROR] Can't write size of message to PtoC\n");
                    exit(6);
                }
                if(write(PtoC[1],instruction,instructionLength)==-1)
                {
                    perror("[ERROR] Can't write message to PtoC\n");
                    exit(7);
                }
                if(close(PtoC[1])==-1)
                {
                    perror("[ERROR] Can't close PtoC for WRITING\n");
                    exit(21); 
                }
                wait(NULL);
                int answerLength;
                if(read(CtoP[0],&answerLength,sizeof(int))==-1)
                {
                    perror("[ERROR] Can't read answer length from CtoP\n");
                    exit(18);
                }
                char *answer=malloc((answerLength+1)*sizeof(char));
                if(read(CtoP[0],answer,answerLength)==-1)
                {
                    perror("[ERROR] Can't read message from CToP\n");
                    exit(19);
                }
                if(close(CtoP[0])==-1)
                {
                    perror("[ERROR] Can't close CtoP for READING\n");
                    exit(22);
                }
                answer[answerLength]='\0';
                printf("%s \n",answer);
                if(answer[9]=='f')
                    logedIn=1;
                printf("Login status: %d\n",logedIn);


            }
            else//Children
            {
                int sizeOfCommand;
                if(close(PtoC[1])==-1)
                {
                    perror("[ERROR] Can't close PtoC for WRITING\n");
                    exit(8);
                }
                if(close(CtoP[0])==-1)
                {
                    perror("[ERROR] Can't close CtoP for READING\n");
                    exit(9);
                }
                if(read(PtoC[0],&sizeOfCommand,sizeof(int))==-1)
                {
                    perror("[ERROR] Can't read the command size from PtoC\n");
                    exit(10);
                }
                char* commandReceived = malloc(sizeof(char) * sizeOfCommand);
                if(read(PtoC[0],commandReceived,sizeOfCommand)==-1)
                {
                    perror("[ERROR] Can't read the command from PtoC\n");
                    exit(11);
                }
                if(close(PtoC[0])==-1)
                {
                    perror("[ERROR] Can't close PtoC for READING\n'");
                    exit(20);
                }
                /*Pentru ca cerinta dorea transmiterea directa a comenzilor de la parinte catre fiu
                fara prelucrare vom trimite/citi prin pipe-ul parinte-fiu 2 caractere suplimentare: 
                1. Caracterul newline preluat de functia fgets
                2. Caracterul EOF transmis prin pipe;*/
                char* commandReady=malloc(sizeof(char)*(sizeOfCommand-2));
                strncpy(commandReady,commandReceived,sizeOfCommand-2);
                free(commandReceived);
                char* user=commandReady+8;
                if(validUser(user)==1)//valid user
                {
                    logedIn=1; /*Inutil setam valoarea pe 1 degeaba*/
                    char* answer="Username found! Login succesful";
                    int answerLen=strlen(answer);
                    if(write(CtoP[1],&answerLen,sizeof(answerLen))==-1)
                    {
                    perror("[ERROR] Can't write message length to CtoP\n");
                    exit(14);
                    }
                    if(write(CtoP[1],answer,answerLen)==-1)
                    {
                    perror("[ERROR] Can't write message to CtoP\n");
                    exit(15);
                    }
                }
                else //invalid user
                {
                    char* answer="Username not found! Login failed";
                    int answerLen=strlen(answer);
                    if(write(CtoP[1],&answerLen,sizeof(answerLen))==-1)
                    {
                    perror("[ERROR] Can't write message length to CtoP\n");
                    exit(16);
                    }
                    if(write(CtoP[1],answer,answerLen)==-1)
                    {
                    perror("[ERROR] Can't write message to CtoP\n");
                    exit(17);
                    }
                }
                if(close(CtoP[1])==-1)
                {
                    perror("[ERROR] Can't close CtoP for WRITING\n");
                    exit(21);
                }
                exit(0); 
            }
            
        }
        else if (strncmp(instruction,"login : ",8)==0&&logedIn==1)
        {
            printf("You are already logged in!\n");
        }
        else if(logedIn==0)
        {
            printf("You are not logged in!\n");
        }
        else if (strncmp(instruction,"mystat ",7)==0&&logedIn==1)
        {
             printf("Comanda valida\n");
             if((pid=fork())==-1)
            {
                perror("[ERROR] Can't fork\n");
                exit(23);
            }
            if(pid)//Parent
            {
              /*if(mknod(FIFO_PtoC, S_IFIFO | 0666, 0)==-1)
                {
                    perror("[ERROR] Can't create fifo\n");
                    exit(24);
                }*/
                int fdPtoC;
                if((fdPtoC=open(FIFO_PtoC, O_WRONLY))==-1)
                {
                    perror("[ERROR] Can't open PtoC FIFO");
                    exit(33);
                }
                int instructionLength=strlen(instruction)+1;
                if(write(fdPtoC,&instructionLength,sizeof(int))==-1)
                {
                    perror("[ERROR] Can't write message size to fifo PtoC\n");
                    exit(24);
                }
                if(write(fdPtoC,instruction,instructionLength)==-1)
                {
                    perror("[ERROR] Can't write instruction to fifo PtoC\n");
                    exit(25);
                }
                close(fdPtoC);
                int fdCtoP;
                if((fdCtoP=open(FIFO_CtoP, O_RDONLY /*| O_NONBLOCK */))==-1)
                {
                    perror("[ERROR] Can't open fifo CtoP\n");
                    exit(26);
                }
                wait(NULL);
                //sleep(1);
                
                int sizeOfAnswer=0;
                if(read(fdCtoP, &sizeOfAnswer, sizeof(int))==-1)
                {
                    perror("[ERROR] Can't read message size from fifo CtoP\n");
                    exit(31);
                }
                //printf("Size of message: %d\n",sizeOfAnswer);
                char* answer=malloc(sizeof(char)*(sizeOfAnswer+1));
                if(read(fdCtoP,answer,sizeOfAnswer)==-1)
                {
                    perror("[ERROR] Can't read message from fifo CtoP\n");
                    exit(32);
                }
                answer[sizeOfAnswer]='\0';
                printf("%s\n",answer);
                fflush(stdout);
            }
            else//Child
            {
                
                int fdPtoC=open(FIFO_PtoC, O_RDONLY);
                int sizeOfCommand;
                if(read(fdPtoC,&sizeOfCommand,sizeof(int))==-1)
                {
                    perror("[ERROR] Can't read message size from fifo PtoC\n");
                    exit(26);
                }
                char* commandReceived = malloc(sizeof(char) * sizeOfCommand);
                if(read(fdPtoC,commandReceived,sizeOfCommand)==-1)
                {
                    perror("[ERROR] Can't read message from fifo PtoC\n");
                    exit(27);
                }
                close(fdPtoC);
                char* commandReady=malloc(sizeof(char)*(sizeOfCommand-2));
                strncpy(commandReady,commandReceived,sizeOfCommand-2);
                free(commandReceived);
                //Stat mimic
                char answer[1000];
                int answerLen;
                int * answerPtr=&answerLen;

                int correct=mystat(commandReady+7,answer,answerPtr);
                if(correct==0)
                {
                    sprintf(answer,"%s is not a file or directory!",commandReady+7);
                }
               /* 
                if(mknod(FIFO_CtoP, S_IFIFO | 0666, 0)==-1)
                {
                    perror("[ERROR] Can't create fifo\n");
                    exit(24);
                }
                */
                int fdCtoP;
                if((fdCtoP=open(FIFO_CtoP, O_WRONLY))==-1)
                {
                    perror("[ERROR] Can't open fifo CtoP");
                    exit(30);
                }
                if(write(fdCtoP,&answerLen,sizeof(int))==-1)
                {
                    perror("[ERROR] Can't write message size to fifo CtoP\n");
                    exit(29);
                }
                int written;
                if((written=write(fdCtoP,answer,answerLen))==-1)
                {
                    perror("[ERROR] Can't write anwer to fifo CtoP\n");
                    exit(30);
                }
                close(fdCtoP);
                exit(0);
            }
        }
        else if (strncmp(instruction,"myfind ",7)==0&&logedIn==1)
        {
            int socketPair[2];
            if(socketpair(AF_UNIX, SOCK_STREAM,0, socketPair)<0)
            {
                perror("[ERROR] Can't create socker pair\n");
                exit(34);
            }
            if((pid=fork())==-1)
            {
                perror("[ERROR] Can't fork\n");
                exit(35);
            }
            if(pid)//Parent
            {
                if(close(socketPair[1])==-1)
                {
                    perror("[ERROR] Can't close socketPair 1\n'");
                    exit(36);
                }
                int instructionLength=strlen(instruction)+1;
                if(write(socketPair[0],&instructionLength,sizeof(int))==-1)
                {
                    perror("[ERROR] Can't write instruction length to socket\n");
                    exit(37);
                }
                if(write(socketPair[0],instruction,instructionLength)==-1)
                {
                    perror("[ERROR] Can't write instruction to socket1\n");
                    exit(38);
                }
                wait(NULL);
                int answerSize;
                if(read(socketPair[0],&answerSize,sizeof(int))==-1)
                {
                    perror("[ERROR] Can't read answer length from socket1\n");
                    exit(44);
                }
                char* answer=malloc(sizeof(char)*(answerSize+1));
                if(read(socketPair[0],answer,answerSize)==-1)
                {
                    perror("[ERROR] Can't read answer from socket1\n");
                    exit(45);
                }
                printf("%s\n",answer);
                free(answer);
            }
            else//Child
            {
                if(close(socketPair[0])==-1)
                {
                    perror("[ERROR] Can't close socketPair 0\n'");
                    exit(39);
                }
                int sizeOfCommand;
                if(read(socketPair[1],&sizeOfCommand,sizeof(int))==-1)
                {
                    perror("[ERROR] Can't read message size from socketPair 1\n");
                    exit(40);
                }
                char* commandReceived = malloc(sizeof(char) * sizeOfCommand);
                if(read(socketPair[1],commandReceived,sizeOfCommand)==-1)
                {
                    perror("[ERROR] Can't read message from socketPair 1\n");
                    exit(41);
                }
                char* commandReady=malloc(sizeof(char)*(sizeOfCommand-2));
                strncpy(commandReady,commandReceived,sizeOfCommand-2);
                //printf("Mesaj primit: %s cu dimensiunea %d\nMesaj prelucrat: %s cu dimensiunea %d\n",commandReceived,strlen(commandReceived),commandReady,strlen(commandReady));
                free(commandReceived);
                //printf("Trimis:%s",commandReady+7);
                char aux[10000];
                char aux2[100000];
                int aux2Len;
                int * aux2Ptr=&aux2Len;
                searchFileRecursively(commandReady+7,HOMEDIR,aux);
                if(strlen(aux)==0)
                {
                    sprintf(aux2,"\nNo file or directory found with name %s",commandReady+7);
                    aux2Len=strlen(aux2);
                }
                else
                {
                sprintf(aux2,"\n~~~~~~~~~~~~~~~~~~~~~~~~~~~\nFiles found:\n~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n%s\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~\nDetailed informations:\n~~~~~~~~~~~~~~~~~~~~~~~~~~~\n",aux);
                int i=0,j=0;
                char auxStat[256];
                while(aux[i]!='\0')
                {
                    if(aux[i]=='\n')
                    {
                        auxStat[j]='\0';
                       // printf("%s\n",auxStat);
                        mystat(auxStat,aux2,aux2Ptr);
                        sprintf(aux2+strlen(aux2),"\n\n");
                        j=0;
                    }
                    else
                    {
                       auxStat[j++]=aux[i];
                    }
                    
                   // printf("%c",aux[i]);
                    i++;
                }
                }
                //printf("%s%d",aux2,aux2Len);
                if(write(socketPair[1],&aux2Len,sizeof(int))==-1)
                {
                    perror("[ERROR] Can't write answer length to Socket1\n");
                    exit(42);
                }
                if(write(socketPair[1],aux2,aux2Len)==-1)
                {
                    perror("[ERROR] Can't write answer to Socket1\n");
                    exit(43);
                }
                close(socketPair[1]);
                exit(0);
            }
        }
        else if(strncmp(instruction,"quit",4)==0)
        {
            printf("Quitting...\n");
            exit(99);
        }
        else
        {
            printf("Unknown command! Try one of the following: \nlogin\nmystat [fullpath]\nmyfind [filename]\n");
        } 
    }
}
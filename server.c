#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>

/***********************
This protocol looks like:
Packet Offset 0: Message type:
    1 - time request
    2 - data point entry
    20 - SEKRIT BACKDOOR (NSA!??!?)

*Message Types
Time request:
    takes no additional data returns timestamp

Data point entry:
    Packet Offset 1: data point type
        1 - int
        2 - message
        3 - blob

    Int:
    Packet Offset 2: unsigned long point ID
    Packet Offset 6: unsigned long point value

    Message:
    Packet Offset 2: unsigned long length
    Packet Offset 6: Character array of length
    Packet Offset 6+len: checksum for string
        This checksum is the number required to make checksum+(sum of all bytes in string) = 0

    Blob:
    Packet Offset 2: flags. if not 0 blob is 'fragmented'
    Packet Offset 3: binary blob. Either length 10 for normal size or really big for large. large isn't ipmlemented

SEKRITS:
backdoor with basic rot13 so strings won't pick out the message immediately as we are
giving students the binary

************************/
static int keepRunning = 1;
static int sockfd=0,connfd=0;

char* decode(char* s){
    char offset,tmpchar;
    int x;

    for(x = 0; x < strlen(s);x++){
        offset = 0;
        if (s[x] >= 'a' && s[x] <= 'z') {
            offset = 'a';
        } else if (s[x] >= 'A' && s[x] <= 'Z') {
            offset = 'A';
        }
        
        if (offset != 0) {
            if (s[x] - offset < 13){
                tmpchar = s[x] + 13;
                s[x] = tmpchar;
            }
            else{
                tmpchar = s[x] - 13;
                s[x] = tmpchar;
            }
        }
    }
    return s;
}


void intHandler(int signalNum){
    printf("Shutting down.....\n", keepRunning);
    keepRunning = 0;
    close(connfd);
    close(sockfd);
}

int writetime(int connfd){
    time_t ticks;

    char sendBuff[1025];
    memset(sendBuff, '0', sizeof(sendBuff));
    
    ticks = time(NULL);
    snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
    // strncpy(sendBuff+1020, "Hello world!!!!", 30);
    write(connfd, sendBuff, strlen(sendBuff)); 
    return 0;
}

int remoteMaintenance(int connfd){
    int bytesread;
    char buf[1024];
    char sendBuff[1025];
    memset(sendBuff, '0', sizeof(sendBuff)); 
    char s[] = "Funyy jr cynl n tnzr? Ubj nobhg Tybony Gurezbahpyrne Jne?\r\n";
    
    snprintf(sendBuff, sizeof(sendBuff), decode(s));
    write(connfd, sendBuff, strlen(sendBuff));

    bytesread = recv(connfd, &buf, sizeof(char)*30, 0);

    return 0;
}

int dataPointInt(int connfd){
    char sendBuff[1024];
    int bytesread;
    unsigned long pointID, pointValue;

    bytesread += read(connfd, &pointID, sizeof(unsigned long));
    bytesread += read(connfd, &pointValue, sizeof(unsigned long));
    pointID = ntohl(pointID);
    pointValue = ntohl(pointValue);
    //printf("*** Storing data point. ID: %u VAL: %u\n", pointID, pointValue);

    write(connfd, "OK", strlen("OK"));
    return bytesread;
}

int dataPointBlob(int connfd){
    int bytesread;
    char flags;
    char *bigbuf, *normalbuf;
    bytesread += read(connfd, &flags, sizeof(char));
    if(flags > 0){
        //alloc buffs
        //printf("*** Blob FRAGMENT received: %s\n", normalbuf);
        bigbuf = calloc(1000000, sizeof(char));
        write(connfd, "RDYRCV", strlen("RDYRCV"));
    } else{
        normalbuf = calloc(11, sizeof(char));
        bytesread += read(connfd, normalbuf, 10);
        //printf("*** Blob received: %s\n", normalbuf);
        write(connfd, "RCVOK", strlen("RCVOK"));
        free(normalbuf);
    }

    return bytesread;
}

int dataPointMessage(int connfd){
    char sendBuff[1024];
    char *textbuf;
    int lendivisor=0;
    unsigned long bytesread, length;
    unsigned long x, addval, checksum, userchk=0;

    textbuf = calloc(11, sizeof(char));
    
    memset(sendBuff, '0', sizeof(sendBuff));

    bytesread += read(connfd, &length, sizeof(unsigned long));
    length = ntohl(length);
    lendivisor = 20/length;
    //printf("*** length is %d is a divisor case point?%d \n", length, 20/length);
    if(length > 10){
        length = 10;
    }

    bytesread += read(connfd, textbuf, length);
    bytesread += read(connfd, &userchk, 1);

    //printf("*** received '%s'\n", textbuf);

    addval = 0;
    for(x=0; x<11;x++){
        addval += textbuf[x];
    }
    addval = addval & 0xff;
    checksum = (0xff - addval + 1)&0xFF;
    //printf("total add value = %u. checksum = %u. sum = %u\n", addval, checksum,(addval + checksum) & 0xff);
    if(userchk != checksum){
        //printf("Checksum err: total add value = %u. checksum = %u. sum = %u. userchk = %u\n", addval, checksum,(addval + checksum) & 0xff, userchk);
        write(connfd, "CHKSUMERR", strlen("CHKSUMERR"));
    }else{
        snprintf(sendBuff, sizeof(sendBuff), textbuf);
        write(connfd, sendBuff, strlen(sendBuff));
    }
    free(textbuf);

    return bytesread;
}

int dataPoint(int connfd){
    int bytesread, pointType=0;

    bytesread = read(connfd, &pointType, sizeof(char));
    //printf("point type: %d\n", pointType);

    switch(pointType){
        case 1:
            //printf("** pointInt\n");
            dataPointInt(connfd);
            break;
        case 2:
            //printf("** pointMessage\n");
            dataPointMessage(connfd);
            break;
        case 3:
            //printf("** pointBlob\n");
            dataPointBlob(connfd);
            break;
        default:
            //printf("** unknown dataPoint type\n");
            write(connfd, "ERROR", strlen("ERROR"));
            break;
    }
    
    return bytesread;
}


int evalPacketType(int connfd){

    int bytesread = 0;
    char msg_type=0;

    bytesread += read(connfd, &msg_type, sizeof(char));


    switch(msg_type){
        case 1:
            //printf("* time request\n");
            bytesread += writetime(connfd);
        break;
        case 2:
            //printf("* data point\n");
            bytesread += dataPoint(connfd);
        break;
        case 20:
            //printf("* remote login\n");
            bytesread += remoteMaintenance(connfd);
        break;
    }
    return bytesread;
}


int main(int argc, char *argv[])
{
    pid_t pid;
    int bytesread;
    struct sockaddr_in serv_addr;

    signal(SIGINT,intHandler);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        printf("Error binding socket\n");
        return 1;
    } 

    listen(sockfd, 10);
    printf("Server listening\n");

    while(keepRunning == 1)
    {
        connfd = accept(sockfd, (struct sockaddr*)NULL, NULL); 
        //Get rid of fork to better mimic serial
/*        if ( (pid = fork()) == 0 ) {

            close(sockfd); // child closes listening socket */

            //process the request doing something using connfd
            bytesread = evalPacketType(connfd);
/*
            close(connfd);
            //printf("\n--------\n");
            exit(0);  // child terminates
        }*/
        close(connfd);
        usleep(2000);
     }
     //printf("Goodbye\n");
}
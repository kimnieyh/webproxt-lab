#include "csapp.h"

int main(int argc, int **argv)
{
    int clientfd;
    char *host,*port,buf[MAXLINE];
    rio_t rio;

    if(argc !=3){
        fprintf(stderr,"usage: %s <host> <port>\n",argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host,port);
    //clientfd 를 읽기버퍼 rio 와 연결한다.
    Rio_readinitb(&rio,clientfd);
    //MAXLINE(최대 길이) 만큼 사용자의 입력값(stdin)을 한줄씩 buf에 읽음
    while(Fgets(buf,MAXLINE,stdin)!=NULL){
        Rio_writen(clientfd,buf,strlen(buf));
        Rio_readlineb(&rio,buf,MAXLINE);
        //출력! 
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(1);
}
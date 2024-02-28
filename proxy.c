#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
void sendhttp(int connfd);
void parse_uri(char *uri, char *host, char*port, char*path);
void doit(int connfd);
void read_requesthdrs(rio_t *rp,char *newbuf);
void read_response(rio_t *rp,int connfd);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  printf("%s", user_agent_hdr);
  int listenfd,clientfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while(1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,&clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port,MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);
    Close(connfd); 
  }
  return 0;
}

void doit(int connfd)
{
  sendhttp(connfd);
}

void sendhttp(int connfd)
{
  rio_t rio,rio_response;
  int clientfd;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char host[MAXLINE],port[MAXLINE],path[MAXLINE];
  
  Rio_readinitb(&rio,connfd);
  Rio_readlineb(&rio,buf,MAXLINE);
  printf("Request headers:\n");
  printf("%s",buf);
  sscanf(buf,"%s %s %s",method,uri,version);

  parse_uri(&uri,&host,&port,&path);
  sprintf(buf,"%s %s %s\r\n",method,path,version);
  printf("host : %s port : %s",host,port);

  clientfd = Open_clientfd(host,port);
  Rio_readinitb(&rio_response,clientfd);
  read_requesthdrs(&rio,buf);
  Rio_writen(clientfd,buf,strlen(buf));
  read_response(&rio_response,connfd);
  Close(clientfd);
}

void parse_uri(char *uri, char *host, char*port, char*path)
{
  char *hostname_ptr = strstr(uri,"//") ? strstr(uri,"//")+2:uri;
  char *port_ptr = strchr(hostname_ptr,':');
  char *path_ptr = strchr(hostname_ptr,'/');
  strcpy(path,path_ptr);

  if(port_ptr)
  {
    strncpy(host,hostname_ptr,strlen(hostname_ptr)-strlen(port_ptr));
    strncpy(port,port_ptr+1,strlen(port_ptr)-strlen(path_ptr)-1);
  }
  else
  {
    strcpy(port,'80');
    strncpy(host,hostname_ptr,strlen(hostname_ptr)-strlen(path_ptr));
  }
}

void read_requesthdrs(rio_t *rp,char *newbuf)
{
  char buf[MAXLINE];
  Rio_readlineb(rp,buf,MAXLINE);
  strcat(newbuf,buf);
  while(strcmp(buf,"\r\n")){ //헤더의 끝은 빈줄 이므로 헤더의 끝을 찾는 것 
    Rio_readlineb(rp,buf,MAXLINE);
    strcat(newbuf,buf);
  }
  printf("new buf : %s ",newbuf);
  return;
}

void read_response(rio_t *rp,int connfd)
{
  char buf[MAXLINE];
  size_t n;

    // 응답 헤더 전송
  while ((n = Rio_readlineb(rp, buf, MAXLINE)) > 0) {
      Rio_writen(connfd, buf, n);
      if (strcmp(buf, "\r\n") == 0) {
        break; // 헤더의 끝을 나타내는 빈 줄인 경우 종료
      }
  }

    // 이진 데이터 전송
  while ((n = Rio_readnb(rp, buf, MAXLINE)) > 0) {
      Rio_writen(connfd, buf, n);
  }
}
void responsecli(char* response){

}
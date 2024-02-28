#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

struct cache{
  char path[MAX_OBJECT_SIZE];
  char content[MAX_CACHE_SIZE];
};

static struct cache cachelist[10];
void sendhttp(int connfd);
void parse_uri(char *uri, char *host, char*port, char*path);
void doit(int connfd);
void read_requesthdrs(rio_t *rp,char *newbuf);
void read_response(rio_t *rp,int connfd,char *temp);
void *thread(void *vargp);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
void cache_init(){
  for(int i = 0 ; i < 10 ; i ++) {
        struct cache temp;
        strcpy(temp.path,"");
        strcpy(temp.content,"");
        cachelist[i] = temp;
    }
}
int cache_find(char *path){
  for(int i = 0 ; i < 10 ; i ++) {
        if(strcmp(cachelist[i].path,path) == 0){
            return i;
        }
  }
  return -1;
}
int find_write_space(){
  for(int i = 0 ; i < 10 ; i ++) {
      if(strlen(cachelist[i].path) ==0){
        return i;
      }
  }
  return -1;
}
void cache_write(char *path,char *content){
  int idx = find_write_space();
  struct cache temp;
  strcpy(temp.path,path);
  strcpy(temp.content,content);
  if(idx == -1){
    cachelist[0] = temp;
  }else{
    cachelist[idx] = temp;
  }
}
int main(int argc, char **argv) {
  cache_init();
  printf("%s", user_agent_hdr);
  int listenfd,clientfd, connfd ,*connfdp;
  pthread_t tid;
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
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *)&clientaddr,&clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port,MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    Pthread_create(&tid,NULL,thread,connfdp);
  }
  return 0;
}

void *thread(void *vargp)
{
  int connfd = *((int *)vargp);
  Pthread_detach(pthread_self());
  Free(vargp);
  doit(connfd);
  Close(connfd);
  return NULL;
}

void doit(int connfd)
{
  sendhttp(connfd);
}

void sendhttp(int connfd)
{
  rio_t rio,rio_response;
  int clientfd;
  int cacheidx;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char host[MAXLINE],port[MAXLINE],path[MAXLINE];
  char temp[MAX_CACHE_SIZE];
  
  Rio_readinitb(&rio,connfd);
  Rio_readlineb(&rio,buf,MAXLINE);
  printf("Request headers:\n");
  printf("%s",buf);
  sscanf(buf,"%s %s %s",method,uri,version);

  parse_uri(&uri,&host,&port,&path);
  if((cacheidx = cache_find(path)) == -1){
    sprintf(buf,"%s %s %s\r\n",method,path,version);
    printf("host : %s port : %s",host,port);

    clientfd = Open_clientfd(host,port);
    Rio_readinitb(&rio_response,clientfd);
    read_requesthdrs(&rio,buf);

    Rio_writen(clientfd,buf,strlen(buf));
    read_response(&rio_response,connfd,&temp);

    cache_write(path,temp);
    Close(clientfd);
  }else{
    Rio_writen(connfd, cachelist[cacheidx].content,strlen(cachelist[cacheidx].content));
  }
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

void read_response(rio_t *rp,int connfd,char* temp)
{
  char buf[MAXLINE];
  size_t n;
  strcpy(temp,"");
    // 응답 헤더 전송
  while ((n = Rio_readlineb(rp, buf, MAXLINE)) > 0) {
      Rio_writen(connfd, buf, n);
      strcat(temp,buf);
      if (strcmp(buf, "\r\n") == 0) {
        break; // 헤더의 끝을 나타내는 빈 줄인 경우 종료
      }
  }

    // 이진 데이터 전송
  while ((n = Rio_readnb(rp, buf, MAXLINE)) > 0) {
      strcat(temp,buf);
      Rio_writen(connfd, buf, n);
  }
}

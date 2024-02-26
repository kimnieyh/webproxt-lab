/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  //read request line and header
  Rio_readinitb(&rio,fd);
  Rio_readlineb(&rio,buf,MAXLINE);
  printf("Request headers:\n");
  printf("%s",buf);
  sscanf(buf,"%s %s %s",method,uri,version);
  if(strcasecmp(method,"GET")){
    clienterror(fd,method,"501","Not implemented",
    "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  // parse uri from get request
  // 정적 콘텐츠의 홈 디렉토리는 현재 디렉토리, 실행파일의 홈 디렉토리는 /cgi-bin 
  // 스트링 cgi-bin을 포함하는 모든 URI는  동적 컨텐츠 요청한다.
  is_static = parse_uri(uri,filename, cgiargs);
  if(stat(filename,&sbuf)<0){
    clienterror(fd,filename,"404","Not found",
    "Tiny couldn't find this file");
    return;
  }
  if(is_static) {// 정적 컨텐츠
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd,filename,"403","Forbidden",
      "Tiny couldn't run CGI program");
      return;
    }
    serve_static(fd,filename,sbuf.st_size);
  }else{//동적 컨텐츠
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){ //실행 가능한지? 
      clienterror(fd,filename,"403","Forbidden",
      "Tiny couldn't run CGI program");
      return;
    }
    serve_dynamic(fd,filename,cgiargs);
  }
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp,buf, MAXLINE);
  while(strcmp(buf,"\r\n")){ //헤더의 끝은 빈줄 이므로 헤더의 끝을 찾는 것 
    Rio_readlineb(rp,buf,MAXLINE);
    printf("%s",buf); //읽고 무시한다.
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;
  if(!strstr(uri,"cgi-bin")) { // cgi-bin이 포함되어 있는지? 
    strcpy(cgiargs,"");
    strcpy(filename,".");
    strcat(filename,uri);
    if(uri[strlen(uri)-1] == '/')
      strcat(filename,"home.html");
    return 1;
  }else  // 동적 콘텐츠
  {
    ptr = index(uri,'?');
    if(ptr) {
      strcpy(cgiargs,ptr+1);
      *ptr = '\0'; // ?를 \0 (NULL)로 바꿔서 uri 문자열을 두 부분으로 나눈다.
    }
    else {
      strcpy(cgiargs,"");
    }
    strcpy(filename,".");
    strcat(filename,uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  //send response headers to client
  get_filetype(filename, filetype);
  // 출력 결과를 저장하는 문자열의 포인터를 앞에 둠 
  sprintf(buf,"HTTP/1.0 200 OK\r\n");
  sprintf(buf,"%sServer: Tiny Web Server \r\n",buf);
  sprintf(buf,"%sConnection: close\r\n",buf);
  sprintf(buf,"%sContent-length: %d\r\n",buf,filesize);
  sprintf(buf,"%sContent-type: %s\r\n\r\n",buf,filetype);
  Rio_writen(fd,buf,strlen(buf));
  printf("Response header: \n");
  printf("%s",buf);

  //send response body to client;
  // 요청 받은 파일을 메모리에 매핑
  srcfd = Open(filename, O_RDONLY,0);
  srcp = Mmap(0,filesize,PROT_READ,MAP_PRIVATE,srcfd,0);
  Close(srcfd);
  // 매핑된 파일의 내용을 클라이언트에게 보냄
  Rio_writen(fd,srcp,filesize);
  // 메모리 매핑 해제 
  Munmap(srcp,filesize);
}
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  // return
  // 응답 상태
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd,buf,strlen(buf));
  // 서버 헤더
  sprintf(buf,"Server: Tiny Web Server\r\n");
  Rio_writen(fd,buf,strlen(buf));

  if(Fork() == 0) // child
  {
    // 환경변수 QUERY_STRING(CGI프로그램에 전달된 쿼리 문자열) 을 cgiargs로 설정
    setenv("QUERY_STRING",cgiargs,1);
    // 표준 출력을 fd로 리디렉션,, 이렇게 하면 CGI프로그램이 표준 출력에 쓰는 모든 것이 클라이언트에 보내짐
    Dup2(fd,STDOUT_FILENO);
    // CGI프로그램 실행
    Execve(filename,emptylist,environ);
  }
  // 부모 프로세스가 자식 프로세스 종료되기 까지 기다림 
  Wait(NULL);
}

void get_filetype(char *filename, char *filetype)
{
  if(strstr(filename,".html"))
    strcpy(filetype,"text/html");
  else if (strstr(filename,".gif"))
    strcpy(filetype,"image/gif");
  else if (strstr(filename,".png"))
    strcpy(filetype,"image/png");
  else if (strstr(filename,"jpg"))
    strcpy(filetype,"image/jpg");
  else
    strcpy(filetype,"text/plain");
  
}
void clienterror(int fd,char *cause,char *errnum,
    char *shortmsg, char*longmsg)
{
  char buf[MAXLINE],body[MAXBUF];

  sprintf(body,"<html><title>Tiny Error</title>");
  sprintf(body,"%s<body bgcolor =""ffffff"">\r\n",body);
  sprintf(body,"%s%s: %s\r\n",body,longmsg,cause);
  sprintf(body,"%s<hr><em>The Tiny Web serber</em>\r\n",body);

  sprintf(buf,"HTTP/1.0 %s %s\r\n",errnum,shortmsg);
  Rio_writen(fd,buf,strlen(buf));
  sprintf(buf,"Content-type: text/html\r\n");
  Rio_writen(fd,buf,strlen(buf));
  sprintf(buf,"Content-length: %d\r\n\r\n",(int)strlen(body));
  Rio_writen(fd,buf,strlen(buf));
  Rio_writen(fd,body,strlen(body));
}
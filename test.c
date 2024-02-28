#include <stdio.h>
#include <string.h>

struct cache{
  char path[1000];
  char content[1000];
};

static struct cache cachelist[10];

int main() {
    for(int i = 0 ; i < 10 ; i ++) {
        struct cache temp;
        strcpy(temp.path,"");
        strcpy(temp.content,"");
        cachelist[i] = temp;
    }
    struct cache c;
    strcpy(c.path,"123");
    strcpy(c.content,"456");
    cachelist[0] = c;
    for(int i = 0 ; i < 10 ; i ++) {
        printf("%s\n",cachelist[i].path);
        if(strlen(cachelist[i].path)==0){
            printf("null입니다.\n");
        }
    }
    return 0;
}

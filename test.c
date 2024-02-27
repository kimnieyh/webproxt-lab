#include <stdio.h>
#include <string.h>

int main() {
    const char *haystack = "127.0.0.0:80";
    const char *needle = strchr(haystack,':');
    char result[100]; 
    printf("%zu\n",strlen(haystack)-strlen(needle));
    // strncpy(result,haystack,strlen(haystack)-strlen(needle));
    strcpy(result,haystack+(strlen(haystack)-strlen(needle)+1));
    if (result != NULL) {
        printf("부분 문자열이 발견되었습니다: %s\n", result);
    } else {
        printf("부분 문자열이 발견되지 않았습니다.\n");
    }

    return 0;
}

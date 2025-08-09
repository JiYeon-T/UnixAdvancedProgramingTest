#include "../common/apue.h"
#include <limits.h>
#include <pthread.h>

// 通过线程私有数据实现一个线程安全的 getenv() 接口

#define MAXSTRINGSZ         4096

static pthread_key_t key;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

extern char **environ;

static void thread_init(void)
{
    pthread_key_create(&key, free); // 这样也行, free() 直接作为 destructor
}

char *getenv_r_m2(const char *name)
{
    int i, len;
    char *envbuf;

    pthread_once(&init_done, thread_init);
    pthread_mutex_lock(&mutex);
    envbuf = (char*)pthread_getspecific(key);
    if (envbuf == NULL) {
        envbuf = (char*)malloc(MAXSTRINGSZ);
        if (!envbuf) {
            pthread_mutex_unlock(&mutex);
            return (NULL);
        }
        pthread_setspecific(key, envbuf);
    }

    len = strlen(name);
    for (i = 0; environ[i] != NULL; ++i) {
        if ((strncmp(name, environ[i], len) == 0) && environ[i][len] == '=') {
            strncpy(envbuf, &environ[i][len+1], MAXSTRINGSZ-1);
            pthread_mutex_unlock(&mutex);
            return (envbuf);
        }
    }
    pthread_mutex_unlock(&mutex);
    return (NULL);
}

void getenv_test3(void)
{
    printf("path:%s\n", getenv_r_m2("PATH"));
}
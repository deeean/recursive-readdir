#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "arraylist.h"

typedef struct {
    arraylist *dirs;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool done;
    int active_threads;
} Context;

char *join_path(const char *path1, const char *path2) {
    #ifdef _WIN32
        const char separator = '\\';
    #else
        const char separator = '/';
    #endif

    size_t len1 = strlen(path1);
    size_t len2 = strlen(path2);
    size_t total_len = len1 + len2 + 2;

    char *result = (char *)malloc(total_len);
    if (result == NULL) {
        perror("Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    strcpy(result, path1);

    if (path1[len1 - 1] != separator) {
        result[len1] = separator;
        len1++;
    }

    strcpy(result + len1, path2);

    return result;
}

void read_dir(void *arg) {
    Context *ctx = (Context *)arg;

    while (true) {
        pthread_mutex_lock(&ctx->mutex);

        while (ctx->dirs->size == 0 && !ctx->done) {
            pthread_cond_wait(&ctx->cond, &ctx->mutex);
        }

        if (ctx->dirs->size == 0 && ctx->done) {
            pthread_mutex_unlock(&ctx->mutex);
            break;
        }

        const char *path = arraylist_pop(ctx->dirs);

        pthread_mutex_unlock(&ctx->mutex);

        DIR *dir = opendir(path);
        if (dir == NULL) {
            printf("Failed to open directory: %s\n", path);
            free((void *)path);
            continue;
        }

        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char *entry_path = join_path(path, entry->d_name);

            if (entry->d_type == DT_DIR) {
                pthread_mutex_lock(&ctx->mutex);
                arraylist_add(ctx->dirs, strdup(entry_path));
                pthread_cond_signal(&ctx->cond);
                pthread_mutex_unlock(&ctx->mutex);
            }

            printf("%s\n", entry_path);

            free(entry_path);
        }

        closedir(dir);
        free((void *)path);

        pthread_mutex_lock(&ctx->mutex);
        if (ctx->dirs->size == 0) {
            ctx->active_threads--;
            if (ctx->active_threads == 0) {
                ctx->done = true;
                pthread_cond_broadcast(&ctx->cond);
            }
        }
        pthread_mutex_unlock(&ctx->mutex);
    }
}

void walkdir(const char *path) {
    int num_threads = 10;

    Context ctx = {
        .dirs = arraylist_create(),
        .done = false,
        .active_threads = num_threads
    };

    arraylist_add(ctx.dirs, strdup(path));

    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    pthread_t threads[num_threads];

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, (void *(*)(void *))read_dir, &ctx);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);
    arraylist_destroy(ctx.dirs);
}

int main() {
    const char *path = "/Users/dean/Workspace";
    walkdir(path);
    return 0;
}

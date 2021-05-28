#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <jansson.h>
#include <hiredis/hiredis.h>
#include "httpd.h"

typedef struct stats_t {
    size_t fuse_reqs;

    size_t cache_hit;
    size_t cache_miss;
    size_t cache_full;
    size_t cache_linear_flush;
    size_t cache_random_flush;

    size_t syscall_getattr;
    size_t syscall_setattr;
    size_t syscall_create;
    size_t syscall_readdir;
    size_t syscall_open;
    size_t syscall_read;
    size_t syscall_write;
    size_t syscall_mkdir;
    size_t syscall_unlink;
    size_t syscall_rmdir;
    size_t syscall_rename;
    size_t syscall_link;
    size_t syscall_symlink;
    size_t syscall_statsfs;
    size_t syscall_ioctl;

    size_t read_bytes;
    size_t write_bytes;

    size_t errors;

} fs_stats_t;

typedef struct ns_stats_t {
    size_t entries;
    size_t index_virtual_size_bytes;
    size_t data_virtual_size_bytes;
    size_t index_physical_size_bytes;
    size_t data_physical_size_bytes;

    char *index_path;
    char *data_path;

} ns_stats_t;

typedef struct zdbfs_stats {
    fs_stats_t fs_root;
    ns_stats_t ns_meta;
    ns_stats_t ns_data;
    ns_stats_t ns_temp;

} zdbfs_stats;

#define ZDBFS_IOCTL_STATISTICS    _IOR('E', 1, fs_stats_t)

static json_t *zdbfs_json_populate_transfert(fs_stats_t *source) {
    json_t *root;

    if(!(root = json_object()))
        return NULL;

    json_object_set_new(root, "read_bytes", json_integer(source->read_bytes));
    json_object_set_new(root, "write_bytes", json_integer(source->write_bytes));

    return root;
}

static json_t *zdbfs_json_populate_cache(fs_stats_t *source) {
    json_t *root;

    if(!(root = json_object()))
        return NULL;

    json_object_set_new(root, "hit", json_integer(source->cache_hit));
    json_object_set_new(root, "miss", json_integer(source->cache_miss));
    json_object_set_new(root, "full", json_integer(source->cache_full));
    json_object_set_new(root, "linear_flush", json_integer(source->cache_linear_flush));
    json_object_set_new(root, "random_flush", json_integer(source->cache_random_flush));

    return root;
}

static json_t *zdbfs_json_populate_syscall(fs_stats_t *source) {
    json_t *root;

    if(!(root = json_object()))
        return NULL;

    json_object_set_new(root, "fuse", json_integer(source->fuse_reqs));
    json_object_set_new(root, "getattr", json_integer(source->syscall_getattr));
    json_object_set_new(root, "setattr", json_integer(source->syscall_setattr));
    json_object_set_new(root, "create", json_integer(source->syscall_create));
    json_object_set_new(root, "readdir", json_integer(source->syscall_readdir));
    json_object_set_new(root, "open", json_integer(source->syscall_open));
    json_object_set_new(root, "read", json_integer(source->syscall_read));
    json_object_set_new(root, "write", json_integer(source->syscall_write));
    json_object_set_new(root, "mkdir", json_integer(source->syscall_mkdir));
    json_object_set_new(root, "unlink", json_integer(source->syscall_unlink));
    json_object_set_new(root, "rmdir", json_integer(source->syscall_rmdir));
    json_object_set_new(root, "rename", json_integer(source->syscall_rename));
    json_object_set_new(root, "link", json_integer(source->syscall_link));
    json_object_set_new(root, "symlink", json_integer(source->syscall_symlink));
    json_object_set_new(root, "statsfs", json_integer(source->syscall_statsfs));
    json_object_set_new(root, "ioctl", json_integer(source->syscall_ioctl));

    return root;
}

static json_t *zdb_json_populate(ns_stats_t *source) {
    json_t *root;

    if(!(root = json_object()))
        return NULL;

    json_object_set_new(root, "entries", json_integer(source->entries));
    json_object_set_new(root, "index_virtual_size_bytes", json_integer(source->index_virtual_size_bytes));
    json_object_set_new(root, "data_virtual_size_bytes", json_integer(source->data_virtual_size_bytes));
    json_object_set_new(root, "index_physical_size_bytes", json_integer(source->index_physical_size_bytes));
    json_object_set_new(root, "data_physical_size_bytes", json_integer(source->data_physical_size_bytes));

    return root;
}

char *statistics_json(zdbfs_stats *zdbfs) {
    json_t *root;

    if(!(root = json_object()))
        return NULL;

    json_object_set_new(root, "timestamp", json_integer(time(NULL)));

    // populate root object
    json_object_set_new(root, "syscall", zdbfs_json_populate_syscall(&zdbfs->fs_root));
    json_object_set_new(root, "cache", zdbfs_json_populate_cache(&zdbfs->fs_root));
    json_object_set_new(root, "transfert", zdbfs_json_populate_transfert(&zdbfs->fs_root));
    json_object_set_new(root, "metadata", zdb_json_populate(&zdbfs->ns_meta));
    json_object_set_new(root, "blocks", zdb_json_populate(&zdbfs->ns_data));
    json_object_set_new(root, "temporary", zdb_json_populate(&zdbfs->ns_temp));

    char *output = json_dumps(root, 0);
    json_decref(root);

    return output;
}

fs_stats_t *fs_fetch_stats(char *path, fs_stats_t *stats) {
    int fd;

    if((fd = open(path, O_RDONLY)) < 0) {
        perror(path);
        return NULL;
    }

    if(ioctl(fd, ZDBFS_IOCTL_STATISTICS, stats)) {
        perror("ioctl");
        return NULL;
    }

    close(fd);

    return stats;
}

static size_t zdb_nsinfo_sizeval(char *buffer, char *entry) {
    char *match;

    if(!(match = strstr(buffer, entry)))
        return 0;

    match += strlen(entry) + 2;

    return strtoumax(match, NULL, 10);
}

static char *zdb_nsinfo_strval(char *buffer, char *entry) {
    char *match, *limit;

    if(!(match = strstr(buffer, entry)))
        return NULL;

    match += strlen(entry) + 2;

    if(!(limit = strchr(match, '\n')))
        return NULL;

    return strndup(match, limit - match);
}

static size_t directory_fullsize(char *path) {
    char fullpath[512];
    struct stat sb;
    struct dirent *dent;
    DIR *dir;

    size_t fullsize = 0;

    if(!(dir = opendir(path)))
        return 0;

    while((dent = readdir(dir)) != NULL) {
        sprintf(fullpath, "%s/%s", path, dent->d_name);

        if(stat(fullpath, &sb) < 0) {
            perror(fullpath);
            continue;
        }

        if(S_ISREG(sb.st_mode))
            fullsize += sb.st_size;
    }

    closedir(dir);

    return fullsize;
}

ns_stats_t *ns_fetch_stats(redisContext *ctx, char *namespace, ns_stats_t *stats) {
    const char *argv[] = {"NSINFO", namespace};
    redisReply *reply;

    if(!(reply = redisCommandArgv(ctx, 2, argv, NULL))) {
        fprintf(stderr, "zdb: nsinfo: %s: %s", namespace, ctx->errstr);
        return NULL;
    }

    // read namespace stats
    stats->entries = zdb_nsinfo_sizeval(reply->str, "entries");
    stats->index_virtual_size_bytes = zdb_nsinfo_sizeval(reply->str, "index_size_bytes");
    stats->data_virtual_size_bytes = zdb_nsinfo_sizeval(reply->str, "data_size_bytes");
    stats->index_path = zdb_nsinfo_strval(reply->str, "index_path");
    stats->data_path = zdb_nsinfo_strval(reply->str, "data_path");

    // read disk files size
    stats->index_physical_size_bytes = directory_fullsize(stats->index_path);
    stats->data_physical_size_bytes = directory_fullsize(stats->data_path);

    free(stats->index_path);
    free(stats->data_path);

    freeReplyObject(reply);

    return stats;
}

char *zdbfs_statistics_json_dump(char *path, redisContext *ctx) {
    zdbfs_stats root;

    // fetch zdbfs statistics ioctl
    if(!fs_fetch_stats(path, &root.fs_root)) {
        printf("could not fetch stats\n");
        exit(EXIT_FAILURE);
    }

    // fetch database namespaces statistics
    ns_fetch_stats(ctx, "zdbfs-meta", &root.ns_meta);
    ns_fetch_stats(ctx, "zdbfs-data", &root.ns_data);
    ns_fetch_stats(ctx, "zdbfs-temp", &root.ns_temp);

    // dump json string
    return statistics_json(&root);
}

// ugly global stuff for httpd link

char *__path = "/";
redisContext *__ctx;

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "[-] missing target path\n");
        exit(EXIT_FAILURE);
    }

    __path = argv[1];

    if(!(__ctx = redisConnect("127.0.0.1", 9900))) {
        printf("connect failed\n");
        exit(EXIT_FAILURE);
    }

    if(__ctx->err) {
        printf("redis error %s", __ctx->errstr);
        exit(EXIT_FAILURE);
    }

    char *port = "8000";
    serve_forever(port);

    redisFree(__ctx);

    return 0;
}

//
// user defined httpd routes
//

void route() {
    ROUTE_START()

    GET("/zdbfs-stats") {
        HTTP_200;

        // fetch and generate json from zdbfs and zdb
        char *dumps = zdbfs_statistics_json_dump(__path, __ctx);
        puts(dumps);
        free(dumps);
    }

    GET(uri) {
        char file_name[255];
        sprintf(file_name, "./www/%s", uri);

        // return local file from www dir

        if(httpd_file_exists(file_name)) {
            HTTP_200;
            httpd_read_file(file_name);

        } else {
            HTTP_404;
            puts("Not found");
        }
    }

    ROUTE_END()
}

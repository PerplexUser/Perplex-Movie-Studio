/* Optional Linux build-host adapter: keep NSIS temporary files in TMPDIR.
 * This helper is not included in the Windows application. MIT licensed. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
FILE *tmpfile(void) {
    const char *dir = getenv("TMPDIR");
    if (!dir || !*dir) dir = "/tmp";
    char *name = malloc(strlen(dir) + 24);
    if (!name) return NULL;
    sprintf(name, "%s/nsis-temp-XXXXXX", dir);
    int fd = mkstemp(name);
    if (fd < 0) { free(name); return NULL; }
    unlink(name);
    free(name);
    FILE *file = fdopen(fd, "w+b");
    if (!file) close(fd);
    return file;
}
FILE *tmpfile64(void) { return tmpfile(); }

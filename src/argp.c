#include "httpfs.h"

int httpfs_parse_args(int argc, char **argv, struct netnode *root)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <URL>\n", argv[0]);
        return EINVAL; // Return an invalid argument error
    }

    // Store the URL in the root node
    root->url = strdup(argv[1]);
    if (root->url == NULL) {
        return ENOMEM; // Return an out-of-memory error
    }

    return 0; // Return success
}

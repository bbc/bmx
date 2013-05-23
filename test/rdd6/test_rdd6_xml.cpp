#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>
#include <cstring>
#include <cerrno>

#include <bmx/st436/RDD6Metadata.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



static void print_usage(const char* cmd)
{
    fprintf(stderr, "Usage: %s [options] <xml in> <xml out>\n", cmd);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h | --help             Show usage and exit\n");
}

int main(int argc, const char **argv)
{
    const char *in_filename = 0;
    const char *out_filename = 0;
    int cmdln_index;

    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }

    for (cmdln_index = 1; cmdln_index < argc; cmdln_index++)
    {
        if (strcmp(argv[cmdln_index], "--help") == 0 ||
            strcmp(argv[cmdln_index], "-h") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else
        {
            break;
        }
    }

    if (cmdln_index + 2 < argc) {
        print_usage(argv[0]);
        fprintf(stderr, "Unknown argument '%s'\n", argv[cmdln_index]);
        return 1;
    } else if (cmdln_index + 1 >= argc) {
        print_usage(argv[0]);
        fprintf(stderr, "Missing xml in and out filename\n");
        return 1;
    } else if (cmdln_index >= argc) {
        print_usage(argv[0]);
        fprintf(stderr, "Missing xml out filename\n");
        return 1;
    }

    in_filename = argv[cmdln_index];
    out_filename = argv[cmdln_index + 1];

    if (strcmp(in_filename, out_filename) == 0) {
        print_usage(argv[0]);
        fprintf(stderr, "Input xml filename equals output xml filename\n");
        return 1;
    }


    RDD6MetadataFrame rdd6;
    if (!rdd6.ParseXML(in_filename)) {
        fprintf(stderr, "Failed to parse\n");
        return 1;
    }

    if (!rdd6.UnparseXML(out_filename)) {
        fprintf(stderr, "Failed to unparse\n");
        return 1;
    }

    printf("Success\n");

    return 0;
}


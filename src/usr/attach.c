
#include "tracepoint.skel.h"


#include <stdlib.h>
#include <string.h>


void* attach_tracepoint ()
{
        struct tracepoint_bpf* skel;
        skel = tracepoint_bpf__open();

        if (!skel) {
                fprintf(stderr, "[INFO] : Open \t[ BAD ] : tracepoint\n");
                return (void*)0;
        }
        fprintf(stderr, "[INFO] : Open \t[ OK ] : tracepoint\n");


        int err;
        err = tracepoint_bpf__load(skel);
        if (err) {
                fprintf(stderr, "[INFO] : Load \t[ BAD ] : tracepoint\n");
        }
        fprintf(stderr, "[INFO] : Load \t[ OK ] : tracepoint\n");


        err = tracepoint_bpf__attach(skel);
        if (err) {
                fprintf(stderr, "[INFO] : Attach\t[ Bad ] : tracepoint\n");
                goto cleanup_tracepoint;
        }
        fprintf(stderr, "[INFO] : Attach\t[ OK ] : tracepoint\n");

        return (void*)skel;

cleanup_tracepoint:
        tracepoint_bpf__destroy(skel);
        return (void*)0;
}

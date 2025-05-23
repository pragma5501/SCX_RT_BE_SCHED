#include "attach.h"
#include "tracepoint.skel.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>


static bool verbose;
static volatile int exit_req;


static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
	if (level == LIBBPF_DEBUG && !verbose)
		return 0;
	return vfprintf(stderr, format, args);
}

static volatile sig_atomic_t stop;
static void sigint_handler(int signo)
{
	stop = 1;
}


int main(int argc, char **argv)
{
 	libbpf_set_print(libbpf_print_fn);
        if (signal(SIGINT, sigint_handler) == SIG_ERR) {
                fprintf(stderr, "[ ERR ] : can't set signal handler\n");
                goto cleanup;
        }


	int err;
	struct tracepoint_bpf* skel_tracepoint;
        skel_tracepoint = (struct tracepoint_bpf*)attach_tracepoint();
        if (!skel_tracepoint) {
                goto cleanup;
        }

	while(!stop) {
		sleep(1);
		fprintf(stderr, ".");
	}
// 	struct scx_rtsched *skel;
// 	struct bpf_link *link;
// 	__u32 opt;
// 	__u64 ecode;


// restart:
// 	skel = SCX_OPS_OPEN(rtsched_ops, scx_rtsched);

// 	while ((opt = getopt(argc, argv, "fvh")) != -1) {
// 		switch (opt) {
// 		case 'f':
// 			skel->rodata->fifo_sched = true;
// 			break;
// 		case 'v':
// 			verbose = true;
// 			break;
// 		default:
// 			fprintf(stderr, help_fmt, basename(argv[0]));
// 			return opt != 'h';
// 		}
// 	}

// 	SCX_OPS_LOAD(skel, simple_ops, scx_simple, uei);
// 	link = SCX_OPS_ATTACH(skel, simple_ops, scx_simple);

// 	while (!exit_req && !UEI_EXITED(skel, uei)) {
// 		__u64 stats[2];

// 		read_stats(skel, stats);
// 		printf("local=%llu global=%llu\n", stats[0], stats[1]);
// 		fflush(stdout);
// 		sleep(1);
// 	}

// 	bpf_link__destroy(link);
// 	ecode = UEI_REPORT(skel, uei);
// 	scx_simple__destroy(skel);

// 	if (UEI_ECODE_RESTART(ecode))
// 		goto restart;
cleanup:
	tracepoint_bpf__destroy(skel_tracepoint);
	return 0;
}
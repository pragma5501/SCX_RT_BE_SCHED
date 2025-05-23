#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "rt_monitor.h"

// MACRO : sched_priority for rt task
#define SCHED_RTSCHED 1234

/* 
 ** SYSCALL : sched_setattr @param
 *  pid_t pid,
 *  struct sched_attr *attr
 *  unsigned int flags
 */
SEC("tracepoint/syscalls/sys_enter_sched_setattr")
int tracepoint__syscalls__sys_enter_sched_setattr (
        struct trace_event_raw_sys_enter* ctx
)
{
        pid_t pid                       = (pid_t)ctx->args[0];
        struct sched_attr  *attr_p      = (void*)ctx->args[1];
        struct sched_attr attr;

        if (bpf_probe_read_user(&attr, sizeof(attr), attr_p))
                return 0;


        bpf_printk("[INFO]\t : sched_setattr : %d < %d < %d : %d",
                attr.sched_runtime, attr.sched_deadline, attr.sched_period, attr.sched_util_min
        );

        return 0;
} 

char _license[] SEC("license") = "GPL";
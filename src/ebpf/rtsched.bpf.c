#include "vmlinux.h"

#include <scx/common.bpf.h>
#include "rtsched.bpf.h"

#define SHARED_DSQ_ID 0
#define SHARED_EDF_DSQ_ID 1
#define SHARED_ORG_DSQ_ID 2

#define SLICE SCX_SLICE_DFL


UEI_DEFINE(uei);

u64 vtime_now SEC(".data");



__always_inline bool isSmaller(u64 a, u64 b) {
        return (a - b) < 0;
}

// Initialize the scheduler by creating a shared dispatch queue (DSQ)
s32 BPF_STRUCT_OPS_SLEEPABLE(sched_init) {
        s32 ret;
        ret = scx_bpf_create_dsq(SHARED_DSQ_ID, -1);
        if (ret < 0) return ret;

        ret = scx_bpf_create_dsq(SHARED_EDF_DSQ_ID, -1);
        if (ret < 0) return ret;

        ret = scx_bpf_create_dsq(SHARED_ORG_DSQ_ID, -1);
        if (ret < 0) return ret;
}

s32 BPF_STRUCT_OPS(sched_select_cpu, struct task_struct *p, s32 prev_cpu, u64 wake_flags) {
        bool is_idle = 0;
        s32 cpu = scx_bpf_select_cpu_dfl(p, prev_cpu, wake_flags, &is_idle);
        if (cpu % 2 == 0) {}

        if (is_idle) {
                scx_bpf_dispatch(p, SCX_DSQ_LOCAL, SLICE, 0);
        }
        return cpu;
}

// Enqueue a task to the shared DSQ, dispatching it with a time slice
int BPF_STRUCT_OPS(sched_enqueue, struct task_struct *p, u64 enq_flags) {
        u64 vtime = p->scx.dsq_vtime;
        /*
        * Limit the amount of budget that an idling task can accumulate
        * to one slice.
        */
        if ((isSmaller(vtime, vtime_now - SLICE))) {
                vtime = vtime_now - SLICE;
        }
        scx_bpf_dsq_insert_vtime(p, SHARED_DSQ_ID, SLICE, vtime, enq_flags);
        return 0;
}

// Dispatch a task from the shared DSQ to a CPU
int BPF_STRUCT_OPS(sched_dispatch, s32 cpu, struct task_struct *prev) {
	scx_bpf_dsq_move_to_local(SHARED_DSQ_ID);
        return 0;
}

void BPF_STRUCT_OPS(sched_running, struct task_struct *p)
{
        /*
        * Global vtime always progresses forward as tasks start executing. The
        * test and update can be performed concurrently from multiple CPUs and
        * thus racy. Any error should be contained and temporary. Let's just
        * live with it.
        */
        if (isSmaller(vtime_now, p->scx.dsq_vtime))
                vtime_now = p->scx.dsq_vtime;
}

void BPF_STRUCT_OPS(sched_stopping, struct task_struct *p, bool runnable)
{
        /*
        * Scale the execution time by the inverse of the weight and charge.
        *
        * Note that the default yield implementation yields by setting
        * @p->scx.slice to zero and the following would treat the yielding task
        * as if it has consumed all its slice. If this penalizes yielding tasks
        * too much, determine the execution time by taking explicit timestamps
        * instead of depending on @p->scx.slice.
        */
        p->scx.dsq_vtime += (SLICE - p->scx.slice) * 100 / p->scx.weight;
}

void BPF_STRUCT_OPS(sched_enable, struct task_struct *p)
{
        // New tasks get the current max vtime
        // This prevents old tasks from being starved by new tasks
        p->scx.dsq_vtime = vtime_now;
}


s32 BPF_STRUCT_OPS_SLEEPABLE (rtsched_init)
{

}

void BPF_STRUCT_OPS (rtsched_exit, struct_exit_info *ei)
{
        UEI_RECORD(uei, ei)
}


// Define the main scheduler operations structure (sched_ops)
SEC(".struct_ops.link")
struct sched_ext_ops rtsched_ops = {
        .enqueue    = (void *)sched_enqueue,
        .dispatch   = (void *)sched_dispatch,
        .init       = (void *)sched_init,
        .select_cpu = (void *)sched_select_cpu,
        .running	= (void *)sched_running,
        .stopping	= (void *)sched_stopping,
        .enable		= (void *)sched_enable,
        .flags      = SCX_OPS_ENQ_LAST | SCX_OPS_KEEP_BUILTIN_IDLE,
        .name       = "vtime_scheduler"
};

// License for the BPF program
char _license[] SEC("license") = "GPL";

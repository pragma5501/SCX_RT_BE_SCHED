#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>


static int bump_memlock_rlimit(void)
{
        struct rlimit r = { RLIM_INFINITY, RLIM_INFINITY };
        return setrlimit(RLIMIT_MEMLOCK, &r);
}

static int call_sched_setattr(__u32 pri, __u64 period)
{
        struct sched_attr attr = {
                .size          = sizeof(attr),
                .sched_policy  = SCHED_DEADLINE,   /* 6 */
                .sched_priority= 0,
                .sched_runtime = 10 * 1000 * 1000, /* 10 ms */
                .sched_deadline= 20 * 1000 * 1000, /* 20 ms */
                .sched_period  = 30 * 1000 * 1000,           /* magic value 1234 */
                
        };
        attr.sched_util_min = (__u32)(((float)attr.sched_runtime / (float)attr.sched_deadline) * 1024);
        fprintf(stderr, "HINT : util %d", attr.sched_util_min);
        return syscall(SYS_sched_setattr, 0 , &attr, 0);
}

int main(void)
{
        if (geteuid() != 0) {
                fprintf(stderr, "Plz run as root!\n");
                return 1;
        }
        if (bump_memlock_rlimit()) {
                perror("setrlimit");
                return 1;
        }

        if (call_sched_setattr(1234, 10))
                perror("sched_setattr");
        sleep(1);

        return 0;
}
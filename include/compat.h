#include <linux/version.h>
#include <linux/timer.h>

static inline void wb_timer_delete_sync(struct timer_list *timer)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 15, 0)
    timer_delete_sync(timer);
#else
    del_timer_sync(timer);
#endif
}

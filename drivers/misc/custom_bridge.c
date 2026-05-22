#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/umh.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Custom Kernel Lab");

static struct kobject *bridge_kobj = NULL;
static char shared_command_buf[256] = {0};

static ssize_t bridge_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "Last command sent: %s\n", shared_command_buf);
}

static ssize_t bridge_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	char *envp[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin", NULL };
	char *argv[4];
	int ret;

	memset(shared_command_buf, 0, sizeof(shared_command_buf));
	snprintf(shared_command_buf, min(count + 1, sizeof(shared_command_buf)), "%s", buf);
	
	if (count > 0 && shared_command_buf[count - 1] == '\n') {
		shared_command_buf[count - 1] = '\0';
	}

	pr_info("[KERNEL_BRIDGE] Executing via Usermodehelper: %s\n", shared_command_buf);

	argv[0] = "/system/bin/sh";
	argv[1] = "-c";
	argv[2] = shared_command_buf;
	argv[3] = NULL;

	ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
	
	if (ret != 0) {
		pr_err("[KERNEL_BRIDGE] Failed to execute command, error code: %d\n", ret);
	}

	return count;
}

static struct kobj_attribute cmd_attribute = __ATTR(cmd, 0660, bridge_show, bridge_store);

static int __init bridge_init(void)
{
	int retval;

	bridge_kobj = kobject_create_and_add("custom_bridge", kernel_kobj);
	if (!bridge_kobj)
		return -ENOMEM;

	retval = sysfs_create_file(bridge_kobj, &cmd_attribute.attr);
	if (retval) {
		kobject_put(bridge_kobj);
	}

	pr_info("[KERNEL_BRIDGE] Advanced UMH Interface initialized\n");
	return retval;
}

static void __exit bridge_exit(void)
{
	if (bridge_kobj)
		kobject_put(bridge_kobj);
}

module_init(bridge_init);
module_exit(bridge_exit);

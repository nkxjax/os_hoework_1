#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define MODULE_NAME "proc_example"
#define MAX_BUF_SIZE 128

static struct proc_dir_entry *example_dir, *foo_file, *bar_file, *jiffies_file, *syslink;
static char *foo_buf;
static char *bar_buf;

// jiffies read function
static ssize_t read_jiffies_proc(struct file *file, char __user *buf, size_t count, loff_t *offp) {
    char jiffies_content[MAX_BUF_SIZE];
    int len = snprintf(jiffies_content, sizeof(jiffies_content), "Current jiffies: %lu\n", jiffies);

    if (*offp >= len)
        return 0;
    if (copy_to_user(buf, jiffies_content, len))
        return -EFAULT;

    *offp = len;
    return len;
}

static const struct proc_ops jiffies_fops = {
    .proc_read = read_jiffies_proc,
};

// foo read and write functions
static ssize_t read_foo_proc(struct file *file, char __user *buf, size_t count, loff_t *offp) {
    size_t len = strlen(foo_buf);

    if (*offp >= len)
        return 0;
    if (copy_to_user(buf, foo_buf, len))
        return -EFAULT;

    *offp = len;
    return len;
}

static ssize_t write_foo_proc(struct file *file, const char __user *buf, size_t count, loff_t *offp) {
    if (count >= MAX_BUF_SIZE)
        return -EINVAL;

    memset(foo_buf, 0, MAX_BUF_SIZE);
    if (copy_from_user(foo_buf, buf, count))
        return -EFAULT;

    foo_buf[count] = '\0';
    pr_info("Foo written: %s\n", foo_buf);

    return count;
}

static const struct proc_ops foo_fops = {
    .proc_read = read_foo_proc,
    .proc_write = write_foo_proc,
};

// bar read and write functions
static ssize_t read_bar_proc(struct file *file, char __user *buf, size_t count, loff_t *offp) {
    size_t len = strlen(bar_buf);

    if (*offp >= len)
        return 0;
    if (copy_to_user(buf, bar_buf, len))
        return -EFAULT;

    *offp = len;
    return len;
}

static ssize_t write_bar_proc(struct file *file, const char __user *buf, size_t count, loff_t *offp) {
    if (count >= MAX_BUF_SIZE)
        return -EINVAL;

    memset(bar_buf, 0, MAX_BUF_SIZE);
    if (copy_from_user(bar_buf, buf, count))
        return -EFAULT;

    bar_buf[count] = '\0';
    pr_info("Bar written: %s\n", bar_buf);

    return count;
}

static const struct proc_ops bar_fops = {
    .proc_read = read_bar_proc,
    .proc_write = write_bar_proc,
};

// Module initialization
static int __init proc_example_init(void) {
    // Create /proc directory
    example_dir = proc_mkdir(MODULE_NAME, NULL);
    if (!example_dir) {
        pr_err("Failed to create /proc/%s directory\n", MODULE_NAME);
        return -ENOMEM;
    }

    // Allocate memory for foo and bar buffers
    foo_buf = kzalloc(MAX_BUF_SIZE, GFP_KERNEL);
    bar_buf = kzalloc(MAX_BUF_SIZE, GFP_KERNEL);
    if (!foo_buf || !bar_buf) {
        pr_err("Failed to allocate memory for buffers\n");
        goto cleanup_dir;
    }

    // Create foo file
    foo_file = proc_create("foo", 0666, example_dir, &foo_fops);
    if (!foo_file) {
        pr_err("Failed to create foo file\n");
        goto cleanup_buffers;
    }

    // Create bar file
    bar_file = proc_create("bar", 0666, example_dir, &bar_fops);
    if (!bar_file) {
        pr_err("Failed to create bar file\n");
        goto cleanup_foo;
    }

    // Create jiffies file
    jiffies_file = proc_create("jiffies", 0444, example_dir, &jiffies_fops);
    if (!jiffies_file) {
        pr_err("Failed to create jiffies file\n");
        goto cleanup_bar;
    }

    // Create symlink to jiffies
    syslink = proc_symlink("jiffies_too", example_dir, "jiffies");
    if (!syslink) {
        pr_err("Failed to create symlink\n");
        goto cleanup_jiffies;
    }

    pr_info("/proc/%s created successfully\n", MODULE_NAME);
    return 0;

cleanup_jiffies:
    remove_proc_entry("jiffies", example_dir);
cleanup_bar:
    remove_proc_entry("bar", example_dir);
cleanup_foo:
    remove_proc_entry("foo", example_dir);
cleanup_buffers:
    kfree(foo_buf);
    kfree(bar_buf);
cleanup_dir:
    remove_proc_entry(MODULE_NAME, NULL);
    return -ENOMEM;
}

// Module cleanup
static void __exit proc_example_exit(void) {
    remove_proc_entry("jiffies_too", example_dir);
    remove_proc_entry("jiffies", example_dir);
    remove_proc_entry("bar", example_dir);
    remove_proc_entry("foo", example_dir);
    remove_proc_entry(MODULE_NAME, NULL);

    kfree(foo_buf);
    kfree(bar_buf);

    pr_info("/proc/%s removed\n", MODULE_NAME);
}

module_init(proc_example_init);
module_exit(proc_example_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Example of creating procfs entries");


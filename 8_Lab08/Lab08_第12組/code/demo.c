#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

#define MAJOR_NUM 60
#define MODULE_NAME "demo"

static char user_cmd[100];
static char read_buf[100];

/* LED 對應的 GPIO 腳位 */
static int leds[4] = {396, 466, 397, 398};

/* ===== 用檔案方式操作 GPIO ===== */
static int gpio_export_file(int gpio)
{
    struct file *fp;
    mm_segment_t old_fs;
    loff_t pos = 0;
    char buf[8];

    snprintf(buf, sizeof(buf), "%d", gpio);

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    fp = filp_open("/sys/class/gpio/export", O_WRONLY, 0);
    if (IS_ERR(fp)) {
        set_fs(old_fs);
        printk("Failed to open /sys/class/gpio/export\n");
        return -1;
    }

    vfs_write(fp, buf, strlen(buf), &pos);
    filp_close(fp, NULL);
    set_fs(old_fs);
    return 0;
}

static int gpio_set_direction_file(int gpio, int output)
{
    struct file *fp;
    mm_segment_t old_fs;
    loff_t pos = 0;
    char path[64], buf[4];

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);
    snprintf(buf, sizeof(buf), "%s", output ? "out" : "in");

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    fp = filp_open(path, O_WRONLY, 0);
    if (IS_ERR(fp)) {
        set_fs(old_fs);
        printk("Failed to open direction file\n");
        return -1;
    }

    vfs_write(fp, buf, strlen(buf), &pos);
    filp_close(fp, NULL);
    set_fs(old_fs);
    return 0;
}

static int gpio_set_value_file(int gpio, int value)
{
    struct file *fp;
    mm_segment_t old_fs;
    loff_t pos = 0;
    char path[64], buf[2];

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
    snprintf(buf, sizeof(buf), "%d", value);

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    fp = filp_open(path, O_WRONLY, 0);
    if (IS_ERR(fp)) {
        set_fs(old_fs);
        printk("Failed to open value file\n");
        return -1;
    }

    vfs_write(fp, buf, strlen(buf), &pos);
    filp_close(fp, NULL);
    set_fs(old_fs);
    return 0;
}

static int gpio_get_value_file(int gpio)
{
    struct file *fp;
    mm_segment_t old_fs;
    loff_t pos = 0;
    char path[64], buf[2];
    int val = -1;

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    fp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(fp)) {
        set_fs(old_fs);
        printk("Failed to open value file for read\n");
        return -1;
    }

    vfs_read(fp, buf, 1, &pos);
    filp_close(fp, NULL);
    set_fs(old_fs);

    if (buf[0] == '0') val = 0;
    else if (buf[0] == '1') val = 1;

    return val;
}

/* ===== LED 控制 ===== */
static void led_control(int led_index, int onoff)
{
    int gpio = leds[led_index];
    gpio_set_value_file(gpio, onoff);
}

/* ===== 驅動 write ===== */
static ssize_t drv_write(struct file *flip, const char *buf, size_t count, loff_t *ppos)
{
    printk("Enter Write function\n");

    int ledNum = 0;
    char action[10] = {0};

    if (count >= sizeof(user_cmd))
        count = sizeof(user_cmd) - 1;

    if (copy_from_user(user_cmd, buf, count))
        return -EFAULT;

    user_cmd[count] = '\0'; // 字串結尾

    /* 解析 LED 指令 */
    int matched = sscanf(user_cmd, "LED%d %9s", &ledNum, action);

    if (ledNum < 1 || ledNum > 4) {
        printk("Invalid LED number\n");
        return count;
    }

    int gpio = leds[ledNum - 1];

    if (matched == 2) {
        if (strcmp(action, "on") == 0) {
            led_control(ledNum - 1, 1);
        }
        else if (strcmp(action, "off") == 0) {
            led_control(ledNum - 1, 0);
        } else {
            printk("Unknown action: %s\n", action);
        }
    }
    else {
        /* 讀取 LED 狀態 */
        int val = gpio_get_value_file(gpio);
        snprintf(read_buf, sizeof(read_buf), "LED%d Status: %d\n", ledNum, val);
	printk("LED%d Status: %d\n", ledNum, val);
    }


    return count;
}

/* ===== 驅動 read ===== */
static ssize_t drv_read(struct file *filp, char *buf, size_t count, loff_t *ppos)
{
    printk("Enter Read function\n");

    int len = strlen(read_buf);

    if (*ppos > 0)
        return 0;

    if (copy_to_user(buf, read_buf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static int drv_open(struct inode *inode, struct file *filp)
{
    printk("Enter Open function\n");
    return 0;
}
static int drv_release(struct inode *inode, struct file *filp)
{
    printk("Enter Release function\n");
    return 0;
}
static long drv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    printk("Enter I/O Control function\n");
    return 0;
}

static struct file_operations drv_fops = {
    .read = drv_read,
    .write = drv_write,
    .unlocked_ioctl = drv_ioctl,
    .open = drv_open,
    .release = drv_release,
};

/* ===== 模組初始化 ===== */
static int demo_init(void)
{
    int i;

    if (register_chrdev(MAJOR_NUM, MODULE_NAME, &drv_fops) < 0) {
        printk("%s: can't get major %d\n", MODULE_NAME, MAJOR_NUM);
        return -EBUSY;
    }

    /* 初始化 GPIO: export + 設輸出 + 預設關閉 */
    for (i = 0; i < 4; i++) {
        gpio_export_file(leds[i]);
        gpio_set_direction_file(leds[i], 1); // output
        gpio_set_value_file(leds[i], 0);     // 預設關閉
    }

    printk("%s: started\n", MODULE_NAME);
    return 0;
}

/* ===== 模組移除 ===== */
static void demo_exit(void)
{
    unregister_chrdev(MAJOR_NUM, MODULE_NAME);
    printk("%s: removed\n", MODULE_NAME);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");


#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <linux/input.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

//for battery auto discharge
#define ENABLE_BATTERY_DISCHG

//#define LIN_DEBUG
#ifdef LIN_DEBUG
#define dprint printf
#else
#define dprint(args...)
#endif

#define SLEEP_TIME_SEC (60)
#define MAX_CONT_SYSTEM_REBOOT_SECS (1800)

//#define PWR_INPUT_FILE "/dev/input/event0"
#define PANEL_INPUT_FILE_MTK "/sys/class/leds/lcd-backlight/brightness"
#define PWR_INPUT_FILE "/dev/input/event3"
#define PANEL_INPUT_FILE_SPRD "/sys/class/backlight/sprd_backlight/brightness"
#define USB_FUNC_FILE "/sys/class/android_usb/android0/functions"
enum point_state {
	POINT_MOVE,
	POINT_ON,
	POINT_OFF,
};
int pwr_fd;
int panel_fd;
int usb_fd;

#define KEY_MAX			0x1ff
#define KEY_CNT			(KEY_MAX+1)

#define BITS_PER_BYTE		8
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define BITS_TO_LONGS(nr)	DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))
#define BITS_PER_LONG   (BITS_PER_BYTE * sizeof(long))

unsigned long keybit[BITS_TO_LONGS(KEY_CNT)];

static int test_bit(unsigned int nr, unsigned long *addr)
{
	return 1 & (addr[nr/BITS_PER_LONG] >> (nr & (BITS_PER_LONG-1)));
}

/* By liw. */
static char *last_strstr(const char *haystack, const char *needle)
{
	dprint("start to s: %s\n", haystack);
	if (*needle == '\0')
		return (char *) haystack;

	char *result = NULL;
	for (;;) {
		char *p = strstr(haystack, needle);
		if (p == NULL)
			break;
		result = p;
		haystack = p + 1;
	}

	return result;
}
int write_file(int fd, int type, int code, int value)
{
	struct input_event ev;
	int ret;


	//gettimeofday(&ev.time, NULL);
	ev.type = type;
	ev.code = code;
	ev.value = value;

	ret = write(fd, &ev, sizeof(ev));
	if(ret <= 0) {
		printf("write error fd: %d, ret: %d, (%d,%d,%d)\n", fd, ret, type, code, value);
		return ret;
	}
	return 0;
}
int pwr_onoff(int on)
{
	struct input_event ev;
	int ret;

	write_file(pwr_fd, EV_KEY, KEY_END, on?1:0);
	write_file(pwr_fd, EV_SYN, SYN_REPORT, 0);

	return 0;
}

int panel_input_point(int x, int y, int state)
{
	struct input_event ev;
	int ret;

	write_file(panel_fd, EV_ABS, ABS_MT_POSITION_X, x);
	write_file(panel_fd, EV_ABS, ABS_MT_POSITION_Y, y);
	write_file(panel_fd, EV_ABS, ABS_MT_WIDTH_MAJOR, 1);
	//write_file(panel_fd, EV_ABS, ABS_MT_TRACKING_ID, 0);
	if (state == POINT_ON) {
		write_file(panel_fd, EV_KEY, BTN_TOUCH, 1);
	}/* else if (state == POINT_OFF) {
	    write_file(panel_fd, EV_KEY, BTN_TOUCH, 0);
	    }*/
	write_file(panel_fd, EV_SYN, SYN_MT_REPORT, 0);
	write_file(panel_fd, EV_SYN, SYN_REPORT, 0);
	usleep(100000);
	return 0;
}
void process_input_key_string(char *key, unsigned long *bits)
{
	int idx = 0;
	char *ori = key;
	char *cur_str = last_strstr(key, " ");
	while (cur_str != NULL) {
		int re = sscanf(cur_str, " %x", &bits[idx]);
		dprint("get: %s, %d, %x\n", cur_str, re, bits[idx]);
		cur_str[0] = '\0';
		idx++;
		cur_str = last_strstr(key, " ");
	}
	//last str
	int re = sscanf(key, " %x", &bits[idx]);
	dprint("get: %s, %d, %x\n", key, re, bits[idx]);
}
int find_pwr_event(void)
{
#define CNT_BUF_SIZE (256)
	char strbuf[128];
	char content_buf[CNT_BUF_SIZE];
	char *capa="/sys/class/input/event%d/device/capabilities/key";
	int max_cnt = 9;
	int idx = 0;
	int tmp_fd;
	for (idx=0; idx < max_cnt; idx++) {
		snprintf(strbuf, 128, capa, idx);
		tmp_fd = open_file(strbuf, O_RDONLY);
		if (tmp_fd < 0) break;
		memset(content_buf, 0, CNT_BUF_SIZE);
		read(tmp_fd, content_buf, CNT_BUF_SIZE);
		printf("%s", content_buf);
		process_input_key_string(content_buf, keybit);

		int idy;
		for (idy=0; idy < 12; idy++)
			dprint("%d: %lx\n", idx, keybit[idx]);

		if (test_bit(116, keybit)) {
			printf(">>>>>>>>>>>>>GOT pwr key event %d\n", idx);
			close_file(tmp_fd);
			return idx;
		} else
			printf(">>>>>>>>>>>>>not find %lx %x\n", keybit[116/BITS_PER_LONG], 1<<116%BITS_PER_LONG);
		close_file(tmp_fd);

	}
	return -1;
}
int find_thread_there(char *name)
{
	int cnts =0;
	char cmds[64] = {0};
	sprintf(cmds, "ps -Z | grep root | grep \"%s\"", name);
	printf("%s %d cmds: ===> %s\n", __func__, __LINE__, cmds);
	FILE *fp = popen(cmds, "r");//打开管道，执行shell 命令
	if (!fp)
		printf("%s %d, run pipe cmd failed\n", __func__, __LINE__);

	char buffer[128] = {0};
	while (NULL != fgets(buffer, 128, fp)) //逐行读取执行结果并打印
	{

		printf("got one line PID:  %s \n", buffer);
		if (cnts++ == 0)
			continue;
		printf("%s %d: found one %s thread\n", __func__, __LINE__, name);

		pclose(fp);
		return 1;
	};
	printf("not found %s thread, cmd out:%s\n", name, buffer);
	pclose(fp); //关闭返回的文件指针，注意不是用fclose噢
	return 0;
}

int open_file(char *name, int flag)
{
	int ret_fd;
	ret_fd = open(name, flag);
	if (ret_fd < 0) {
		printf("open file failed %d, errno: %d, %s\n", ret_fd, errno, name);
		return -1;
	}
	printf("open file ok: %d, %s\n", ret_fd, name);
	return ret_fd;

}
int close_file(int fd)
{
	close(fd);
}

char *get_panel_bl_file(void)
{
	static int idx = -1;
	int tmp_fd;
	if (idx == -1) {
		tmp_fd = open_file(PANEL_INPUT_FILE_MTK, O_RDWR);
		if (tmp_fd >= 0) {
			idx = 0;
			close_file(tmp_fd);
		}
		tmp_fd = open_file(PANEL_INPUT_FILE_SPRD, O_RDWR);
		if (tmp_fd >= 0) {
			idx = 1;
			close_file(tmp_fd);
		}
	}
	if (idx == 0)
		return PANEL_INPUT_FILE_MTK;
	else if (idx == 1)
		return PANEL_INPUT_FILE_SPRD;
	else
		return NULL;
}
int jlink_check_record_conm_start(void)
{
#define JLINK_RECORD_PROC_FN "/proc/jlink_fgu"
#define JLINK_MTK_FULL_FN "/proc/jlink_full"
	int fgu_fd = 0;
	int start_flag = 0;
	char buffer[2];


	fgu_fd = open_file(JLINK_RECORD_PROC_FN, O_RDONLY);
	if (fgu_fd < 0) {
		printf("@@@@@@@@@@@@ fgu: not sprd: %d\n", fgu_fd);
		fgu_fd = open_file(JLINK_MTK_FULL_FN, O_RDONLY);
		printf("@@@@@@@@@@@@ fgu: not sprd: %d\n", fgu_fd);
	}
	read(fgu_fd, buffer, 2);
	printf("@@@@@@@@@@@@ fgu: read out: %s\n", buffer);

	sscanf(buffer, "%d", &start_flag);
	printf("read out flag: %d\n", start_flag);
	close_file(fgu_fd);
	return start_flag;
}

int main(void)
{
	int ret;
	int pwr_idx=-1;
	char pwr_fname[64];
	char *cur_panel_bl_fn=NULL;
	system("setenforce 0");
	struct input_event ev;
	printf("=======================>%s %d\n", __func__, __LINE__);

	//find bl filename.
	cur_panel_bl_fn = get_panel_bl_file();
	if (cur_panel_bl_fn == NULL) {
		printf("!!! not find panel bl file\n");
		return -1;
	}

	//open pwr event
	pwr_idx = find_pwr_event();
	if (pwr_idx < 0) {
		printf("!!!! not find the pwr key input device\n");
		return -1;
	}
	snprintf(pwr_fname, 64, "/dev/input/event%d", pwr_idx);
	pwr_fd = open_file(pwr_fname, O_RDWR);
	if (pwr_fd < 0) return -1;
	panel_fd = open_file(cur_panel_bl_fn, O_RDWR);
	if (panel_fd < 0) {
		close_file(pwr_fd);
		return -1;
	}
	printf("=======================>%s %d\n", __func__, __LINE__);

	close_file(pwr_fd);
	close_file(panel_fd);

#ifdef ENABLE_BATTERY_DISCHG
	//start for conm auto record
	while (jlink_check_record_conm_start() == 0) {
		printf("jlink charging not done, we need to wait for start flag be 1\n");
		usleep(5*800000);
	}
	//end for conm auto_record
#endif

	printf("=======================>%s %d\n", __func__, __LINE__);

	char arg[300] = "sh /data/auto_test_monkey.sh &";
	char buf[256];
	int continues_sys_reboot = 0;

	ret = find_thread_there(" sh");
	printf("%s find thread ret: %d\n", __func__, ret);
	system(arg);

	printf("=======================>%s %d: now start while loop\n", __func__, __LINE__);
	while(1) {
		//######open file############
		panel_fd = open_file(cur_panel_bl_fn, O_RDWR);
		if (panel_fd < 0) return -1;

		usb_fd = open_file(USB_FUNC_FILE, O_RDWR);
		if (usb_fd < 0) {
			close_file(panel_fd);
			return -1;
		}

		//######## check sh runing ##############
		ret = find_thread_there(" sh");
		printf("%s find thread ret: %d\n", __func__, ret);

		//########### READ brightness and power up the panel ###########
		memset(buf,0,256);
		read(panel_fd, buf, 256);
		printf("%s %d current brightness: %s\n", __func__, __LINE__, buf);

		if (buf[0] == '0') {
			system("input keyevent 26;");
			printf("power up the lcd, input keyevent 26\n");
		}

		//##################check adb enable##########################
		memset(buf,0,256);
		read(usb_fd, buf, 256);
		printf("%s %d cur usb func is: %s\n", __func__, __LINE__, buf);
		//if (strstr(buf, "adb") == NULL && strstr(buf, "ffs") == NULL){
		if (strstr(buf, "adb") == NULL) {
			//disable adb while doing monkey. in bin setprop not work, we set it in shell
			//ret = system("setprop sys.usb.config mass_storage,adb;");ret = system("setprop ro.sys.usb.storage.type mass_storage,adb");
			ret = system("sh /data/adb_enable.sh &");
			printf("%s ret enable adb:%d, %d\n", __func__, errno, ret);
		}

		//##################close file##########################
		close(usb_fd);
		close(panel_fd);

		//##########rerun monkey################
		ret = find_thread_there(" sh");
		printf("%s find thread ret: %d\n", __func__, ret);
		if (ret == 0) {
			system(arg);
		}
#if 0
		if (ret) {
			continues_sys_reboot = continues_sys_reboot + 1;
			printf("%s continues_sys_reboot:%d\n", __func__, continues_sys_reboot);
		} else
			continues_sys_reboot = 0;

		if (SLEEP_TIME_SEC*continues_sys_reboot > MAX_CONT_SYSTEM_REBOOT_SECS) {
			printf("system continues reboot!!!! make it crash\n");
			system("echo c > /proc/sysrq-trigger");
		}
#endif

		usleep(SLEEP_TIME_SEC*800000);
	}




	return 0;

}


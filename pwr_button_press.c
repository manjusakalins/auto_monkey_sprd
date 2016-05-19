#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <linux/input.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>


#define SLEEP_TIME_SEC (60)
#define MAX_CONT_SYSTEM_REBOOT_SECS (1800)

//#define PWR_INPUT_FILE "/dev/input/event0"
//#define PANEL_INPUT_FILE "/sys/class/leds/lcd-backlight/brightness"
#define PWR_INPUT_FILE "/dev/input/event3"
#define PANEL_INPUT_FILE "/sys/class/backlight/sprd_backlight/brightness"
#define USB_FUNC_FILE "/sys/class/android_usb/android0/functions"
enum point_state {
	POINT_MOVE,
	POINT_ON,
	POINT_OFF,
};
int pwr_fd;
int panel_fd;
int usb_fd;

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

int find_thread_there(char *name)
{
	int cnts =0;
	char cmds[64] = {0};
	sprintf(cmds, "ps -Z | grep root | grep \"%s\"", name);
	printf("%s %d === %s\n", __func__, __LINE__, cmds);
	FILE *fp = popen(cmds, "r");//打开管道，执行shell 命令
	if (!fp)
		printf("%s %d\n", __func__, __LINE__);

	char buffer[128] = {0};
	while (NULL != fgets(buffer, 128, fp)) //逐行读取执行结果并打印
	{

		printf("PID:  %s \n", buffer);
		if (cnts++ == 0)
			continue;
		printf("%s %d\n", __func__, __LINE__);

		pclose(fp);
		return 1;
	};
	//printf("PID:  %s", buffer);
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
int main(void)
{
	int ret;
	system("setenforce 0");
	struct input_event ev;
	printf("%s %d\n", __func__, __LINE__);

	pwr_fd = open_file(PWR_INPUT_FILE, O_RDWR);
	if (pwr_fd < 0) return -1;
	panel_fd = open_file(PANEL_INPUT_FILE, O_RDWR);
	if (panel_fd < 0) {
		close_file(pwr_fd);
		return -1;
	}
	printf("%s %d\n", __func__, __LINE__);

#if 0
	//ret = write(pwr_fd, &ret, sizeof(ret));
	if (ret < 0) {
		close_file(pwr_fd);
		close_file(panel_fd);
		printf("error file open %d, ret: %d, errno: %d, %d\n", panel_fd, ret, errno, __LINE__);
		return -1;
	}
#endif

	close_file(pwr_fd);
	close_file(panel_fd);

	printf("%s %d\n", __func__, __LINE__);

	char arg[300] = "sh /data/auto_test_monkey.sh &";
	//char arg[80] = "sh /data/run.sh &"; //run nenamark2
	char buf[256];
	int continues_sys_reboot = 0;

	//ret = find_thread_there(" sh");
	printf("%s %d\n", __func__, ret);
	system(buf);
	system(arg);

	while(1) {
    	//######open file############
		panel_fd = open_file(PANEL_INPUT_FILE, O_RDWR);
		if (panel_fd < 0) return -1;

		usb_fd = open_file(USB_FUNC_FILE, O_RDWR);
	    if (usb_fd < 0) {
	        close_file(panel_fd);
		    return -1;
	    }

        //######## check sh runing ##############
		ret = find_thread_there(" sh");
		printf("%s %d\n", __func__, ret);

        //########### READ brightness and power up the panel ###########
		memset(buf,0,256);
		//printf("%s %d\n", __func__, __LINE__);
		read(panel_fd, buf, 256);
		close(panel_fd);
		printf("%s %d brightness: %s\n", __func__, __LINE__, buf);

		if (buf[0] == '0') {
			system("input keyevent 26;");
			printf("main thread running \n");
			//system(arg);
			printf("main thread running 2\n");
		}

		//##################check adb enable##########################
		memset(buf,0,256);
		read(usb_fd, buf, 256);
		close(usb_fd);
		printf("%s %d brightness: %s\n", __func__, __LINE__, buf);
		//if (strstr(buf, "adb") == NULL && strstr(buf, "ffs") == NULL){
		if (strstr(buf, "adb") == NULL) {
		    //disable adb while doing monkey. in bin setprop not work, we set it in shell 
		    //ret = system("setprop sys.usb.config mass_storage,adb;");ret = system("setprop ro.sys.usb.storage.type mass_storage,adb");
            ret = system("sh /data/adb_enable.sh &");
    		printf("%s ret adb:%d, %d\n", __func__, errno, ret);
		}

		//##########rerun monkey################
		ret = find_thread_there(" sh");
		printf("%s grep sh %d\n", __func__, ret);
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

		usleep(SLEEP_TIME_SEC*100000);
	}




	return 0;

}

#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <linux/input.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>


#define SLEEP_TIME_SEC (70)
#define MAX_CONT_SYSTEM_REBOOT_SECS (1800)

#define PWR_INPUT_FILE "/dev/input/event0"
#define PANEL_INPUT_FILE "/sys/class/leds/lcd-backlight/brightness"
enum point_state {
	POINT_MOVE,
	POINT_ON,
	POINT_OFF,
};
int pwr_fd;
int panel_fd;

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
	char cmds[64] = {0};
	sprintf(cmds, "ps | grep \'%s\'", name);
	//printf("%s %d === %s\n", __func__, __LINE__, cmds);
	FILE *fp = popen(cmds, "r");//打开管道，执行shell 命令
	if (!fp)
		printf("%s %d\n", __func__, __LINE__);

	char buffer[128] = {0};
	while (NULL != fgets(buffer, 128, fp)) //逐行读取执行结果并打印
	{
		printf("PID:  %s \n", buffer);
		pclose(fp);
		return 1;
	};
	//printf("PID:  %s", buffer);
	pclose(fp); //关闭返回的文件指针，注意不是用fclose噢
	return 0;
}

int main(void)
{
	int ret;
	struct input_event ev;
	printf("%s %d\n", __func__, __LINE__);
	pwr_fd = open(PWR_INPUT_FILE, O_RDWR);
	if (pwr_fd < 0) {
		printf("open file failed %d, errno: %d, %s\n", pwr_fd, errno, PWR_INPUT_FILE);
		return -1;
	}
	printf("%s %d\n", __func__, __LINE__);

	panel_fd = open(PANEL_INPUT_FILE, O_RDWR);
	if (panel_fd < 0) {
		close(pwr_fd);
		printf("error file open %d, errno: %d, %s\n", panel_fd, errno, PANEL_INPUT_FILE);
		return -1;
	}
	printf("%s %d\n", __func__, __LINE__);

	//ret = write(pwr_fd, &ret, sizeof(ret));
	if (ret < 0) {
		close(pwr_fd);
		close(panel_fd);
		printf("error file open %d, ret: %d, errno: %d, %d\n", panel_fd, ret, errno, __LINE__);
		return -1;
	}
	close(pwr_fd);
	close(panel_fd);

	printf("%s %d\n", __func__, __LINE__);

	//char arg[300] = "sh /data/auto_test_monkey.sh &";
	char arg[80] = "sh /data/run.sh &"; //run nenamark2
	char buf[256];
	int continues_sys_reboot = 0;

	while(1) {
		panel_fd = open(PANEL_INPUT_FILE, O_RDWR);
		if (panel_fd < 0) {
			close(pwr_fd);
			printf("error file open %d, errno: %d, %s\n", panel_fd, errno, PANEL_INPUT_FILE);
			return -1;
		}
		//printf("%s %d\n", __func__, __LINE__);

		memset(buf,0,256);
		//printf("%s %d\n", __func__, __LINE__);
		read(panel_fd, buf, 256);
		close(panel_fd);
		printf("%s %d brightness: %s\n", __func__, __LINE__, buf);
		//if (buf[0] == '0') {
			printf("main thread running \n");
			system(arg);
			printf("main thread running 2\n");
		//}

		ret = find_thread_there("bootanimation");
		if (ret) {
			continues_sys_reboot = continues_sys_reboot + 1;
			printf("%s continues_sys_reboot:%d\n", __func__, continues_sys_reboot);
		} else
			continues_sys_reboot = 0;

		if (SLEEP_TIME_SEC*continues_sys_reboot > MAX_CONT_SYSTEM_REBOOT_SECS) {
			printf("system continues reboot!!!! make it crash\n");
			system("echo c > /proc/sysrq-trigger");
		}


		usleep(SLEEP_TIME_SEC*1000000);
	}




	return 0;

}

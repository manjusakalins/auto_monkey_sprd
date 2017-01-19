#!/system/bin/sh

touch /data/whitelist.txt
touch /data/auto_test.log
touch /data/auto_test.err.log

echo $(date) > /data/auto_test.err.log

echo $(date) > /data/auto_test.log
#ls /data/data > /data/whitelist.txt
echo com.google.android.googlequicksearchbox > /data/blacklist.txt
echo mcube  >> /data/blacklist.txt
echo com.yingyonghui.market >> /data/blacklist.txt
echo com.google.android.apps.maps >> /data/blacklist.txt
echo com.chartcross.gpstestplus >> /data/blacklist.txt
echo com.google.android.apps.maps >> /data/blacklist.txt
echo com.google.android.backup >> /data/blacklist.txt
echo com.google.android.gm >> /data/blacklist.txt
echo com.google.android.gms >> /data/blacklist.txt
echo com.google.android.googlequicksearchbox >> /data/blacklist.txt
echo com.google.android.gsf >> /data/blacklist.txt
echo com.google.android.gsf.login >> /data/blacklist.txt
echo com.google.android.location >> /data/blacklist.txt
echo com.google.android.partnersetup >> /data/blacklist.txt
echo com.google.android.syncadapters.bookmarks >> /data/blacklist.txt
echo com.google.android.syncadapters.calendar >> /data/blacklist.txt
echo com.google.android.syncadapters.contacts >> /data/blacklist.txt
echo com.wiselinksz.user2root >> /data/blacklist.txt
echo com.wlcom.mmismttest >> /data/blacklist.txt
echo com.wlcom.mmitest >> /data/blacklist.txt
echo com.mediatek.engineermode >> /data/blacklist.txt
echo com.mediatek.mtklogger >> /data/blacklist.txt
echo com.mediatek.schpwronoff >> /data/blacklist.txt
#echo "monkeytesting" > /sys/class/android_usb/android0/iSerial
echo $RANDOM > /sys/class/android_usb/android0/iSerial

#CMD_PRE="adb shell"
CMD_PRE=""
pros=0
input_on=0
act_on=0
login_cnt=0
function check_if_activity_showup()
{
	let act_on=0
	re=$(dumpsys SurfaceFlinger | grep $1 )
	echo $re
	if [[ $re == *"$1"* ]];then
		let act_on=1
	else
		let act_on=0
	fi
}

function check_current_project()
{
	re=$(dumpsys SurfaceFlinger | grep "FB TARGET" )
	echo $re
	if [[ $re == *"800"* ]];then
		let pros=800
	elif [[ $re == *"1366"* ]]; then
		let pros=1366
	elif [[ $re == *"720"* ]]; then
		let pros=720
	else
		echo "not pro"
		let pros=101
	fi

}

function if_input_showup()
{
	re=$(dumpsys SurfaceFlinger | grep InputMethod )
	echo $re
	if [[ $re == *"InputMethod"* ]];then
		let input_on=1
	else
		let input_on=0
	fi	
}

function wait_act_showup()
{
	let w_cnt=0;
	echo "want act: " $1
	check_if_activity_showup $1
	while (( $act_on == 0 ));do
		sleep 2
		let w_cnt=$w_cnt+1;
		if [[ $w_cnt > 6 ]];then
			return 0
		fi
		echo "waiting times: "$w_cnt

		check_if_activity_showup $1
		
	done
}

while(true){

	echo "begin monkey test ..."
	#echo "monkeytesting" > /sys/class/android_usb/android0/iSerial
	echo $RANDOM > /sys/class/android_usb/android0/iSerial
	br=$(cat /sys/class/leds/lcd-backlight/brightness)
	echo $br
	if [ $br -eq "0" ]
	then
		echo "lcd is off, power it up"
		input keyevent 26
	fi

	#for sprd platform
	br=$(cat /sys/class/backlight/sprd_backlight/brightness)
	echo $br
	if [ $br -eq "0" ]
	then
		echo "lcd is off, power it up"
		input keyevent 26
	fi

	adbfunc=$(cat /sys/class/android_usb/android0/functions)
	echo $adbfunc
	if [[ $adbfunc == *"adb"* ]]
	then
		echo "got adb"
	else
		if [[ $adbfunc == *"ffs"* ]]
		then
			echo "got adb ffs"
		else
			echo "not adb, enable it"
			setprop sys.usb.config mass_storage,adb;
			setprop ro.sys.usb.storage.type mass_storage,adb;
		fi
	fi

	echo $RANDOM > /sys/class/android_usb/android0/iSerial

	input swipe 336 900  300 200 # unlock screen
	am force-stop com.android.settings
	sleep 2
	am start com.android.settings
	wait_act_showup com.android.settings.Settings
	input swipe 336 508 300 1130
	input tap 150 930
	wait_act_showup com.android.settings.SubSettings
	input tap 300 460
	sleep 1
	input tap 300 940
	sleep 1
	input tap 527 567
	sleep 1
	input tap 300 220
	sleep 5
}

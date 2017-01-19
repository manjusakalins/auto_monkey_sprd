adb root;
adb remount;
adb shell setenforce 0
adb push ./l1_ko_eng/mali.ko /system/lib/modules/
adb push auto_mon /system/bin/
adb push auto_test_monkey.sh /system/
adb push adb_enable.sh /data/
adb shell rm -rf /data/aee_exp

adb shell ps | grep auto_mon  | awk '{system("adb shell kill "$2)}'

adb shell ps | grep " sh"
adb shell rm -rf /data/aee_exp/;
adb shell rm -rf /sdcard/last_kmsg*
adb shell rm -rf /data/slog
adb shell rm -rf /sdcard/slog

adb shell rm -rf /custom/black_app-config.xml
adb push black_app-config.xml /custom/


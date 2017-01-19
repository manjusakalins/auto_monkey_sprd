adb shell rm /data/auto_test_monkey.sh
adb shell ps | grep vt | awk '{system("adb shell kill "$2)}'

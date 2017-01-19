#!/system/bin/sh
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

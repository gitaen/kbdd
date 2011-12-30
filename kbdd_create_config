#!/bin/sh

stop kbdd

export BTADAPTER=`dbus-send --system --dest=org.bluez --print-reply / org.bluez.Manager.DefaultAdapter | tail -1 | sed 's/^.*"\(.*\)".*$/\1/'`

dbus-send --system --dest=org.bluez --print-reply $BTADAPTER org.bluez.Adapter.ListDevices | \
grep path | sed 's/object path "\(.*\)"/\1/' > /tmp/devices

spp_address=none

while read device
do
	spp=0

	dbus-send --system --dest=org.bluez --print-reply $device org.bluez.Device.GetProperties \
	| grep -q 00001101-0000-1000-8000-00805f9b34fb && spp=1

	if [ $spp == 1 ]; then
		echo "Found spp device $device"	
		export spp_address=`echo $device | sed -e 's|/org.*dev_||' -e 's/_/:/g'`
		echo $spp_address
	fi
done < /tmp/devices
rm -f /tmp/devices

if [ $spp_address == none ]; then
	echo "Could not find any spp device!"
else
	cat <<- EOF

	kbdd knows the following driver: Please enter the right one"
	  foldable  - Compaq/HP foldable keyboard
	  stowaway  - Targus Stowaway keyboard
	  stowawayxt - Stowaway XT
	  snapntype - Snap'n'Type
	  snapntypebt - Snap'n'Type Bluetooth
	  hpslim    - HP Slim keyboard
	  smartbt   - Smart Bluetooth keyboard
	  lirc      - LIRC consumer IR
	  belkinir  - Belkin IR (not IrDA)
	  flexis    - Flexis FX-100 keyboard
	  g250      - Benq G250 gamepad
	  pocketvik - GrandTec PocketVIK
	  microfold - Micro Innovations Foldaway keyboard
	  micropad  - Micro Innovations Datapad
	  microkbd  - Compaq MicroKeyboard
	  targusir  - Targus Universal Wireless keyboard
	  freedom   - Freedom keyboard
	  btfoldable - HP iPAQ Bluetooth Foldable Keyboard
	  elektex - Elektex Cloth Keyboard
	  elektex_unmapped - Elektex Cloth Keyboard (Unmapped)
	EOF
	read driver 

	tee /tmp/rfcomm.conf <<- EOF
	#
	# RFCOMM configuration file.
	# created by $0
	#

	rfcomm1 {
	  # Automatically bind the device at startup
	  bind yes;

	  # Bluetooth address of the device
	  device $spp_address;

	  # RFCOMM channel for the connection
	  channel 1;

	  # Description of the connection
	  comment "spp keyboard for kbdd driver $driver";
	}
	EOF

	tee /tmp/kbdd.conf <<- EOF
	#
	# kbdd configuration file
	# created by $0
	#

	# Has to be consistent with rfcomm.conf
	port: /dev/rfcomm1

	# Manually-chosen driver
	type: $driver
	EOF

	echo
	echo "apply y/n?"
	read apply
	if [ $apply == y ]; then
		mv -f /tmp/rfcomm.conf /etc/bluetooth/rfcomm.conf
		mv -f /tmp/kbdd.conf /etc/bluetooth/kbdd/kbdd.conf
	else
		echo
		echo "The config files are in /tmp. You can still manually copy them"
		echo "to /etc/bluetooth/rfcomm.conf and /etc/bluetooth/kbdd/kbdd.conf"
		echo
		echo "To reload the kbdd daemon, type"
		echo "stop kbdd; start kbdd"
	fi

fi



start kbdd

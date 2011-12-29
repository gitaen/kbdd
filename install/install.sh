find . -name '*~' | xargs rm -f
cp -a etc usr /
chmod a+x /etc/init.d/kbdd /usr/bin/kbdd
update-sudoers

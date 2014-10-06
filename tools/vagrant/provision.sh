#!/bin/bash

die() { echo "$@" 1>&2; exit 1; }

apt-get update
apt-get install -y python python-setuptools python-dev libc6:i386 lib32stdc++6

easy_install cmdln ZODB3 colorama pyparsing intelhex BeautifulSoup4 Cheetah pyserial pytest decorator

SCONSVERSION=2.3.2
if [ -e /vagrant/.cached_downloads/scons-$SCONSVERSION.tar.gz ]; then
	echo "Found cached SCONS installer!"
	cp /vagrant/.cached_downloads/scons-$SCONSVERSION.tar.gz .
else
	URL=http://downloads.sourceforge.net/project/scons/scons/$SCONSVERSION/scons-$SCONSVERSION.tar.gz
	wget $URL
	cp ./scons-$SCONSVERSION.tar.gz /vagrant/.cached_downloads
fi
tar -xvzf scons-$SCONSVERSION.tar.gz

cd scons-$SCONSVERSION
python setup.py install
cd ..

gem install rake

XC8INSTALLER='xc8-v1.30-linux.run'
if [ -e /vagrant/.cached_downloads/$XC8INSTALLER ]; then
	echo "Found cached xc8 installer!"
	cp /vagrant/.cached_downloads/$XC8INSTALLER .
elif [ ! -e ./$XC8INSTALLER.run ]; then
	URL=http://ww1.microchip.com/downloads/en/DeviceDoc/$XC8INSTALLER.tar
	wget $URL -O - | tar -xvf -
	SHA=`openssl sha256 ./$XC8INSTALLER`
	if [ "$SHA" = "SHA256(./$XC8INSTALLER)= c47c850a23b018bd61b45bc1a87988e0a6f4223667b0761375fbf5f33469164b" ]; then
		mkdir -p /vagrant/.cached_downloads
		cp ./$XC8INSTALLER /vagrant/.cached_downloads
	else
		rm -f ./$XC8INSTALLER
		die "Invalid downloaded file hash, exiting!"
	fi
fi
echo "Installing xc8 compiler..."
./$XC8INSTALLER --mode unattended --netservername "" --prefix "/opt/microchip/xc8/v1.30"
CODE=$?
echo "PATH=\"\$PATH:/opt/microchip/xc8/v1.30/bin\"" >> /home/vagrant/.profile
rm -f ./$XC8INSTALLER
if [ $CODE -ne 0 ]; then
	die "Failed to install xc8, exiting!"
fi
echo "DONE!"

XC16INSTALLER=xc16-v1.21-linux-installer.run
if [ -e /vagrant/.cached_downloads/$XC16INSTALLER ]; then
	echo "Found cached xc16 installer!"
	cp /vagrant/.cached_downloads/$XC16INSTALLER .
elif [ ! -e ./$XC16INSTALLER ]; then
	URL=http://ww1.microchip.com/downloads/en/DeviceDoc/$XC16INSTALLER.tar
	wget $URL -O - | tar -xvf -
	SHA=`openssl sha256 ./$XC16INSTALLER`
	if [ "$SHA" = "SHA256(./$XC16INSTALLER)= 2d9a3736a439ad8842bac24de26bc9956cddf6e965288b8152bec770a4a3f06e" ]; then
		mkdir -p /vagrant/.cached_downloads
		cp ./$XC16INSTALLER /vagrant/.cached_downloads
	else
		rm -f ./$XC16INSTALLER
		die "Invalid downloaded file hash, exiting!"
	fi
fi
echo "Installing xc16 compiler..."
./$XC16INSTALLER --mode unattended --netservername "" --prefix "/opt/microchip/xc16/v1.21"
CODE=$?
rm -f ./$XC16INSTALLER
if [ $CODE -ne 0 ]; then
	die "Failed to install xc16, exiting!"
fi
echo "PATH=\"\$PATH:/opt/microchip/xc16/v1.21/bin\"" >> /home/vagrant/.profile
echo "DONE!"

#echo "Downloading GPSIM..."
#GPSIMVERSION=0.27.0
#URL=http://downloads.sourceforge.net/project/gpsim/gpsim/$GPSIMVERSION/gpsim-$GPSIMVERSION.tar.gz
#wget $URL -O - | tar -xvzf -

#echo "Installing GPSIM..."
#cd gpsim-$GPSIMVERSION
#./configure
#make
#make install
#cd ..
#echo "DONE!"

echo "Adding MoMo tool bin (/vagrant/tools/bin) to the path..."
echo "PATH=\"\$PATH:/vagrant/tools/bin\"" >> /home/vagrant/.profile
echo "DONE!"

echo "Adding user 'vagrant' to the group 'dialout' so it can access USB devices..."
usermod vagrant -a -G dialout
echo "DONE!"

exit 0
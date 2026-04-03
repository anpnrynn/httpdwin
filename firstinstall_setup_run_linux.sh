#!/bin/zsh
#If you are seeing this, then,
#Open terminal and goto /Volumes/HttpdWin/ and run this script
#zsh firstinstall_setup_run.sh
#Don't forget to create Public, Private keys and Certificates files in ~/HttpdWin/Certs/ folder

echo "Creating Directories"
echo ""
echo ""

mkdir -p $HOME/HttpdWin/
mkdir -p $HOME/HttpdWin/Pages
mkdir -p $HOME/HttpdWin/Certs
mkdir -p $HOME/HttpdWin/Tests
mkdir -p $HOME/HttpdWin/Temp
mkdir -p $HOME/HttpdWin/Storage

echo "Copying files"
echo ""
echo ""
cp httpdwin.conf $HOME/HttpdWin/
cp Pages/* $HOME/HttpdWin/Pages/
cp Tests/* $HOME/HttpdWin/Tests/

echo "Setting permissions"
echo ""
echo ""
chmod -R 755 $HOME/HttpdWin/*

echo "NOTE: Don't forget to create Public, Private keys and Certificates files in ~/HttpdWin/Certs/ folder"
echo ""
echo ""
echo "cd ~/HttpdWin/Certs"
echo "openssl req -x509 -newkey rsa:4096 -nodes -keyout httpdwinkey.pem -out httpdwincert.pem -days 3650"
echo "openssl x509 -pubkey -noout -in httpdwincert.pem  > httpdwinpub.pem"
echo "cp httpdwinkey.pem  httpdwinkey6.pem"
echo "cp httpdwinpub.pem  httpdwinpub6.pem"
echo "cp httpdwincert.pem httpdwincert6.pem"
echo ""
echo ""
echo "NOTE: Install Python version 3.14.0 from python website pkg image"
echo "and openssl3 from macports"

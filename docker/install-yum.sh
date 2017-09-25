#!/bin/bash

set -ex

EPEL_RPM_HASH=0dcc89f9bf67a2a515bad64569b7a9615edc5e018f676a578d5fd0f17d3c81d4
DEVTOOLS_HASH=a8ebeb4bed624700f727179e6ef771dafe47651131a00a78b342251415646acc

# CentOS 5 is only available through vault.
sed -i 's/enabled=1/enabled=0/' /etc/yum/pluginconf.d/fastestmirror.conf
sed -i 's/mirrorlist/#mirrorlist/' /etc/yum.repos.d/*.repo
sed -i 's|#\(baseurl.*\)mirror.centos.org/centos/$releasever|\1vault.centos.org/5.11|' /etc/yum.repos.d/*.repo
sed -i 's|#\(baseurl.*\)download.fedoraproject.org/pub|\1archives.fedoraproject.org/pub/archive|' /etc/yum.repos.d/*.repo

MY_DIR=$(dirname "${BASH_SOURCE[0]}")
source "$MY_DIR/build-utils.sh"

# EPEL support
yum -y install wget curl
curl -sLO https://dl.fedoraproject.org/pub/archive/epel/5/x86_64/epel-release-5-4.noarch.rpm
check_sha256sum epel-release-5-4.noarch.rpm $EPEL_RPM_HASH

# Dev toolset (for LLVM and other projects requiring C++11 support)
curl -sLO http://people.centos.org/tru/devtools-2/devtools-2.repo
check_sha256sum devtools-2.repo $DEVTOOLS_HASH
mv devtools-2.repo /etc/yum.repos.d/devtools-2.repo
rpm -Uvh --replacepkgs epel-release-5*.rpm
rm -f epel-release-5*.rpm

yum install -y java-1.7.0-openjdk java-1.7.0-openjdk-devel

#!/bin/bash
# this file is a shim to perform additional steps that configure the main ToolAnalysis container
# for use with the online Monitoring toolchain. This involves checking out the Monitoring branch,
# configuring the timezone, copying in a slack webhook, and re-building the Analyse executable.

cd /ToolAnalysis/ToolDAQ/ToolDAQFramework;
git pull;
make -j2;
cd /ToolAnalysis;
echo "fetching ToolAnalysis marc1uk fork";
git remote add marc1uk https://github.com/marc1uk/ToolAnalysis.git;
git fetch marc1uk;
echo "checking out monitoring branch";
git checkout --track marc1uk/monitoring_detached;
# prune unused tools
rm -rf ./UserTools/*
# retrieve those we need
git checkout -- .
echo "setting up environment";
. Setup.sh;
echo "building";
make clean;
make -j2;
echo "building again to be sure";
make -j2;
echo "configuring timezone to Chicago time";
if [ -f /etc/localtime ]; then
  rm -f /etc/localtime;
fi
if [ -f /usr/share/zoneinfo/America/Chicago ]; then
  ln -s /usr/share/zoneinfo/America/Chicago /etc/localtime;
else
  echo "Chicago timezone not found!"
  export TZ=America/Chicago
fi
if [ -f configfiles/Monitoring/slack_webhook ] && [ -f /tmp/slack_webhook ]; then
  echo "removing old slack webhook"
  rm config/Monitoring/slack_webhook;
fi;
if [ -f /tmp/slack_webhook ]; then
  echo "binding in new slack webhook"
  ln -s /tmp/slack_webhook configfiles/Monitoring/slack_webhook;
else
  echo "new slack webhook not found!"
fi
echo "ToolAnalysis ready: source Setup.sh and run './Analyse Monitoring&'"

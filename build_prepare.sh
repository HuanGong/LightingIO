#!/bin/bash

system=$(lsb_release -i -s)
if [ "Ubuntu" == "$system" ];then
  echo fucking
  sudo apt install libssl-dev
else
  echo aaaaaa
fi

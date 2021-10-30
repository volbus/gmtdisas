#!/bin/bash
# Scris de volbus la:	18-03-2021
# ultima modificare:	23-10-2021

#extragem versiunea software din git
#git tag -l --sort=-version:refname >> /tmp/gmtflasher_project_tags
#set_project_version /tmp/gmtflasher_project_tags version.h
#if [ $? -ne 0 ]; then
#	rm /tmp/gmtflasher_project_tags
#	exit
#fi
#rm /tmp/gmtflasher_project_tags


BASE_FLAGS="-g -Wall -o2 -std=gnu99"

gcc $BASE_FLAGS gmtdisas.c -o /usr/local/bin/gmtdisas

#check existance of /usr/share/gmtdisas folder, create if needed
if [ ! -d "/usr/share/gmtdisas" ]; then
	mkdir /usr/share/gmtdisas
	if [ $? -ne 0 ]; then
		exit
	fi
fi

#copy the stm8 IO-definition file
cp -u stm8.inc /usr/share/gmtdisas/

#
# A Makefile for osFree Janus Notepad
# (c) osFree project
#

DESCRIPTION = Janus Notepad
TARGET_VERSION=310

ADD_COPT = -ms -DDEBUG=1

!include $(%ROOT)tools/mk/build.mk


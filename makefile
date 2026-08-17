#
# A Makefile for osFree Janus Notepad
# (c) osFree project
#

DESCRIPTION = osFree Janus Notepad
TARGET_VERSION=310
SOURCES = main dialog

ADD_COPT = -ms -sg -DDEBUG=1
LIBS = commdlg shell

!include $(%ROOT)tools/mk/build.mk


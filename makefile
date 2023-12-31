#
# A Makefile for WinOS/2 Program Manager
# (c) osFree project,
#

PROJ  = notepad1
PROJ1 = notepad
TRGT = $(PROJ1).exe
DESC = Windows Notepad
srcfiles = $(p)main$(e) $(p)dialog$(e)

ADD_COPT = -ms -sg -DDEBUG=1

CLEAN_ADD = *.mbr
HEAPSIZE = 4k
STACKSIZE = 8k

ADD_LINKOPT = LIB commdlg.lib, shell.lib

!include $(%ROOT)tools/mk/appsw16.mk

TARGETS = $(PATH)$(PROJ1).exe # subdirs

.ico: $(MYDIR)res

$(PATH)$(PROJ1).exe: $(PATH)$(PROJ).exe $(MYDIR)rsrc.rc
 @$(SAY) RESCMP   $^. $(LOG)
 @wrc -q -bt=windows $]@ $[@ -fe=$@ -fo=$^@ -i=$(MYDIR) -i=$(%WATCOM)$(SEP)h$(SEP)win

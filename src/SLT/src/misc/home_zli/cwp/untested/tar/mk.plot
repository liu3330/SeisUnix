#########
#
#         THIS MAKEFILE IS FOR INITIAL CWP GRAPHICS INSTALLS
#         It should NOT be used on a machine already running CWP GRAPHICS!!!
#
#	  The source files on the tar tape use xxx Mb of disk.
#	  The installed executables and libs take another xxx Mb of disk.
#
#         CWP GRAPHICS requires ANSI C compiler/include files.
#         CWP GRAPHICS requires CWP's cwp and par directories
#
#########


#******  YOUR INPUT NEEDED *****************#
#
# ROOT is the parent of the directory where you want the binary to go
# If it is acceptable to you, choose /usr/local/cwp, then the binary
# will go into /usr/local/cwp/bin, the libs into /usr/local/cwp/lib and
# the includes into /usr/local/cwp/include.
ROOT = /usr/local/cwp
#
# OPTC is the optimization flag for your C compiler (-O for most)
OPTC = -O
#
#******  END INPUT--BUT KEEP READING!! *****#


#########
#
#           Many files at CWP have been maintained under RCS.
#           Hence they will often be read-only.  When such files
#           have to edited during the installation, the simplest
#           expedient is to give yourself write privileges by
#           "chmod u+w filename."
#
#           These files assume that libcwp and libpar are already
#           installed--these are in the tar.cwp.Z file.
#
#	    Some makes don't understand the $$@ notation and you'll
#           have to type the compile routine for each program in the group.
#
#           The codes are intended to be ANSI C.  Pre ANSI C compilers
#           are going to have a real hard time.
#
#           Some directories have Portability subdirectories, if you
#           seem to be missing something poke around in these directories.
#           We think that for full ANSI C compatible compilers, you won't
#           need any of this.
#
#           If the default ROOT was changed, after installation please
#           scrutinize the Makefiles for incorrect paths so they can be
#           used to install later fixes and enhancements.
#
#           This makefile is experimental.  It may well require some
#           intervention by a sophisticated user willing to diddle
#           around in the sub-directory makefiles.
#
######### INSTALLATION INSTRUCTIONS ########
#
#          "make -f mk.plot install" should do it!
#
#########


#########
#
#
#          Address questions to:
#
#                Jack K. Cohen
#	         Center for Wave Phenomena
#                Colorado School of Mines
#                Golden, CO 80401
#                (303) 273-3557
#                jkc@keller.Mines.Colorado.edu
#
#
######## THANKS.  YOU CAN STOP READING HERE UNLESS YOU ARE CURIOUS.


# Some convenient abbreviations
B =	$(ROOT)/bin
I =	$(ROOT)/include
L =	$(ROOT)/lib



donothing: # To protect against an idle "make" to "see what happens"
	@echo "This is a dangerous makefile--so the default is do_nothing"


install: mkdirs compile

mkdirs: # Make the binary directories if not already there
	@echo ""
	@echo "Creating binary directories (if not already there) ..."
	-mkdir $(ROOT) $B $L $I

compile	:
	cd ./psplot; $(MAKE) ROOT=$(ROOT) OPTC=$(OPTC)
	cd ./xplot; $(MAKE) ROOT=$(ROOT) OPTC=$(OPTC)

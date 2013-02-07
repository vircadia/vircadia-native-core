# DO NOT EDIT
# This makefile makes sure all linkable targets are
# up-to-date with anything they link to
default:
	echo "Do not invoke directly"

# For each target create a dummy rule so the target does not have to exist


# Rules to remove targets that are older than anything to which they
# link.  This forces Xcode to relink the targets from scratch.  It
# does not seem to check these dependencies itself.
PostBuild.interface.Debug:
/Users/birarda/code/worklist/interface/interface/src/Debug/interface:
	/bin/rm -f /Users/birarda/code/worklist/interface/interface/src/Debug/interface


PostBuild.interface.Release:
/Users/birarda/code/worklist/interface/interface/src/Release/interface:
	/bin/rm -f /Users/birarda/code/worklist/interface/interface/src/Release/interface


PostBuild.interface.MinSizeRel:
/Users/birarda/code/worklist/interface/interface/src/MinSizeRel/interface:
	/bin/rm -f /Users/birarda/code/worklist/interface/interface/src/MinSizeRel/interface


PostBuild.interface.RelWithDebInfo:
/Users/birarda/code/worklist/interface/interface/src/RelWithDebInfo/interface:
	/bin/rm -f /Users/birarda/code/worklist/interface/interface/src/RelWithDebInfo/interface



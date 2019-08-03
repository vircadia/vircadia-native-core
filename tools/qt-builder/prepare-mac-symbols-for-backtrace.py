# Scans the Mac qt-install directory for all known qt dylibs used by high fidelity.
# Then copies any DWARF symbols into a coorespoding dSYM file.
#
# usage
#   python prepare-mac-symbols-for-backtace.py QT_BUILD_DIR
#
# QT_BUILD_DIR should be the directory where qt was built, because
# it requires that the temporary .o files associated with the dylibs
# are present.  See hifi/tools/qt-builder/README.md for more info
#

import sys

# python 3 script
if (not sys.version_info > (3, 5)):
    print("ERROR: REQUIRES Python 3")
    quit()

import pathlib
import shutil
import os
import tarfile
import subprocess
import re

def cp(a, b):
    print("cp {} {}".format(a, b))
    shutil.copyfile(a, b)

# list of all qt dylibs used by hifi
# generarted by installing hifi and running these commands
# > find interface.app -exec file {} \; > files.txt
# > awk '$2 == "Mach-O"' files.txt > dylibs.txt
# then removing any non-qt dylibs from the list.
hifi_dylibs = [
    "Contents/PlugIns/mediaservice/libqavfmediaplayer.dylib",
    "Contents/PlugIns/mediaservice/libqtmedia_audioengine.dylib",
    "Contents/PlugIns/mediaservice/libqavfcamera.dylib",
    "Contents/PlugIns/quick/libdeclarative_multimedia.dylib",
    "Contents/PlugIns/quick/libqtquickextrasflatplugin.dylib",
    "Contents/PlugIns/quick/libqmlxmllistmodelplugin.dylib",
    "Contents/PlugIns/quick/libqtgraphicaleffectsprivate.dylib",
    "Contents/PlugIns/quick/libqtgraphicaleffectsplugin.dylib",
    "Contents/PlugIns/quick/libdeclarative_webchannel.dylib",
    "Contents/PlugIns/quick/libqtqmlremoteobjects.dylib",
    "Contents/PlugIns/quick/libmodelsplugin.dylib",
    "Contents/PlugIns/quick/libqtwebengineplugin.dylib",
    "Contents/PlugIns/quick/libqtquickextrasplugin.dylib",
    "Contents/PlugIns/quick/libqmlfolderlistmodelplugin.dylib",
    "Contents/PlugIns/quick/libqquicklayoutsplugin.dylib",
    "Contents/PlugIns/quick/libqtquickcontrols2materialstyleplugin.dylib",
    "Contents/PlugIns/quick/libqtquick2plugin.dylib",
    "Contents/PlugIns/quick/libwindowplugin.dylib",
    "Contents/PlugIns/quick/libqtquickcontrols2imaginestyleplugin.dylib",
    "Contents/PlugIns/quick/libdialogplugin.dylib",
    "Contents/PlugIns/quick/libqtquickcontrolsplugin.dylib",
    "Contents/PlugIns/quick/libwidgetsplugin.dylib",
    "Contents/PlugIns/quick/libqtquickcontrols2universalstyleplugin.dylib",
    "Contents/PlugIns/quick/libdialogsprivateplugin.dylib",
    "Contents/PlugIns/quick/libqtquickcontrols2fusionstyleplugin.dylib",
    "Contents/PlugIns/quick/libqtquicktemplates2plugin.dylib",
    "Contents/PlugIns/quick/libqmlsettingsplugin.dylib",
    "Contents/PlugIns/quick/libqtqmlstatemachine.dylib",
    "Contents/PlugIns/quick/libqtquickcontrols2plugin.dylib",
    "Contents/PlugIns/styles/libqmacstyle.dylib",
    "Contents/PlugIns/audio/libqtaudio_coreaudio.dylib",
    "Contents/PlugIns/bearer/libqgenericbearer.dylib",
    "Contents/PlugIns/iconengines/libqsvgicon.dylib",
    "Contents/PlugIns/imageformats/libqgif.dylib",
    "Contents/PlugIns/imageformats/libqwbmp.dylib",
    "Contents/PlugIns/imageformats/libqwebp.dylib",
    "Contents/PlugIns/imageformats/libqico.dylib",
    "Contents/PlugIns/imageformats/libqmacheif.dylib",
    "Contents/PlugIns/imageformats/libqjpeg.dylib",
    "Contents/PlugIns/imageformats/libqtiff.dylib",
    "Contents/PlugIns/imageformats/libqsvg.dylib",
    "Contents/PlugIns/imageformats/libqicns.dylib",
    "Contents/PlugIns/imageformats/libqtga.dylib",
    "Contents/PlugIns/imageformats/libqmacjp2.dylib",
    "Contents/Frameworks/QtGui.framework/Versions/5/QtGui",
    "Contents/Frameworks/QtGui.framework/QtGui",
    "Contents/Frameworks/QtDBus.framework/Versions/5/QtDBus",
    "Contents/Frameworks/QtDBus.framework/QtDBus",
    "Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork",
    "Contents/Frameworks/QtNetwork.framework/QtNetwork",
    "Contents/Frameworks/QtMultimedia.framework/Versions/5/QtMultimedia",
    "Contents/Frameworks/QtMultimedia.framework/QtMultimedia",
    "Contents/Frameworks/QtQml.framework/QtQml",
    "Contents/Frameworks/QtQml.framework/Versions/5/QtQml",
    "Contents/Frameworks/QtXml.framework/Versions/5/QtXml",
    "Contents/Frameworks/QtXml.framework/QtXml",
    "Contents/Frameworks/QtWidgets.framework/Versions/5/QtWidgets",
    "Contents/Frameworks/QtWidgets.framework/QtWidgets",
    "Contents/Frameworks/QtMultimediaQuick.framework/Versions/5/QtMultimediaQuick",
    "Contents/Frameworks/QtMultimediaQuick.framework/QtMultimediaQuick",
    "Contents/Frameworks/QtWebChannel.framework/Versions/5/QtWebChannel",
    "Contents/Frameworks/QtWebChannel.framework/QtWebChannel",
    "Contents/Frameworks/QtScriptTools.framework/Versions/5/QtScriptTools",
    "Contents/Frameworks/QtScriptTools.framework/QtScriptTools",
    "Contents/Frameworks/libPolyVoxCore.dylib",
    "Contents/Frameworks/QtWebSockets.framework/QtWebSockets",
    "Contents/Frameworks/QtWebSockets.framework/Versions/5/QtWebSockets",
    "Contents/Frameworks/QtQuickTemplates2.framework/QtQuickTemplates2",
    "Contents/Frameworks/QtQuickTemplates2.framework/Versions/5/QtQuickTemplates2",
    "Contents/Frameworks/QtCore.framework/Versions/5/QtCore",
    "Contents/Frameworks/QtCore.framework/QtCore",
    "Contents/Frameworks/QtWebEngine.framework/Versions/5/QtWebEngine",
    "Contents/Frameworks/QtWebEngine.framework/QtWebEngine",
    "Contents/Frameworks/QtOpenGL.framework/QtOpenGL",
    "Contents/Frameworks/QtOpenGL.framework/Versions/5/QtOpenGL",
    "Contents/Frameworks/QtMultimediaWidgets.framework/Versions/5/QtMultimediaWidgets",
    "Contents/Frameworks/QtMultimediaWidgets.framework/QtMultimediaWidgets",
    "Contents/Frameworks/QtQuickControls2.framework/Versions/5/QtQuickControls2",
    "Contents/Frameworks/QtQuickControls2.framework/QtQuickControls2",
    "Contents/Frameworks/QtRemoteObjects.framework/QtRemoteObjects",
    "Contents/Frameworks/QtRemoteObjects.framework/Versions/5/QtRemoteObjects",
    "Contents/Frameworks/QtConcurrent.framework/Versions/5/QtConcurrent",
    "Contents/Frameworks/QtConcurrent.framework/QtConcurrent",
    "Contents/Frameworks/QtScript.framework/QtScript",
    "Contents/Frameworks/QtScript.framework/Versions/5/QtScript",
    "Contents/Frameworks/QtQuick.framework/Versions/5/QtQuick",
    "Contents/Frameworks/QtQuick.framework/QtQuick",
    "Contents/Frameworks/QtPrintSupport.framework/Versions/5/QtPrintSupport",
    "Contents/Frameworks/QtPrintSupport.framework/QtPrintSupport",
    "Contents/Frameworks/QtSvg.framework/Versions/5/QtSvg",
    "Contents/Frameworks/QtSvg.framework/QtSvg",
    "Contents/Frameworks/QtWebEngineCore.framework/Versions/5/QtWebEngineCore",
    "Contents/Frameworks/QtWebEngineCore.framework/Versions/5/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess",
    "Contents/Frameworks/QtWebEngineCore.framework/QtWebEngineCore",
    "Contents/Frameworks/QtXmlPatterns.framework/Versions/5/QtXmlPatterns",
    "Contents/Frameworks/QtXmlPatterns.framework/QtXmlPatterns",
    "Contents/Resources/qml/QtGraphicalEffects/libqtgraphicaleffectsplugin.dylib",
    "Contents/Resources/qml/QtGraphicalEffects/private/libqtgraphicaleffectsprivate.dylib",
    "Contents/Resources/qml/QtQml/StateMachine/libqtqmlstatemachine.dylib",
    "Contents/Resources/qml/QtQml/Models.2/libmodelsplugin.dylib",
    "Contents/Resources/qml/QtQml/RemoteObjects/libqtqmlremoteobjects.dylib",
    "Contents/Resources/qml/Qt/labs/settings/libqmlsettingsplugin.dylib",
    "Contents/Resources/qml/Qt/labs/folderlistmodel/libqmlfolderlistmodelplugin.dylib",
    "Contents/Resources/qml/QtQuick.2/libqtquick2plugin.dylib",
    "Contents/Resources/qml/QtWebEngine/libqtwebengineplugin.dylib",
    "Contents/Resources/qml/QtWebChannel/libdeclarative_webchannel.dylib",
    "Contents/Resources/qml/QtMultimedia/libdeclarative_multimedia.dylib",
    "Contents/Resources/qml/QtQuick/Extras/libqtquickextrasplugin.dylib",
    "Contents/Resources/qml/QtQuick/XmlListModel/libqmlxmllistmodelplugin.dylib",
    "Contents/Resources/qml/QtQuick/PrivateWidgets/libwidgetsplugin.dylib",
    "Contents/Resources/qml/QtQuick/Layouts/libqquicklayoutsplugin.dylib",
    "Contents/Resources/qml/QtQuick/Window.2/libwindowplugin.dylib",
    "Contents/Resources/qml/QtQuick/Dialogs/Private/libdialogsprivateplugin.dylib",
    "Contents/Resources/qml/QtQuick/Dialogs/libdialogplugin.dylib",
    "Contents/Resources/qml/QtQuick/Templates.2/libqtquicktemplates2plugin.dylib",
    "Contents/Resources/qml/QtQuick/Controls.2/Material/libqtquickcontrols2materialstyleplugin.dylib",
    "Contents/Resources/qml/QtQuick/Controls.2/Universal/libqtquickcontrols2universalstyleplugin.dylib",
    "Contents/Resources/qml/QtQuick/Controls.2/Imagine/libqtquickcontrols2imaginestyleplugin.dylib",
    "Contents/Resources/qml/QtQuick/Controls.2/Fusion/libqtquickcontrols2fusionstyleplugin.dylib",
    "Contents/Resources/qml/QtQuick/Controls.2/libqtquickcontrols2plugin.dylib",
    "Contents/Resources/qml/QtQuick/Controls/Styles/Flat/libqtquickextrasflatplugin.dylib",
    "Contents/Resources/qml/QtQuick/Controls/libqtquickcontrolsplugin.dylib",
]

# strip off the path
desired_dylibs = [pathlib.PurePath(i).name for i in hifi_dylibs]

QT_BUILD_DIRECTORY = sys.argv[1]

# all files in the qt build directory
found_files = pathlib.Path(QT_BUILD_DIRECTORY).glob('**/*')

# create temp directory
TEMP_DIR_NAME = "backtrace"
if os.path.exists(TEMP_DIR_NAME):
    shutil.rmtree(TEMP_DIR_NAME)
os.mkdir(TEMP_DIR_NAME)

# for each file in the build directory (this might take a while)
for f in found_files:

    # if this file is desired
    if f.name in desired_dylibs:

        # run the file command on this file to determine what kind of file it is.
        result = subprocess.run(['file', f], stdout=subprocess.PIPE)

        # if this file is a dylib
        if re.match(".*Mach-O 64-bit dynamically linked shared library x86_64", str(result.stdout)):

            dst_dylib = pathlib.PurePath(TEMP_DIR_NAME, f.name)
            dst_dsym = pathlib.PurePath(TEMP_DIR_NAME, f.stem + ".dSYM")

            # generate a dSYM file for this dylib in the temp folder
            subprocess.run(['dsymutil', f, '-o', dst_dsym])

            # copy dylib into the temp folder
            cp(f, dst_dylib)

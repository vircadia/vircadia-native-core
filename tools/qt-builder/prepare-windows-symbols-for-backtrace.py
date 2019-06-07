# Scans the Windows qt-install directory for all known qt dlls used by high fidelity.
# Then copies all matching dlls and pdbs into folder named backtrace.
#
# usage
#   python prepare-windows-symbols-for-backrace.py QT_INSTALL_DIR
#
# QT_INSTALL_DIR should be the directory where qt is installed after running `jom install` or `nmake install`
# see hifi/tools/qt-builder/README.md for more info
#

import sys

# python 3 script
if (not sys.version_info > (3, 2)):
    print("ERROR: REQUIRES Python 3")
    quit()

import pathlib
import shutil
import os
import tarfile

def cp(a, b):
    print("cp {} {}".format(a, b))
    shutil.copyfile(a, b)

# list of all qt dlls used in hifi
hifi_dlls = [
    "audioWin7/audio/qtaudio_windows.dll",
    "audioWin8/audio/qtaudio_wasapi.dll",
    "bearer/qgenericbearer.dll",
    "iconengines/qsvgicon.dll",
    "imageformats/qgif.dll",
    "imageformats/qicns.dll",
    "imageformats/qico.dll",
    "imageformats/qjpeg.dll",
    "imageformats/qsvg.dll",
    "imageformats/qtga.dll",
    "imageformats/qtiff.dll",
    "imageformats/qwbmp.dll",
    "imageformats/qwebp.dll",
    "mediaservice/dsengine.dll",
    "mediaservice/qtmedia_audioengine.dll",
    "mediaservice/wmfengine.dll",
    "platforminputcontexts/qtvirtualkeyboardplugin.dll",
    "platforms/qwindows.dll",
    "playlistformats/qtmultimedia_m3u.dll",
    "position/qtposition_geoclue.dll",
    "position/qtposition_positionpoll.dll",
    "position/qtposition_serialnmea.dll",
    "qmltooling/qmldbg_debugger.dll",
    "qmltooling/qmldbg_inspector.dll",
    "qmltooling/qmldbg_local.dll",
    "qmltooling/qmldbg_messages.dll",
    "qmltooling/qmldbg_native.dll",
    "qmltooling/qmldbg_nativedebugger.dll",
    "qmltooling/qmldbg_preview.dll",
    "qmltooling/qmldbg_profiler.dll",
    "qmltooling/qmldbg_quickprofiler.dll",
    "qmltooling/qmldbg_server.dll",
    "qmltooling/qmldbg_tcp.dll",
    "Qt/labs/folderlistmodel/qmlfolderlistmodelplugin.dll",
    "Qt/labs/settings/qmlsettingsplugin.dll",
    "Qt5Core.dll",
    "Qt5Gui.dll",
    "Qt5Multimedia.dll",
    "Qt5MultimediaQuick.dll",
    "Qt5Network.dll",
    "Qt5Positioning.dll",
    "Qt5Qml.dll",
    "Qt5Quick.dll",
    "Qt5QuickControls2.dll",
    "Qt5QuickTemplates2.dll",
    "Qt5RemoteObjects.dll",
    "Qt5Script.dll",
    "Qt5ScriptTools.dll",
    "Qt5SerialPort.dll",
    "Qt5Svg.dll",
    "Qt5WebChannel.dll",
    "Qt5WebEngine.dll",
    "Qt5WebEngineCore.dll",
    "Qt5WebSockets.dll",
    "Qt5Widgets.dll",
    "Qt5XmlPatterns.dll",
    "QtGraphicalEffects/private/qtgraphicaleffectsprivate.dll",
    "QtGraphicalEffects/qtgraphicaleffectsplugin.dll",
    "QtMultimedia/declarative_multimedia.dll",
    "QtQml/Models.2/modelsplugin.dll",
    "QtQml/RemoteObjects/qtqmlremoteobjects.dll",
    "QtQml/StateMachine/qtqmlstatemachine.dll",
    "QtQuick/Controls/qtquickcontrolsplugin.dll",
    "QtQuick/Controls/Styles/Flat/qtquickextrasflatplugin.dll",
    "QtQuick/Controls.2/Fusion/qtquickcontrols2fusionstyleplugin.dll",
    "QtQuick/Controls.2/Imagine/qtquickcontrols2imaginestyleplugin.dll",
    "QtQuick/Controls.2/Material/qtquickcontrols2materialstyleplugin.dll",
    "QtQuick/Controls.2/qtquickcontrols2plugin.dll",
    "QtQuick/Controls.2/Universal/qtquickcontrols2universalstyleplugin.dll",
    "QtQuick/Dialogs/dialogplugin.dll",
    "QtQuick/Dialogs/Private/dialogsprivateplugin.dll",
    "QtQuick/Extras/qtquickextrasplugin.dll",
    "QtQuick/Layouts/qquicklayoutsplugin.dll",
    "QtQuick/PrivateWidgets/widgetsplugin.dll",
    "QtQuick/Templates.2/qtquicktemplates2plugin.dll",
    "QtQuick/Window.2/windowplugin.dll",
    "QtQuick/XmlListModel/qmlxmllistmodelplugin.dll",
    "QtQuick.2/qtquick2plugin.dll",
    "QtWebChannel/declarative_webchannel.dll",
    "QtWebEngine/qtwebengineplugin.dll",
    "scenegraph/qsgd3d12backend.dll",
    "styles/qwindowsvistastyle.dll",
]

# list of desired pdbs, created from hifi_dlls
desired_pdbs = [pathlib.PurePath(i).stem + ".pdb" for i in hifi_dlls]

QT_INSTALL_DIRECTORY = sys.argv[1]

# find all pdb files in the qt build directory
found_pdbs = pathlib.Path(QT_INSTALL_DIRECTORY).glob('**/*.pdb')

# create temp directory
TEMP_DIR_NAME = "backtrace"
if os.path.exists(TEMP_DIR_NAME):
    shutil.rmtree(TEMP_DIR_NAME)
os.mkdir(TEMP_DIR_NAME)

# copy all found dlls and pdbs into the temp directory
for pdb in found_pdbs:
    if pdb.name in desired_pdbs:
        dll = pathlib.PurePath(pdb.parent, pdb.stem + ".dll")
        cp(pdb, pathlib.PurePath(TEMP_DIR_NAME, pdb.name))
        cp(dll, pathlib.PurePath(TEMP_DIR_NAME, dll.name))

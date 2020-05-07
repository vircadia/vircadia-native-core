# Post build script
import os
import sys
import shutil
import zipfile
import base64

SOURCE_PATH = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), '..', '..'))
# FIXME move the helper python modules somewher other than the root of the repo
sys.path.append(SOURCE_PATH)

import hifi_utils

BUILD_PATH = os.path.join(SOURCE_PATH, 'build')
INTERFACE_BUILD_PATH = os.path.join(BUILD_PATH, 'interface', 'Release')
WIPE_PATHS = []

if sys.platform == "win32":
    WIPE_PATHS = [
        'jsdoc'
    ]
elif sys.platform == "darwin":
    INTERFACE_BUILD_PATH = os.path.join(INTERFACE_BUILD_PATH, "Interface.app", "Contents", "Resources")
    WIPE_PATHS = [
        'jsdoc'
    ]



# Customize the output filename
def computeArchiveName(prefix):
    RELEASE_TYPE = os.getenv("RELEASE_TYPE", "DEV")
    RELEASE_NUMBER = os.getenv("RELEASE_NUMBER", "")
    GIT_COMMIT_SHORT = os.getenv("SHA7", "")
    if GIT_COMMIT_SHORT == '':
        GIT_COMMIT_SHORT = os.getenv("GIT_COMMIT_SHORT", "")

    if RELEASE_TYPE == "PRODUCTION":
        BUILD_VERSION = "{}-{}".format(RELEASE_NUMBER, GIT_COMMIT_SHORT)
    elif RELEASE_TYPE == "PR":
        BUILD_VERSION = "PR{}-{}".format(RELEASE_NUMBER, GIT_COMMIT_SHORT)
    else:
        BUILD_VERSION = "dev"

    if sys.platform == "win32":
        PLATFORM = "windows"
    elif sys.platform == "darwin":
        PLATFORM = "mac"
    else:
        PLATFORM = "other"

    ARCHIVE_NAME = "{}-{}-{}".format(prefix, BUILD_VERSION, PLATFORM)
    return ARCHIVE_NAME

def wipeClientBuildPath(relativePath):
    targetPath = os.path.join(INTERFACE_BUILD_PATH, relativePath)
    print("Checking path {}".format(targetPath))
    if os.path.exists(targetPath):
        print("Removing path {}".format(targetPath))
        shutil.rmtree(targetPath)

def fixupMacZip(filename):
    fullPath = os.path.join(BUILD_PATH, "{}.zip".format(filename))
    outFullPath = "{}.zip".format(fullPath)
    print("Fixup mac ZIP file {}".format(fullPath))
    with zipfile.ZipFile(fullPath) as inzip:
        with zipfile.ZipFile(outFullPath, 'w') as outzip:
            rootPath = inzip.infolist()[0].filename
            for entry in inzip.infolist():
                if entry.filename == rootPath:
                    continue
                newFilename = entry.filename[len(rootPath):]
                # ignore the icon
                if newFilename.startswith('Icon'):
                    continue
                # ignore the server console
                if newFilename.startswith('Console.app'):
                    continue
                # ignore the nitpick app
                if newFilename.startswith('nitpick.app'):
                    continue
                # if we made it here, include the file in the output
                buffer = inzip.read(entry.filename)
                entry.filename = newFilename
                outzip.writestr(entry, buffer)
            outzip.close()
    print("Replacing {} with fixed {}".format(fullPath, outFullPath))
    shutil.move(outFullPath, fullPath)

def fixupWinZip(filename):
    fullPath = os.path.join(BUILD_PATH, "{}.zip".format(filename))
    outFullPath = "{}.zip".format(fullPath)
    print("Fixup windows ZIP file {}".format(fullPath))
    with zipfile.ZipFile(fullPath) as inzip:
        with zipfile.ZipFile(outFullPath, 'w') as outzip:
            for entry in inzip.infolist():
                # ignore the server console
                if entry.filename.startswith('server-console/'):
                    continue
                # ignore the nitpick app
                if entry.filename.startswith('nitpick/'):
                    continue
                # if we made it here, include the file in the output
                buffer = inzip.read(entry.filename)
                outzip.writestr(entry, buffer)
            outzip.close()
    print("Replacing {} with fixed {}".format(fullPath, outFullPath))
    shutil.move(outFullPath, fullPath)

def signBuild(executablePath):
    if sys.platform != 'win32':
        print('Skipping signing because platform is not win32')
        return

    RELEASE_TYPE = os.getenv("RELEASE_TYPE", "")
    if RELEASE_TYPE != "PRODUCTION":
        print('Skipping signing because RELEASE_TYPE "{}" != "PRODUCTION"'.format(RELEASE_TYPE))
        return

    HF_PFX_FILE = os.getenv("HF_PFX_FILE", "")
    if HF_PFX_FILE == "":
        print('Skipping signing because HF_PFX_FILE is empty')
        return

    HF_PFX_PASSPHRASE = os.getenv("HF_PFX_PASSPHRASE", "")
    if HF_PFX_PASSPHRASE == "":
        print('Skipping signing because HF_PFX_PASSPHRASE is empty')
        return

    # FIXME use logic similar to the SetPackagingParameteres.cmake to locate the executable
    SIGN_TOOL = "C:/Program Files (x86)/Windows Kits/10/bin/10.0.17763.0/x64/signtool.exe"
    # sign the launcher executable
    print("Signing {}".format(executablePath))
    hifi_utils.executeSubprocess([
            SIGN_TOOL,
            'sign', 
            '/fd', 'sha256',
            '/f', HF_PFX_FILE,
            '/p', HF_PFX_PASSPHRASE,
            '/tr', 'http://sha256timestamp.ws.symantec.com/sha256/timestamp',
            '/td', 'SHA256',
            executablePath
        ])


def zipDarwinLauncher():
    launcherSourcePath = os.path.join(SOURCE_PATH, 'launchers', 'qt')
    launcherBuildPath = os.path.join(BUILD_PATH, 'launcher')

    archiveName = computeArchiveName('HQ Launcher')

    cpackCommand = [
        'cpack',
        '-G', 'ZIP',
        '-D', "CPACK_PACKAGE_FILE_NAME={}".format(archiveName),
        '-D', "CPACK_INCLUDE_TOPLEVEL_DIRECTORY=OFF"
    ]
    print("Create ZIP version of installer archive")
    print(cpackCommand)
    hifi_utils.executeSubprocess(cpackCommand, folder=launcherBuildPath)
    launcherZipDestFile = os.path.join(BUILD_PATH, "{}.zip".format(archiveName))
    launcherZipSourceFile = os.path.join(launcherBuildPath, "{}.zip".format(archiveName))
    print("Moving {} to {}".format(launcherZipSourceFile, launcherZipDestFile))
    shutil.move(launcherZipSourceFile, launcherZipDestFile)


def buildLightLauncher():
    launcherSourcePath = os.path.join(SOURCE_PATH, 'launchers', 'qt')
    launcherBuildPath = os.path.join(BUILD_PATH, 'launcher')
    if not os.path.exists(launcherBuildPath):
        os.makedirs(launcherBuildPath)
    # configure launcher build
    cmakeArgs = [ 'cmake', '-S', launcherSourcePath ]

    if sys.platform == 'darwin':
        cmakeArgs.append('-G')
        cmakeArgs.append('Xcode')
    elif sys.platform == 'win32':
        cmakeArgs.append('-A')
        cmakeArgs.append('x64')

    hifi_utils.executeSubprocess(cmakeArgs, folder=launcherBuildPath)

    buildTarget = 'package'
    if sys.platform == 'win32':
        buildTarget = 'ALL_BUILD'
    hifi_utils.executeSubprocess([
            'cmake',
            '--build', launcherBuildPath,
            '--config', 'Release',
            '--target', buildTarget
        ], folder=launcherBuildPath)
    if sys.platform == 'darwin':
        zipDarwinLauncher()
        launcherDestFile = os.path.join(BUILD_PATH, "{}.dmg".format(computeArchiveName('Launcher')))
        launcherSourceFile = os.path.join(launcherBuildPath, "HQ Launcher.dmg")
    elif sys.platform == 'win32':
        launcherDestFile = os.path.join(BUILD_PATH, "{}.exe".format(computeArchiveName('Launcher')))
        launcherSourceFile = os.path.join(launcherBuildPath, "Release", "HQLauncher.exe")

    print("Moving {} to {}".format(launcherSourceFile, launcherDestFile))
    shutil.move(launcherSourceFile, launcherDestFile)
    signBuild(launcherDestFile)



# Main 
for wipePath in WIPE_PATHS:
    wipeClientBuildPath(wipePath)

# Need the archive name for ad-hoc zip file manipulation
archiveName = computeArchiveName('HighFidelity-Beta-Interface')

cpackCommand = [
    'cpack', 
    '-G', 'ZIP', 
    '-D', "CPACK_PACKAGE_FILE_NAME={}".format(archiveName), 
    '-D', "CPACK_INCLUDE_TOPLEVEL_DIRECTORY=OFF"
    ]

print("Create ZIP version of installer archive")
print(cpackCommand)
hifi_utils.executeSubprocess(cpackCommand, folder=BUILD_PATH)

if sys.platform == "win32":
    fixupWinZip(archiveName)
elif sys.platform == "darwin":
    fixupMacZip(archiveName)

buildLightLauncher()

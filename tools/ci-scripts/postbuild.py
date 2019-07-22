# Post build script
import os
import sys
import shutil
import subprocess
import tempfile
import uuid
import zipfile
import base64

SOURCE_PATH = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), '..', '..'))
# FIXME move the helper python modules somewher other than the root of the repo
sys.path.append(SOURCE_PATH)

import hifi_utils


class ZipAttrs:
    """ A readable wrapper for ZipInfo's external attributes bit field """

    S_IFREG = 0x8
    S_IFLNK = 0xa
    S_IFDIR = 0x4
    MODE_MASK = 0xfff0000

    def __init__(self, zip_info):
        # File stats are the 4 high bits of external_attr, a 32 bit field.
        self._stat = zip_info.external_attr >> 28
        self.mode = (zip_info.external_attr & self.MODE_MASK) >> 16

    @property
    def is_symlink(self):
        return self._stat == self.S_IFLNK

    @property
    def is_dir(self):
        return self._stat == self.S_IFDIR

    @property
    def is_regular(self):
        return self._stat == self.S_IFREG


BUILD_PATH = os.path.join(SOURCE_PATH, 'build')
INTERFACE_BUILD_PATH = os.path.join(BUILD_PATH, 'interface', 'Release')
WIPE_PATHS = []

if sys.platform == "win32":
    WIPE_PATHS = [
        'jsdoc',
        'resources/serverless'
    ]
elif sys.platform == "darwin":
    INTERFACE_BUILD_PATH = os.path.join(INTERFACE_BUILD_PATH, "Interface.app", "Contents", "Resources")
    WIPE_PATHS = [
        'jsdoc',
        'serverless'
    ]



# Customize the output filename
def computeArchiveName(prefix):
    RELEASE_TYPE = os.getenv("RELEASE_TYPE", "DEV")
    RELEASE_NUMBER = os.getenv("RELEASE_NUMBER", "")
    GIT_PR_COMMIT_SHORT = os.getenv("SHA7", "")
    if GIT_PR_COMMIT_SHORT == '':
        GIT_PR_COMMIT_SHORT = os.getenv("GIT_PR_COMMIT_SHORT", "")

    if RELEASE_TYPE == "PRODUCTION":
        BUILD_VERSION = "{}-{}".format(RELEASE_NUMBER, GIT_PR_COMMIT_SHORT)
    elif RELEASE_TYPE == "PR":
        BUILD_VERSION = "PR{}-{}".format(RELEASE_NUMBER, GIT_PR_COMMIT_SHORT)
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
    tmpDir = os.path.join(tempfile.gettempdir(),
                          'com.highfidelity.launcher.postbuild',
                          str(uuid.uuid4()))

    try:
        with zipfile.ZipFile(fullPath) as inzip:
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
                # ignore the serverless content
                if newFilename.startswith('interface.app/Contents/Resources/serverless'):
                    continue
                # if we made it here, include the file in the output
                buffer = inzip.read(entry.filename)
                newFilename = os.path.join(tmpDir, newFilename)

                attrs = ZipAttrs(entry)
                if attrs.is_dir:
                    os.makedirs(newFilename)
                elif attrs.is_regular:
                    with open(newFilename, mode='wb') as _file:
                        _file.write(buffer)
                    os.chmod(newFilename, mode=attrs.mode)
                elif attrs.is_symlink:
                    os.symlink(buffer, newFilename)
                else:
                    raise IOError('Invalid file stat')

        if 'XCODE_DEVELOPMENT_TEAM' in os.environ:
            print('XCODE_DEVELOPMENT_TEAM environment variable is not set. '
                  'Not signing build.', file=sys.stderr)
        else:
            # The interface.app bundle must be signed again after being
            # stripped.
            print('Signing interface.app bundle')
            entitlementsPath = os.path.join(
                os.path.dirname(__file__),
                '../../interface/interface.entitlements')
            subprocess.run([
                'codesign', '-s', 'Developer ID Application', '--deep',
                '--timestamp', '--force', '--entitlements', entitlementsPath,
                os.path.join(tmpDir, 'interface.app')
            ], check=True)

        # Repackage the zip including the signed version of interface.app
        print('Replacing {} with fixed {}'.format(fullPath, outFullPath))
        if os.path.exists(outFullPath):
            print('fixed zip already exists, deleting it', file=sys.stderr)
            os.unlink(outFullPath)
        previous_cwd = os.getcwd()
        os.chdir(tmpDir)
        try:
            subprocess.run(['zip', '--symlink', '-r', outFullPath, './.'],
                           stdout=subprocess.DEVNULL, check=True)
        finally:
            os.chdir(previous_cwd)

    finally:
        shutil.rmtree(tmpDir)

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


def buildLightLauncher():
    launcherSourcePath = os.path.join(SOURCE_PATH, 'launchers', sys.platform)
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

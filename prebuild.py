#!python

# The prebuild script is intended to simplify life for developers and dev-ops.  It's repsonsible for acquiring 
# tools required by the build as well as dependencies on which we rely.  
# 
# By using this script, we can reduce the requirements for a developer getting started to:
#
# * A working C++ dev environment like visual studio, xcode, gcc, or clang
# * Qt 
# * CMake
# * Python 3.x
#
# The function of the build script is to acquire, if not already present, all the other build requirements
# The build script should be idempotent.  If you run it with the same arguments multiple times, that should 
# have no negative impact on the subsequent build times (i.e. re-running the prebuild script should not 
# trigger a header change that causes files to be rebuilt).  Subsequent runs after the first run should 
# execute quickly, determining that no work is to be done

import hifi_singleton
import hifi_utils
import hifi_android
import hifi_vcpkg
import hifi_qt

import argparse
import concurrent
import hashlib
import importlib
import json
import os
import platform
import shutil
import ssl
import sys
import re
import tempfile
import time
import functools
import subprocess
import logging

from uuid import uuid4
from contextlib import contextmanager

print = functools.partial(print, flush=True)

class TrackableLogger(logging.Logger):
    guid = str(uuid4())

    def _log(self, msg, *args, **kwargs):
        x = {'guid': self.guid}
        if 'extra' in kwargs:
            kwargs['extra'].update(x)
        else:
            kwargs['extra'] = x
        super()._log(msg, *args, **kwargs)

logging.setLoggerClass(TrackableLogger)
logger = logging.getLogger('prebuild')

@contextmanager
def timer(name):
    ''' Print the elapsed time a context's execution takes to execute '''
    start = time.time()
    yield
    # Please take care when modifiying this print statement.
    # Log parsing logic may depend on it.
    logger.info('%s took %.3f secs' % (name, time.time() - start))

def parse_args():
    # our custom ports, relative to the script location
    defaultPortsPath = hifi_utils.scriptRelative('cmake', 'ports')
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Prepare build dependencies.')
    parser.add_argument('--android', type=str)
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--force-bootstrap', action='store_true')
    parser.add_argument('--force-build', action='store_true')
    parser.add_argument('--release-type', type=str, default="DEV", help="DEV, PR, or PRODUCTION")
    parser.add_argument('--vcpkg-root', type=str, help='The location of the vcpkg distribution')
    parser.add_argument('--vcpkg-build-type', type=str, help='Could be `release` or `debug`. By default it doesn`t set the build-type')
    parser.add_argument('--vcpkg-skip-clean', action='store_true', help='Skip the cleanup of vcpkg downloads and packages folders after vcpkg build completition.')
    parser.add_argument('--build-root', required=True, type=str, help='The location of the cmake build')
    parser.add_argument('--ports-path', type=str, default=defaultPortsPath)
    parser.add_argument('--ci-build', action='store_true', default=os.getenv('CI_BUILD') is not None)
    if True:
        args = parser.parse_args()
    else:
        args = parser.parse_args(['--android', 'questInterface', '--build-root', 'C:/git/vircadia/android/apps/questInterface/.externalNativeBuild/cmake/debug/arm64-v8a'])
    return args

def main():
    # Fixup env variables.  Leaving `USE_CCACHE` on will cause scribe to fail to build
    # VCPKG_ROOT seems to cause confusion on Windows systems that previously used it for 
    # building OpenSSL
    removeEnvVars = ['VCPKG_ROOT', 'USE_CCACHE']
    for var in removeEnvVars:
        if var in os.environ:
            del os.environ[var]

    args = parse_args()
    assets_url = hifi_utils.readEnviromentVariableFromFile(args.build_root, 'EXTERNAL_BUILD_ASSETS')

    if args.ci_build:
        logging.basicConfig(datefmt='%H:%M:%S', format='%(asctime)s %(guid)s %(message)s', level=logging.INFO)

    logger.info('start')

    # OS dependent information
    system = platform.system()
    if 'Windows' == system and 'CI_BUILD' in os.environ and os.environ["CI_BUILD"] == "Github":
        logger.info("Downloading NSIS")
        with timer('NSIS'):
            hifi_utils.downloadAndExtract(assets_url + '/dependencies/NSIS-hifi-plugins-1.0.tgz', "C:/Program Files (x86)")

    qtInstallPath = None
    # If not android, install our Qt build
    if not args.android:
        qt = hifi_qt.QtDownloader(args)
        qtInstallPath = qt.cmakePath

        if qtInstallPath is not None:
            # qtInstallPath is None when we're doing a system Qt build
            print("cmake path: " + qtInstallPath)

            with hifi_singleton.Singleton(qt.lockFile) as lock:
                with timer('Qt'):
                    qt.installQt()
                    qt.writeConfig()
        else:
            if (os.environ["VIRCADIA_USE_SYSTEM_QT"]):
                print("System Qt selected")
            else:
                raise Exception("Internal error: System Qt not selected, but hifi_qt.py failed to return a cmake path")

    pm = hifi_vcpkg.VcpkgRepo(args)
    if qtInstallPath is not None:
        pm.writeVar('QT_CMAKE_PREFIX_PATH', qtInstallPath)

    # Only allow one instance of the program to run at a time

    if qtInstallPath is not None:
        pm.writeVar('QT_CMAKE_PREFIX_PATH', qtInstallPath)

    # Only allow one instance of the program to run at a time
    with hifi_singleton.Singleton(pm.lockFile) as lock:

        with timer('Bootstraping'):
            if not pm.upToDate():
                pm.bootstrap()

        # Always write the tag, even if we changed nothing.  This 
        # allows vcpkg to reclaim disk space by identifying directories with
        # tags that haven't been touched in a long time
        pm.writeTag()

        # Grab our required dependencies:
        #  * build host tools, like spirv-cross and scribe
        #  * build client dependencies like openssl and nvtt
        with timer('Setting up dependencies'):
            pm.setupDependencies(qt=qtInstallPath)

        # wipe out the build directories (after writing the tag, since failure 
        # here shouldn't invalidate the vcpkg install)
        with timer('Cleaning builds'):
            pm.cleanBuilds()

        # If we're running in android mode, we also need to grab a bunch of additional binaries
        # (this logic is all migrated from the old setupDependencies tasks in gradle)
        if args.android:
            # Find the target location
            appPath = hifi_utils.scriptRelative('android/apps/' + args.android)
            # Copy the non-Qt libraries specified in the config in hifi_android.py
            hifi_android.copyAndroidLibs(pm.androidPackagePath, appPath)
            # Determine the Qt package path
            qtPath = os.path.join(pm.androidPackagePath, 'qt')
            hifi_android.QtPackager(appPath, qtPath).bundle()

        # Fixup the vcpkg cmake to not reset VCPKG_TARGET_TRIPLET
        pm.fixupCmakeScript()

        if not args.vcpkg_skip_clean:
            # Cleanup downloads and packages folders in vcpkg to make it smaller for CI
            pm.cleanupDevelopmentFiles()

        # Write the vcpkg config to the build directory last
        with timer('Writing configuration'):
            pm.writeConfig()

    logger.info('end')

print(sys.argv)
try:
    main()
except hifi_utils.SilentFatalError as fatal_ex:
    sys.exit(fatal_ex.exit_code)

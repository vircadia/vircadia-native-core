import os
import hashlib
import platform
import shutil
import ssl
import subprocess
import sys
import tarfile
import re
import urllib
import urllib.request
import zipfile
import tempfile
import time
import functools

print = functools.partial(print, flush=True)

ansi_colors = {
    'black' : 30,
    'red': 31,
    'green': 32,
    'yellow': 33,
    'blue': 34,
    'magenta': 35,
    'cyan': 36,
    'white': 37,
    'clear': 0
}

def scriptRelative(*paths):
    scriptdir = os.path.dirname(os.path.realpath(sys.argv[0]))
    result = os.path.join(scriptdir, *paths)
    result = os.path.realpath(result)
    result = os.path.normcase(result)
    return result


def recursiveFileList(startPath, excludeNamePattern=None ):
    result = []
    if os.path.isfile(startPath):
        result.append(startPath)
    elif os.path.isdir(startPath):
        for dirName, subdirList, fileList in os.walk(startPath):
            for fname in fileList:
                if excludeNamePattern and re.match(excludeNamePattern, fname):
                    continue
                result.append(os.path.realpath(os.path.join(startPath, dirName, fname)))
    result.sort()
    return result


def executeSubprocessCapture(processArgs):
    processResult = subprocess.run(processArgs, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if (0 != processResult.returncode):
        raise RuntimeError('Call to "{}" failed.\n\narguments:\n{}\n\nstdout:\n{}\n\nstderr:\n{}'.format(
            processArgs[0],
            ' '.join(processArgs[1:]), 
            processResult.stdout.decode('utf-8'),
            processResult.stderr.decode('utf-8')))
    return processResult.stdout.decode('utf-8')

def executeSubprocess(processArgs, folder=None, env=None):
    restoreDir = None
    if folder != None:
        restoreDir = os.getcwd()
        os.chdir(folder)

    process = subprocess.Popen(
        processArgs, stdout=sys.stdout, stderr=sys.stderr, env=env)
    process.wait()

    if (0 != process.returncode):
        raise RuntimeError('Call to "{}" failed.\n\narguments:\n{}\n'.format(
            processArgs[0],
            ' '.join(processArgs[1:]),
            ))

    if restoreDir != None:
        os.chdir(restoreDir)


def hashFile(file, hasher = hashlib.sha512()):
    with open(file, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hasher.update(chunk)
    return hasher.hexdigest()

# Assumes input files are in deterministic order
def hashFiles(filenames):
    hasher = hashlib.sha256()
    for filename in filenames:
        with open(filename, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hasher.update(chunk)
    return hasher.hexdigest()

def hashFolder(folder):
    filenames = recursiveFileList(folder)
    return hashFiles(filenames)

def downloadFile(url, hash=None, hasher=hashlib.sha512(), retries=3):
    for i in range(retries):
        tempFileName = None
        # OSX Python doesn't support SSL, so we need to bypass it.  
        # However, we still validate the downloaded file's sha512 hash
        if 'Darwin' == platform.system():
            tempFileDescriptor, tempFileName = tempfile.mkstemp()
            context = ssl._create_unverified_context()
            with urllib.request.urlopen(url, context=context) as response, open(tempFileDescriptor, 'wb') as tempFile:
                shutil.copyfileobj(response, tempFile)
        else:
            tempFileName, headers = urllib.request.urlretrieve(url)

        downloadHash = hashFile(tempFileName, hasher)
        # Verify the hash
        if hash is not None and hash != downloadHash:
            print("Try {}: Downloaded file {} hash {} does not match expected hash {} for url {}".format(i + 1, tempFileName, downloadHash, hash, url))
            os.remove(tempFileName)
            continue
        return tempFileName

    raise RuntimeError("Downloaded file hash {} does not match expected hash {} for\n{}".format(downloadHash, hash, url))


def downloadAndExtract(url, destPath, hash=None, hasher=hashlib.sha512(), isZip=False):
    tempFileName = downloadFile(url, hash, hasher)
    if isZip or ".zip" in url:
        with zipfile.ZipFile(tempFileName) as zip:
            zip.extractall(destPath)
    else:
        # Extract the archive
        with tarfile.open(tempFileName, 'r:*') as tgz:
            tgz.extractall(destPath)
    os.remove(tempFileName)

def readEnviromentVariableFromFile(buildRootDir, var):
    with open(os.path.join(buildRootDir, '_env', var + ".txt")) as fp:
        return fp.read()

class SilentFatalError(Exception):
    """Thrown when some sort of fatal condition happened, and we already reported it to the user.
    This excecption exists to give a chance to run any cleanup needed before exiting.

    It should be handled at the bottom of the call stack, where the only action is to call
    sys.exit(ex.exit_code)
    """
    def __init__(self, exit_code):
        self.exit_code = exit_code

def color(color_name):
    # Ideally we'd use the termcolor module, but this avoids adding it as a dependency.
    print("\033[1;{}m".format(ansi_colors[color_name]), end='')
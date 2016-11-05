#
# Tool to extract atp files from the asset server cache.
# Usage: python2 atp-extract.py -[lxa] [filename]
#
# cd into the c:\Users\BettySpaghetti\AppData\Roaming\High Fidelity\assignment-client\assets dir
# run 'python2 atp-extract.py -l' to list all files
# run 'python2 atp-extract.py -x file' to extract that particular file to the current directory.
# run 'python2 atp-extract.py -a' to extract all files.
#

import os, json, sys, shutil

def loadMapFile(filename):
    with open(filename, 'r') as f:
        return json.load(f)

def extractFile(assetMap, filename):
    if filename != None:
        assetFilename = assetMap.get("/" + filename)
        if assetFilename != None:
            dir = os.path.dirname(filename)
            if dir != "" and not os.path.exists(dir):
                os.makedirs(dir)
            shutil.copy("files/" + assetFilename, filename)
            return True
    return False

option = sys.argv[1]
if option == '-l':
    assetMap = loadMapFile("map.json")
    for key, value in assetMap.iteritems():
        print key[1:]
elif option == '-x':
    assetMap = loadMapFile("map.json")
    outputFilename = sys.argv[2]
    if not extractFile(assetMap, outputFilename):
        print("Error could not extract file: \"" + outputFilename + "\"")
elif option == '-a':
    assetMap = loadMapFile("map.json")
    for key, value in assetMap.iteritems():
        print("Extracting " + key[1:])
        extractFile(assetMap, key[1:])
else:
    print("unsuported option \"" + option + "\"")

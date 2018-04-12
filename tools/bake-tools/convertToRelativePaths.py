import json, os, sys, gzip

prefix = 'file:///~/'
MAP = {}
def createAssetMapping(assetDirectory):
    baseDirectory = os.path.basename(os.path.normpath(assetDirectory))
    for root, subfolder, filenames in os.walk(assetDirectory):
        for filename in filenames:
            if not filename.endswith('.ktx') or os.path.basename(root) == 'skyboxes':
                substring = os.path.commonprefix([assetDirectory, root])
                newPath = root.replace(substring, '');
                filePath = os.sep.join([newPath, filename])
                if filePath[0] == '\\':
                    filePath = filePath[1:]
                finalPath = prefix + baseDirectory + '/' + filePath
                finalPath = finalPath.replace('\\', '/')
                file = os.path.splitext(filename)[0]
                file = os.path.splitext(file)[0]
                MAP[file] = finalPath

def hasURL(prop):
    if "URL" in prop:
        return True
    return False


def handleURL(url):
    newUrl = url
    if "atp:" in url:
        baseFilename = os.path.basename(url)
        filename = os.path.splitext(baseFilename)[0]
        newUrl = MAP[filename]
    print url + ' -> ' + newUrl
    return newUrl

def handleOptions():
    option = sys.argv[1]
    if option == '--help' or option == '-h':
        print 'Usage: convertToRelativePaths.py  INPUT[json file you want to update the urls] INPUT[directory that the baked files are located in]'
        sys.exit()

def main():
    argsLength = len(sys.argv)
    if argsLength == 3:
        jsonFile = sys.argv[1]
        assetDirectory = sys.argv[2]
        inputFilename = os.path.basename(jsonFile);
        absPath = os.path.abspath(assetDirectory)
        finalPath = absPath.replace('\\', '/');
        gzipFile = finalPath + '/' + inputFilename + '.gz'
        print gzipFile
        createAssetMapping(assetDirectory)
        f = open(jsonFile)
        data = json.load(f)
        f.close()
        for entity in data['Entities']:
            for prop in entity:
                value = entity[prop]
                if hasURL(prop):
                   value = handleURL(value)
                if prop == "script":
                    value = handleURL(value)
                if prop == "textures":
                    try:
                        tmp = json.loads(value)
                        for index in tmp:
                            tmp[index] = handleURL(tmp[index])
                        value = json.dumps(tmp)
                    except:
                        value = handleURL(value)


                if  prop == "ambientLight":
                    for index in value:
                        value[index] = handleURL(value[index])

                if prop == "skybox":
                    for index in value:
                        value[index] = handleURL(value[index])

                if prop == "serverScripts":
                    value = handleURL(value)

                entity[prop] = value


        jsonString = json.dumps(data)
        jsonBytes= jsonString.encode('utf-8')
        with gzip.GzipFile(gzipFile, 'w') as fout:   # 4. gzip
            fout.write(jsonBytes)

    elif argsLength == 2:
        handleOptions()

main()

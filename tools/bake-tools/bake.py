import os, json, sys, shutil, subprocess, shlex, time
EXE = os.environ['HIFI_OVEN']

def getRelativePath(path1, path2, stop):
    parts1 = path1.split('/');
    parts2 = path2.split('/');
    if len(parts1) <= len(parts2):
        for part in parts1:
            if part != stop and part != '':
                index = parts2.index(part)
                parts2.pop(index)
    return os.path.join(*parts2)

def listFiles(directory, extension):
    items = os.listdir(directory)
    fileList = []
    for f in items:
        if f.endswith('.' + extension):
            fileList.append(f)
    return fileList

def camelCaseString(string):
    string = string.replace('-', ' ')
    return ''.join(x for x in string.title() if not x.isspace())

def groupFiles(originalDirectory, newDirectory, files):
    for file in files:
        newPath = os.sep.join([newDirectory, file])
        originalPath = os.sep.join([originalDirectory, file])
        shutil.move(originalPath, newPath)

def groupKTXFiles(directory, filePath):
    baseFile = os.path.basename(filePath)
    filename = os.path.splitext(baseFile)[0]
    camelCaseFileName =  camelCaseString(filename)
    path = os.sep.join([directory, camelCaseFileName])
    files = listFiles(directory, 'ktx')
    if len(files) > 0:
        createDirectory(path)
        groupFiles(directory, path, files)

        newFilePath = os.sep.join([path, baseFile+'.baked.fbx'])
        originalFilePath = os.sep.join([directory, baseFile+'.baked.fbx'])
        originalFilePath.strip()
        shutil.move(originalFilePath, newFilePath)

def bakeFile(filePath, outputDirectory, fileType):
    createDirectory(outputDirectory)
    cmd = EXE + ' -i ' + filePath + ' -o ' + outputDirectory + ' -t ' + fileType
    args = shlex.split(cmd)
    process = subprocess.Popen(cmd, stdout=False, stderr=False)
    process.wait()
    bakedFile = os.path.splitext(filePath)[0]
    if fileType == 'fbx':
        groupKTXFiles(outputDirectory, bakedFile)

def bakeFilesInDirectory(directory, outputDirectory):
    rootDirectory = os.path.basename(os.path.normpath(directory))
    for root, subFolders, filenames in os.walk(directory):
        for filename in filenames:
            appendPath = getRelativePath(directory, root, rootDirectory);
            name, ext = os.path.splitext('file.txt')
            if filename.endswith('.fbx'):
                filePath = os.sep.join([root, filename])
                absFilePath = os.path.abspath(filePath)
                outputFolder = os.path.join(outputDirectory, appendPath)
                print "Baking file: " + filename
                bakeFile(absFilePath, outputFolder, 'fbx')
            elif os.path.basename(root) == 'skyboxes':
                filePath = os.sep.join([root, filename])
                absFilePath = os.path.abspath(filePath)
                outputFolder = os.path.join(outputDirectory, appendPath)
                print "Baking file: " + filename
                bakeType = os.path.splitext(filename)[1][1:]
                bakeFile(absFilePath, outputFolder, bakeType)
            else:
                filePath = os.sep.join([root, filename])
                absFilePath = os.path.abspath(filePath)
                outputFolder = os.path.join(outputDirectory, appendPath)
                newFilePath = os.sep.join([outputFolder, filename])
                createDirectory(outputFolder)
                print "moving file: " + filename + " to: " + outputFolder
                shutil.copy(absFilePath, newFilePath)

def createDirectory(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)

def checkIfExeExists():
    if not os.path.isfile(EXE) and os.access(EXE, os.X_OK):
        print 'HIFI_OVEN evironment variable is not set'
        sys.exit()

def handleOptions():
    option = sys.argv[1]
    if option == '--help' or option == '-h':
        print 'Usage: bake.py INPUT_DIRECTORY[directory to bake] OUTPUT_DIRECTORY[directory to place backed files]'
        print 'Note: Output directory will be created if directory does not exist'
        sys.exit()

def main():
    argsLength = len(sys.argv)
    if argsLength == 3:
        checkIfExeExists()
        rootDirectory = sys.argv[1]
        outputDirectory = os.path.abspath(sys.argv[2])
        createDirectory(outputDirectory)
        bakeFilesInDirectory(rootDirectory, outputDirectory)
    elif argsLength == 2:
        handleOptions()

main()

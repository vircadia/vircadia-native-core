import os
import json
import shutil
import sys

def scriptRelative(*paths):
    scriptdir = os.path.dirname(os.path.realpath(sys.argv[0]))
    result = os.path.join(scriptdir, *paths)
    result = os.path.realpath(result)
    result = os.path.normcase(result)
    return result



class FrameProcessor:
    def __init__(self, filename):
        self.filename = filename
        dir, name = os.path.split(self.filename)
        self.dir = dir
        self.ktxDir = os.path.join(self.dir, 'ktx')
        os.makedirs(self.ktxDir, exist_ok=True)
        self.resDir = scriptRelative("../interface/resources")

        if (name.endswith(".json")):
            self.name = name[0:-5]
        else:
            self.name = name
            self.filename = self.filename + '.json'

        with open(self.filename, 'r') as f:
            self.json = json.load(f)


    def processKtx(self, texture):
        if texture is None: return
        if not 'ktxFile' in texture: return
        sourceKtx = texture['ktxFile']
        if sourceKtx.startswith(':'):
            sourceKtx = os.path.join(self.resDir, sourceKtx[3:])
        sourceKtxDir, sourceKtxName = os.path.split(sourceKtx)
        destKtx = os.path.join(self.ktxDir, sourceKtxName)
        if not os.path.isfile(destKtx):
            shutil.copy(sourceKtx, destKtx)
        newValue = 'ktx/' + sourceKtxName
        texture['ktxFile'] = newValue
        

    def process(self):
        for texture in self.json['textures']:
            self.processKtx(texture)

        with open(self.filename, 'w') as f:
            json.dump(self.json, f, indent=2)

fp = FrameProcessor("D:/Frames/20190110_1635.json")
fp.process()


#C:\Users\bdavi\git\hifi\interface\resources\meshes
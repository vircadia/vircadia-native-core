//
//  ClipboardScriptingInterface.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Scriptable interface for the Application clipboard
//

#ifndef __interface__Clipboard__
#define __interface__Clipboard__

#include <QObject>
#include <VoxelDetail.h>

class ClipboardScriptingInterface : public QObject {
    Q_OBJECT
public:
    ClipboardScriptingInterface();

public slots:
    void cutVoxels(float x, float y, float z, float s);
    void copyVoxels(float x, float y, float z, float s);
    void pasteVoxels(float x, float y, float z, float s);
    void deleteVoxels(float x, float y, float z, float s);

    void exportVoxels(float x, float y, float z, float s);
    void importVoxels();
};

#endif // __interface__Clipboard__

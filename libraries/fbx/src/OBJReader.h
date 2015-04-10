

#include "FBXReader.h"

FBXGeometry readOBJ(const QByteArray& model, const QVariantHash& mapping);
FBXGeometry readOBJ(QIODevice* device, const QVariantHash& mapping);
void fbxDebugDump(const FBXGeometry& fbxgeo);
void setMeshPartDefaults(FBXMeshPart &meshPart, QString materialID);

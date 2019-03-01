#ifndef hifi_AvatarProjectFile_h
#define hifi_AvatarProjectFile_h

#include <QObject>

class ProjectFilePath {
    Q_GADGET;
public:
    QString absolutePath;
    QString relativePath;
};

#endif // hifi_AvatarProjectFile_h

//
//  FileLogger.h
//  hifi
//
//  Created by Stojce Slavkovski on 12/22/13.
//
//

#ifndef hifi_FileLogger_h
#define hifi_FileLogger_h

#include "AbstractLoggerInterface.h"

class FileLogger : public AbstractLoggerInterface {
    Q_OBJECT

public:
    FileLogger();

    virtual void addMessage(QString);
    virtual QStringList getLogData(QString);

private:
    QStringList _lines;

};

#endif

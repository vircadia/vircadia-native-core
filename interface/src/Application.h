//
//  Application.h
//  interface
//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Application__
#define __interface__Application__

#include <QApplication>

class Application : public QApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);

public slots:
    void pair();
};

#endif /* defined(__interface__Application__) */

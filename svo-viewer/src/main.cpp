//
//  main.cpp
//
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include "svo-viewer-config.h"

#include "svoviewer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	SvoViewer svoV(argc, argv);
	return svoV.exec();
}

//
//  TestingDialog.cpp
//  interface/src/ui
//
//  Created by Ryan Jones on 12/3/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <ScriptEngines.h>

#include "ui/TestingDialog.h"
#include "Application.h"

TestingDialog::TestingDialog(QWidget* parent) :
	QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint)
{
	DependencyManager::get<ScriptEngines>()->loadOneScript(qApp->applicationDirPath() + testRunnerRelativePath);
	this->setWindowTitle(windowLabel);
}

TestingDialog::~TestingDialog() {
	// TODO: Clean up here?
}

void TestingDialog::reject() {
	this->QDialog::close();
}

void TestingDialog::closeEvent(QCloseEvent* event) {
	this->QDialog::closeEvent(event);
	emit closed();
}

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

#include "ScriptEngines.h"

#include "ui/TestingDialog.h"
#include "Application.h"

TestingDialog::TestingDialog(QWidget* parent) :
	QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(windowLabel);

	_console = new JSConsole(this);
	_console->setFixedHeight(TESTING_CONSOLE_HEIGHT);

	auto _engines = DependencyManager::get<ScriptEngines>();
	_engine = _engines->loadScript(qApp->applicationDirPath() + testRunnerRelativePath);
	_console->setScriptEngine(_engine);
	connect(_engine, &ScriptEngine::finished, this, &TestingDialog::onTestingFinished);
}

TestingDialog::~TestingDialog() {
	delete _console;
}

void TestingDialog::onTestingFinished(const QString& scriptPath) {
	_engine = NULL;
	_console->setScriptEngine(NULL);
}

#ifndef hifi_TestingDialog_h
#define hifi_TestingDialog_h

#include <QDialog>

const QString windowLabel = "Testing Dialog";
const QString testRunnerRelativePath = "/scripts/developer/tests/bindUnitTest.js";

class TestingDialog : public QDialog {
	Q_OBJECT
public:
	TestingDialog(QWidget* parent);
	~TestingDialog();

signals:
	void closed();

public slots:
	void reject() override;

protected:
	void closeEvent(QCloseEvent*) override;
};

#endif
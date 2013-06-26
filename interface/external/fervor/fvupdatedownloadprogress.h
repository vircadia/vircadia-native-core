#ifndef FVUPDATEDOWNLOADPROGRESS_H
#define FVUPDATEDOWNLOADPROGRESS_H

#include <QWidget>
#include "ui_fvupdatedownloadprogress.h"

class FvUpdateDownloadProgress : public QWidget
{
	Q_OBJECT

public:
	FvUpdateDownloadProgress(QWidget *parent = 0);
	~FvUpdateDownloadProgress();

public slots:
	void downloadProgress ( qint64 bytesReceived, qint64 bytesTotal );
	void close();

private:
	Ui::FvUpdateDownloadProgress ui;
};

#endif // FVUPDATEDOWNLOADPROGRESS_H

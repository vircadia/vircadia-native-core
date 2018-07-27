//
//  MismatchWindow.cpp
//
//  Created by Nissim Hadar on 9 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "MismatchWindow.h"

#include <QtCore/QFileInfo>

#include <cmath>

MismatchWindow::MismatchWindow(QWidget *parent) : QDialog(parent) {
    setupUi(this);

    expectedImage->setScaledContents(true);
    resultImage->setScaledContents(true);
    diffImage->setScaledContents(true);
}

QPixmap MismatchWindow::computeDiffPixmap(QImage expectedImage, QImage resultImage) {
	// This is an optimization, as QImage.setPixel() is embarrassingly slow
    unsigned char* buffer = new unsigned char[expectedImage.height() * expectedImage.width() * 3];

	// loop over each pixel
    for (int y = 0; y < expectedImage.height(); ++y) {
        for (int x = 0; x < expectedImage.width(); ++x) {
            QRgb pixelP = expectedImage.pixel(QPoint(x, y));
            QRgb pixelQ = resultImage.pixel(QPoint(x, y));

            // Convert to luminance
            double p = R_Y * qRed(pixelP) + G_Y * qGreen(pixelP) + B_Y * qBlue(pixelP);
            double q = R_Y * qRed(pixelQ) + G_Y * qGreen(pixelQ) + B_Y * qBlue(pixelQ);

            // The intensity value is modified to increase the brightness of the displayed image
            double absoluteDifference = fabs(p - q) / 255.0;
            double modifiedDifference = sqrt(absoluteDifference);

            int difference = (int)(modifiedDifference * 255.0);

            buffer[3 * (x + y * expectedImage.width()) + 0] = difference;
            buffer[3 * (x + y * expectedImage.width()) + 1] = difference;
            buffer[3 * (x + y * expectedImage.width()) + 2] = difference;
        }
    }

    QImage diffImage(buffer, expectedImage.width(), expectedImage.height(), QImage::Format_RGB888);
    QPixmap resultPixmap = QPixmap::fromImage(diffImage);

    delete[] buffer;

    return resultPixmap;
}

void MismatchWindow::setTestFailure(TestFailure testFailure) {
    errorLabel->setText("Similarity: " + QString::number(testFailure._error));

    imagePath->setText("Path to test: " + testFailure._pathname);

    expectedFilename->setText(testFailure._expectedImageFilename);
    resultFilename->setText(testFailure._actualImageFilename);

    QPixmap expectedPixmap = QPixmap(testFailure._pathname + testFailure._expectedImageFilename);
    QPixmap actualPixmap = QPixmap(testFailure._pathname + testFailure._actualImageFilename);

    _diffPixmap = computeDiffPixmap(
        QImage(testFailure._pathname + testFailure._expectedImageFilename), 
        QImage(testFailure._pathname + testFailure._actualImageFilename)
    );

    expectedImage->setPixmap(expectedPixmap);
    resultImage->setPixmap(actualPixmap);
    diffImage->setPixmap(_diffPixmap);
}

void MismatchWindow::on_passTestButton_clicked() {
    _userResponse = USER_RESPONSE_PASS;
    close();
}

void MismatchWindow::on_failTestButton_clicked() {
    _userResponse = USE_RESPONSE_FAIL;
    close();
}

void MismatchWindow::on_abortTestsButton_clicked() {
    _userResponse = USER_RESPONSE_ABORT;
    close();
}

QPixmap MismatchWindow::getComparisonImage() {
    return _diffPixmap;
}

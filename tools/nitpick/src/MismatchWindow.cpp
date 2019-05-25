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

QPixmap MismatchWindow::computeDiffPixmap(const QImage& expectedImage, const QImage& resultImage) {
    // Create an empty difference image if the images differ in size
    if (expectedImage.height() != resultImage.height() || expectedImage.width() != resultImage.width()) {
        return QPixmap();
    }

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

void MismatchWindow::setTestResult(const TestResult& testResult) {
    errorLabel->setText("Similarity: " + QString::number(testResult._errorGlobal) + "  (worst tile: " + QString::number(testResult._errorLocal) + ")");

    imagePath->setText("Path to test: " + testResult._pathname);

    expectedFilename->setText(testResult._expectedImageFilename);
    resultFilename->setText(testResult._actualImageFilename);

    QPixmap expectedPixmap = QPixmap(testResult._pathname + testResult._expectedImageFilename);
    QPixmap actualPixmap = QPixmap(testResult._pathname + testResult._actualImageFilename);

    _diffPixmap = computeDiffPixmap(
        QImage(testResult._pathname + testResult._expectedImageFilename), 
        QImage(testResult._pathname + testResult._actualImageFilename)
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

QPixmap MismatchWindow::getSSIMResultsImage(const SSIMResults& ssimResults) {
    // This is an optimization, as QImage.setPixel() is embarrassingly slow
    const int ELEMENT_SIZE { 8 };
    const int WIDTH{ ssimResults.width * ELEMENT_SIZE };
    const int HEIGHT{ ssimResults.height * ELEMENT_SIZE };

    unsigned char* buffer = new unsigned char[WIDTH * HEIGHT * 3];


    // loop over each SSIM result
    for (int y = 0; y < ssimResults.height; ++y) {
        for (int x = 0; x < ssimResults.width; ++x) {
            double scaledResult = (ssimResults.results[x * ssimResults.height + y] + 1.0) / (2.0);
            //double scaledResult = (ssimResults.results[x * ssimResults.height + y] - ssimResults.min) / (ssimResults.max - ssimResults.min);
            // Create a square
            for (int yy = 0; yy < ELEMENT_SIZE; ++yy) {
                for (int xx = 0; xx < ELEMENT_SIZE; ++xx) {
                    buffer[(xx  + yy * WIDTH + x * ELEMENT_SIZE + y * WIDTH * ELEMENT_SIZE) * 3 + 0] = 255 * (1.0 - scaledResult);   // R
                    buffer[(xx  + yy * WIDTH + x * ELEMENT_SIZE + y * WIDTH * ELEMENT_SIZE) * 3 + 1] = 255 * scaledResult;           // G
                    buffer[(xx  + yy * WIDTH + x * ELEMENT_SIZE + y * WIDTH * ELEMENT_SIZE) * 3 + 2] = 0;                            // B
                }
            }
        }
    }

    QImage image(buffer, WIDTH, HEIGHT, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image);

    delete[] buffer;

    return pixmap;
}

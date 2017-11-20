//
//  ITKImageComparer.cpp
//
//  Created by Nissim Hadar on 18 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ITKImageComparer.h"

#include <itkRGBPixel.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

float ITKImageComparer::compareImages(QString file1, QString file2) const {
    using PixelType = itk::RGBPixel<unsigned char>;
    using ImageType = itk::Image<PixelType, 2>;

    using ReaderType = itk::ImageFileReader<ImageType>;
    using WriterType = itk::ImageFileWriter<ImageType>;

    ReaderType::Pointer reader = ReaderType::New();
    WriterType::Pointer writer = WriterType::New();

    reader->SetFileName(file1.toStdString().c_str());
    writer->SetFileName("D:/pics/loni.jpg");

    ImageType::Pointer image = reader->GetOutput();
    writer->SetInput(image);

    writer->Update();

    return 0.0;
}

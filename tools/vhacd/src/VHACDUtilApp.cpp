//
//  VHACDUtil.h
//  tools/vhacd/src
//
//  Created by Seth Alves on 3/5/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCommandLineParser>
#include <VHACD.h>
#include "VHACDUtilApp.h"
#include "VHACDUtil.h"

using namespace std;
using namespace VHACD;



QString formatFloat(double n) {
    // limit precision to 6, but don't output trailing zeros.
    QString s = QString::number(n, 'f', 6);
    while (s.endsWith("0")) {
        s.remove(s.size() - 1, 1);
    }
    if (s.endsWith(".")) {
        s.remove(s.size() - 1, 1);
    }
    return s;
}


bool writeOBJ(QString outFileName, QVector<QVector<VHACD::IVHACD::ConvexHull>>& meshList, bool outputOneMesh) {
    QFile file(outFileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Unable to write to " << outFileName;
        return false;
    }

    QTextStream out(&file);

    unsigned int pointStartOffset = 0;

    foreach (QVector<VHACD::IVHACD::ConvexHull> hulls, meshList) {
        unsigned int nth = 0;
        foreach (VHACD::IVHACD::ConvexHull hull, hulls) {
            out << "g hull-" << nth++ << "\n";
            for (unsigned int i = 0; i < hull.m_nPoints; i++) {
                out << "v ";
                out << formatFloat(hull.m_points[i*3]) << " ";
                // swap y and z because up is 3rd value in OBJ
                out << formatFloat(hull.m_points[i*3+2]) << "\n";
                out << formatFloat(hull.m_points[i*3+1]) << " ";
            }
            for (unsigned int i = 0; i < hull.m_nTriangles; i++) {
                out << "f ";
                out << hull.m_triangles[i*3] + 1 + pointStartOffset << " ";
                out << hull.m_triangles[i*3+1] + 1 + pointStartOffset  << " ";
                out << hull.m_triangles[i*3+2] + 1 + pointStartOffset  << "\n";
            }
            out << "\n";
            pointStartOffset += hull.m_nPoints;
        }
    }

    return true;
}




VHACDUtilApp::VHACDUtilApp(int argc, char* argv[]) :
    QCoreApplication(argc, argv)
{
    vector<int> triangles; // array of indexes
    vector<float> points;  // array of coordinates 
    vhacd::VHACDUtil vUtil;
    vhacd::LoadFBXResults fbx; //mesh data from loaded fbx file
    vhacd::ComputeResults results; // results after computing vhacd
    VHACD::IVHACD::Parameters params;
    vhacd::ProgressCallback pCallBack;


    // parse command-line
    QCommandLineParser parser;
    parser.setApplicationDescription("High Fidelity Object Decomposer");
    parser.addHelpOption();

    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption outputOneMeshOption("1", "output hulls as single mesh");
    parser.addOption(outputOneMeshOption);

    const QCommandLineOption inputFilenameOption("i", "input file", "filename.fbx");
    parser.addOption(inputFilenameOption);

    const QCommandLineOption outputFilenameOption("o", "output file", "filename.obj");
    parser.addOption(outputFilenameOption);


    if (!parser.parse(QCoreApplication::arguments())) {
        qCritical() << parser.errorText() << endl;
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (parser.isSet(helpOption)) {
        parser.showHelp();
        Q_UNREACHABLE();
    }


    bool outputOneMesh = parser.isSet(outputOneMeshOption);

    QString inputFilename;
    // check for an assignment pool passed on the command line or in the config
    if (parser.isSet(inputFilenameOption)) {
        inputFilename = parser.value(inputFilenameOption);
    }

    QString outputFilename;
    // check for an assignment pool passed on the command line or in the config
    if (parser.isSet(outputFilenameOption)) {
        outputFilename = parser.value(outputFilenameOption);
    }


    if (inputFilename == "") {
        cerr << "input filename is required.";
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (outputFilename == "") {
        cerr << "output filename is required.";
        parser.showHelp();
        Q_UNREACHABLE();
    }


    //set parameters for V-HACD
    params.m_callback = &pCallBack; //progress callback
    params.m_resolution              = 100000; // 100000
    params.m_depth                   = 20; // 20
    params.m_concavity               = 0.001; // 0.001
    params.m_delta                   = 0.01; // 0.05
    params.m_planeDownsampling       = 4; // 4
    params.m_convexhullDownsampling  = 4; // 4
    params.m_alpha                   = 0.05; // 0.05  // controls the bias toward clipping along symmetry planes
    params.m_beta                    = 0.05; // 0.05
    params.m_gamma                   = 0.0005; // 0.0005
    params.m_pca                     = 0; // 0  enable/disable normalizing the mesh before applying the convex decomposition
    params.m_mode                    = 0; // 0: voxel-based (recommended), 1: tetrahedron-based
    params.m_maxNumVerticesPerCH     = 64; // 64
    params.m_minVolumePerCH          = 0.00001; // 0.0001
    params.m_callback                = 0; // 0
    params.m_logger                  = 0; // 0
    params.m_convexhullApproximation = true; // true
    params.m_oclAcceleration         = true; // true



    // load the mesh 

    auto begin = std::chrono::high_resolution_clock::now();
    if (!vUtil.loadFBX(inputFilename, &fbx)){
        cout << "Error in opening FBX file....";
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto loadDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();

    //perform vhacd computation
    begin = std::chrono::high_resolution_clock::now();


    if (!vUtil.computeVHACD(&fbx, params, &results)){
        cout << "Compute Failed...";
    }
    end = std::chrono::high_resolution_clock::now();
    auto computeDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();

    int totalVertices = 0;
    for (int i = 0; i < fbx.meshCount; i++){
        totalVertices += fbx.perMeshVertices.at(i).count();
    }

    int totalTriangles = 0;
    for (int i = 0; i < fbx.meshCount; i++){
        totalTriangles += fbx.perMeshTriangleIndices.at(i).count();
    }

    int totalHulls = 0;
    QVector<int> hullCounts = results.convexHullsCountList;
    for (int i = 0; i < results.meshCount; i++){
        totalHulls += hullCounts.at(i);
    }
    cout << endl << "Summary of V-HACD Computation..................." << endl;
    cout << "File Path          : " << inputFilename.toStdString() << endl;
    cout << "Number Of Meshes   : " << fbx.meshCount << endl;
    cout << "Processed Meshes   : " << results.meshCount << endl;
    cout << "Total vertices     : " << totalVertices << endl;
    cout << "Total Triangles    : " << totalTriangles << endl;
    cout << "Total Convex Hulls : " << totalHulls << endl;
    cout << "Total FBX load time: " << (double)loadDuration / 1000000000.00 << " seconds" << endl;
    cout << "V-HACD Compute time: " << (double)computeDuration / 1000000000.00 << " seconds" << endl;
    cout << endl << "Summary per convex hull ........................" << endl <<endl;
    for (int i = 0; i < results.meshCount; i++) {
        cout << "Mesh : " << i + 1 << endl;
        QVector<VHACD::IVHACD::ConvexHull> chList = results.convexHullList.at(i);
        cout << "\t" << "Number Of Hulls : " << chList.count() << endl;

        for (int j = 0; j < results.convexHullList.at(i).count(); j++){
            cout << "\tHUll : " << j + 1 << endl;
            cout << "\t\tNumber Of Points    : " << chList.at(j).m_nPoints << endl;
            cout << "\t\tNumber Of Triangles : " << chList.at(j).m_nTriangles << endl;
        }
    }

    writeOBJ(outputFilename, results.convexHullList, outputOneMesh);
}

VHACDUtilApp::~VHACDUtilApp() {
}

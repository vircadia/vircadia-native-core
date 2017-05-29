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
#include <Trace.h>
#include <VHACD.h>
#include "VHACDUtilApp.h"
#include "VHACDUtil.h"
#include "PathUtils.h"

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


bool VHACDUtilApp::writeOBJ(QString outFileName, FBXGeometry& geometry, bool outputCentimeters, int whichMeshPart) {
    QFile file(outFileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "unable to write to" << outFileName;
        _returnCode = VHACD_RETURN_CODE_FAILURE_TO_WRITE;
        return false;
    }

    QTextStream out(&file);
    if (outputCentimeters) {
        out << "# This file uses centimeters as units\n\n";
    }

    unsigned int nth = 0;

    // vertex indexes in obj files span the entire file
    // vertex indexes in a mesh span just that mesh

    int vertexIndexOffset = 0;

    foreach (const FBXMesh& mesh, geometry.meshes) {
        bool verticesHaveBeenOutput = false;
        foreach (const FBXMeshPart &meshPart, mesh.parts) {
            if (whichMeshPart >= 0 && nth != (unsigned int) whichMeshPart) {
                nth++;
                continue;
            }

            if (!verticesHaveBeenOutput) {
                for (int i = 0; i < mesh.vertices.size(); i++) {
                    glm::vec4 v = mesh.modelTransform * glm::vec4(mesh.vertices[i], 1.0f);
                    out << "v ";
                    out << formatFloat(v[0]) << " ";
                    out << formatFloat(v[1]) << " ";
                    out << formatFloat(v[2]) << "\n";
                }
                verticesHaveBeenOutput = true;
            }

            out << "g hull-" << nth++ << "\n";
            int triangleCount = meshPart.triangleIndices.size() / 3;
            for (int i = 0; i < triangleCount; i++) {
                out << "f ";
                out << vertexIndexOffset + meshPart.triangleIndices[i*3] + 1 << " ";
                out << vertexIndexOffset + meshPart.triangleIndices[i*3+1] + 1  << " ";
                out << vertexIndexOffset + meshPart.triangleIndices[i*3+2] + 1  << "\n";
            }
            out << "\n";
        }

        if (verticesHaveBeenOutput) {
            vertexIndexOffset += mesh.vertices.size();
        }
    }

    return true;
}




VHACDUtilApp::VHACDUtilApp(int argc, char* argv[]) :
    QCoreApplication(argc, argv)
{
    vhacd::VHACDUtil vUtil;

    DependencyManager::set<tracing::Tracer>();

    // parse command-line
    QCommandLineParser parser;
    parser.setApplicationDescription("High Fidelity Object Decomposer");
    parser.addHelpOption();

    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption verboseOutput("v", "verbose output");
    parser.addOption(verboseOutput);

    const QCommandLineOption splitOption("split", "split input-file into one mesh per output-file");
    parser.addOption(splitOption);

    const QCommandLineOption fattenFacesOption("f", "fatten faces");
    parser.addOption(fattenFacesOption);

    const QCommandLineOption generateHullsOption("g", "output convex hull approximations");
    parser.addOption(generateHullsOption);

    const QCommandLineOption inputFilenameOption("i", "input file", "filename.fbx");
    parser.addOption(inputFilenameOption);

    const QCommandLineOption outputFilenameOption("o", "output file", "filename.obj");
    parser.addOption(outputFilenameOption);

    const QCommandLineOption outputCentimetersOption("c", "output units are centimeters");
    parser.addOption(outputCentimetersOption);

    const QCommandLineOption minimumMeshSizeOption("m", "minimum mesh (diagonal) size to consider", "0");
    parser.addOption(minimumMeshSizeOption);

    const QCommandLineOption maximumMeshSizeOption("x", "maximum mesh (diagonal) size to consider", "0");
    parser.addOption(maximumMeshSizeOption);

    const QCommandLineOption vHacdResolutionOption("resolution", "Maximum number of voxels generated during the "
                                                   "voxelization stage (range=10,000-16,000,000)", "100000");
    parser.addOption(vHacdResolutionOption);

    const QCommandLineOption vHacdDepthOption("depth", "Maximum number of clipping stages. During each split stage, parts "
                                              "with a concavity higher than the user defined threshold are clipped "
                                              "according the \"best\" clipping plane (range=1-32)", "20");
    parser.addOption(vHacdDepthOption);


    const QCommandLineOption vHacdAlphaOption("alpha", "Controls the bias toward clipping along symmetry "
                                              "planes (range=0.0-1.0)", "0.05");
    parser.addOption(vHacdAlphaOption);

    const QCommandLineOption vHacdBetaOption("beta", "Controls the bias toward clipping along revolution "
                                             "axes (range=0.0-1.0)", "0.05");
    parser.addOption(vHacdBetaOption);

    const QCommandLineOption vHacdGammaOption("gamma", "Controls the maximum allowed concavity during the "
                                              "merge stage (range=0.0-1.0)", "0.00125");
    parser.addOption(vHacdGammaOption);

    const QCommandLineOption vHacdDeltaOption("delta", "Controls the bias toward maximaxing local "
                                              "concavity (range=0.0-1.0)", "0.05");
    parser.addOption(vHacdDeltaOption);

    const QCommandLineOption vHacdConcavityOption("concavity", "Maximum allowed concavity (range=0.0-1.0)", "0.0025");
    parser.addOption(vHacdConcavityOption);

    const QCommandLineOption vHacdPlanedownsamplingOption("planedownsampling", "Controls the granularity of the search for"
                                                          " the \"best\" clipping plane (range=1-16)", "4");
    parser.addOption(vHacdPlanedownsamplingOption);

    const QCommandLineOption vHacdConvexhulldownsamplingOption("convexhulldownsampling", "Controls the precision of the "
                                                               "convex-hull generation process during the clipping "
                                                               "plane selection stage (range=1-16)", "4");
    parser.addOption(vHacdConvexhulldownsamplingOption);

    // pca
    // mode

    const QCommandLineOption vHacdMaxVerticesPerCHOption("maxvertices", "Controls the maximum number of triangles per "
                                                         "convex-hull (range=4-1024)", "64");
    parser.addOption(vHacdMaxVerticesPerCHOption);

    // minVolumePerCH
    // convexhullApproximation


    if (!parser.parse(QCoreApplication::arguments())) {
        qCritical() << parser.errorText() << endl;
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (parser.isSet(helpOption)) {
        parser.showHelp();
        Q_UNREACHABLE();
    }

    bool verbose = parser.isSet(verboseOutput);
    vUtil.setVerbose(verbose);

    bool outputCentimeters = parser.isSet(outputCentimetersOption);
    bool fattenFaces = parser.isSet(fattenFacesOption);
    bool generateHulls = parser.isSet(generateHullsOption);
    bool splitModel = parser.isSet(splitOption);


    QString inputFilename;
    if (parser.isSet(inputFilenameOption)) {
        inputFilename = parser.value(inputFilenameOption);
    }

    QString outputFilename;
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

    float minimumMeshSize = 0.0f;
    if (parser.isSet(minimumMeshSizeOption)) {
        minimumMeshSize = parser.value(minimumMeshSizeOption).toFloat();
    }

    float maximumMeshSize = 0.0f;
    if (parser.isSet(maximumMeshSizeOption)) {
        maximumMeshSize = parser.value(maximumMeshSizeOption).toFloat();
    }

    int vHacdResolution = 100000;
    if (parser.isSet(vHacdResolutionOption)) {
        vHacdResolution = parser.value(vHacdResolutionOption).toInt();
    }

    int vHacdDepth = 20;
    if (parser.isSet(vHacdDepthOption)) {
        vHacdDepth = parser.value(vHacdDepthOption).toInt();
    }

    float vHacdAlpha = 0.05f;
    if (parser.isSet(vHacdAlphaOption)) {
        vHacdAlpha = parser.value(vHacdAlphaOption).toFloat();
    }

    float vHacdBeta = 0.05f;
    if (parser.isSet(vHacdBetaOption)) {
        vHacdBeta = parser.value(vHacdBetaOption).toFloat();
    }

    float vHacdGamma = 0.00125f;
    if (parser.isSet(vHacdGammaOption)) {
        vHacdGamma = parser.value(vHacdGammaOption).toFloat();
    }

    float vHacdDelta = 0.05f;
    if (parser.isSet(vHacdDeltaOption)) {
        vHacdDelta = parser.value(vHacdDeltaOption).toFloat();
    }

    float vHacdConcavity = 0.0025f;
    if (parser.isSet(vHacdConcavityOption)) {
        vHacdConcavity = parser.value(vHacdConcavityOption).toFloat();
    }

    int vHacdPlanedownsampling = 4;
    if (parser.isSet(vHacdPlanedownsamplingOption)) {
        vHacdPlanedownsampling = parser.value(vHacdPlanedownsamplingOption).toInt();
    }

    int vHacdConvexhulldownsampling = 4;
    if (parser.isSet(vHacdConvexhulldownsamplingOption)) {
        vHacdConvexhulldownsampling = parser.value(vHacdConvexhulldownsamplingOption).toInt();
    }

    int vHacdMaxVerticesPerCH = 64;
    if (parser.isSet(vHacdMaxVerticesPerCHOption)) {
        vHacdMaxVerticesPerCH = parser.value(vHacdMaxVerticesPerCHOption).toInt();
    }

    if (!splitModel && !generateHulls && !fattenFaces) {
        cerr << "\nNothing to do!  Use -g or -f or --split\n\n";
        parser.showHelp();
        Q_UNREACHABLE();
    }

    // load the mesh
    FBXGeometry fbx;
    auto begin = std::chrono::high_resolution_clock::now();
    if (!vUtil.loadFBX(inputFilename, fbx)){
        _returnCode = VHACD_RETURN_CODE_FAILURE_TO_READ;
        return;
    }
    auto end = std::chrono::high_resolution_clock::now();

    if (verbose) {
        auto loadDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
        const double NANOSECS_PER_SECOND = 1.0e9;
        qDebug() << "load time =" << (double)loadDuration / NANOSECS_PER_SECOND << "seconds";
    }

    if (splitModel) {
        QVector<QString> infileExtensions = {"fbx", "obj"};
        QString baseFileName = fileNameWithoutExtension(outputFilename, infileExtensions);
        int count = 0;
        foreach (const FBXMesh& mesh, fbx.meshes) {
            foreach (const FBXMeshPart &meshPart, mesh.parts) {
                QString outputFileName = baseFileName + "-" + QString::number(count) + ".obj";
                writeOBJ(outputFileName, fbx, outputCentimeters, count);
                count++;
                (void)meshPart; // quiet warning
            }
        }
    }

    if (generateHulls) {
        VHACD::IVHACD::Parameters params;
        vhacd::ProgressCallback progressCallback;

        //set parameters for V-HACD
        if (verbose) {
            params.m_callback = &progressCallback; //progress callback
        } else {
            params.m_callback = nullptr;
        }
        params.m_resolution = vHacdResolution;
        params.m_depth = vHacdDepth;
        params.m_concavity = vHacdConcavity;
        params.m_delta = vHacdDelta;
        params.m_planeDownsampling = vHacdPlanedownsampling;
        params.m_convexhullDownsampling = vHacdConvexhulldownsampling;
        params.m_alpha = vHacdAlpha;
        params.m_beta = vHacdBeta;
        params.m_gamma = vHacdGamma;
        params.m_pca = 0; // 0  enable/disable normalizing the mesh before applying the convex decomposition
        params.m_mode = 0; // 0: voxel-based (recommended), 1: tetrahedron-based
        params.m_maxNumVerticesPerCH = vHacdMaxVerticesPerCH;
        params.m_minVolumePerCH = 0.0001; // 0.0001
        params.m_logger = nullptr;
        params.m_convexhullApproximation = true; // true
        params.m_oclAcceleration = true; // true

        //perform vhacd computation
        if (verbose) {
            qDebug() << "running V-HACD algorithm ...";
        }
        begin = std::chrono::high_resolution_clock::now();

        FBXGeometry result;
        bool success = vUtil.computeVHACD(fbx, params, result, minimumMeshSize, maximumMeshSize);

        end = std::chrono::high_resolution_clock::now();
        auto computeDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
        if (verbose) {
            qDebug() << "run time =" << (double)computeDuration / 1000000000.00 << " seconds";
        }

        if (!success) {
            if (verbose) {
                qDebug() << "failed to convexify model";
            }
            _returnCode = VHACD_RETURN_CODE_FAILURE_TO_CONVEXIFY;
            return;
        }

        int totalVertices = 0;
        int totalTriangles = 0;
        foreach (const FBXMesh& mesh, result.meshes) {
            totalVertices += mesh.vertices.size();
            foreach (const FBXMeshPart &meshPart, mesh.parts) {
                totalTriangles += meshPart.triangleIndices.size() / 3;
                // each quad was made into two triangles
                totalTriangles += 2 * meshPart.quadIndices.size() / 4;
            }
        }

        if (verbose) {
            int totalHulls = result.meshes[0].parts.size();
            qDebug() << "output file =" << outputFilename;
            qDebug() << "vertices =" << totalVertices;
            qDebug() << "triangles =" << totalTriangles;
            qDebug() << "hulls =" << totalHulls;
        }

        writeOBJ(outputFilename, result, outputCentimeters);
    }

    if (fattenFaces) {
        FBXGeometry newFbx;
        FBXMesh result;

        // count the mesh-parts
        unsigned int meshCount = 0;
        foreach (const FBXMesh& mesh, fbx.meshes) {
            meshCount += mesh.parts.size();
        }

        result.modelTransform = glm::mat4(); // Identity matrix
        foreach (const FBXMesh& mesh, fbx.meshes) {
            vUtil.fattenMesh(mesh, fbx.offset, result);
        }

        newFbx.meshes.append(result);
        writeOBJ(outputFilename, newFbx, outputCentimeters);
    }
}

VHACDUtilApp::~VHACDUtilApp() {
}

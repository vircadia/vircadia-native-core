//
//  VoxelTree.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <glm/gtc/noise.hpp>

//#include <QtCore/QDebug>
#include <QImage>
#include <QRgb>


#include "VoxelTree.h"
#include "Tags.h"

// Voxel Specific operations....

VoxelTree::VoxelTree(bool shouldReaverage) : Octree(shouldReaverage) {
    _rootNode = createNewElement();
}

VoxelTreeElement* VoxelTree::createNewElement(unsigned char * octalCode) const {
    VoxelSystem* voxelSystem = NULL;
    if (_rootNode) {
        voxelSystem = ((VoxelTreeElement*)_rootNode)->getVoxelSystem();
    }
    VoxelTreeElement* newElement = new VoxelTreeElement(octalCode); 
    newElement->setVoxelSystem(voxelSystem);
    return newElement;
}


void VoxelTree::deleteVoxelAt(float x, float y, float z, float s) {
    deleteOctreeElementAt(x, y, z, s);
}

VoxelTreeElement* VoxelTree::getVoxelAt(float x, float y, float z, float s) const {
    return (VoxelTreeElement*)getOctreeElementAt(x, y, z, s);
}

void VoxelTree::createVoxel(float x, float y, float z, float s,
                            unsigned char red, unsigned char green, unsigned char blue, bool destructive) {
                            
    unsigned char* voxelData = pointToVoxel(x,y,z,s,red,green,blue);

    //int length = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(voxelData)) + BYTES_PER_COLOR;
    //printf("createVoxel()...");
    //outputBufferBits(voxelData,length);

    this->readCodeColorBufferToTree(voxelData, destructive);
    delete[] voxelData;
}


void VoxelTree::createLine(glm::vec3 point1, glm::vec3 point2, float unitSize, rgbColor color, bool destructive) {
    glm::vec3 distance = point2 - point1;
    glm::vec3 items = distance / unitSize;
    int maxItems = std::max(items.x, std::max(items.y, items.z));
    glm::vec3 increment = distance * (1.0f/ maxItems);
    glm::vec3 pointAt = point1;
    for (int i = 0; i <= maxItems; i++ ) {
        pointAt += increment;
        createVoxel(pointAt.x, pointAt.y, pointAt.z, unitSize, color[0], color[1], color[2], destructive);
    }
}

void VoxelTree::createSphere(float radius, float xc, float yc, float zc, float voxelSize,
                             bool solid, creationMode mode, bool destructive, bool debug) {

    bool wantColorRandomizer = (mode == RANDOM);
    bool wantNaturalSurface  = (mode == NATURAL);
    bool wantNaturalColor    = (mode == NATURAL);

    // About the color of the sphere... we're going to make this sphere be a mixture of two colors
    // in NATURAL mode, those colors will be green dominant and blue dominant. In GRADIENT mode we
    // will randomly pick which color family red, green, or blue to be dominant. In RANDOM mode we
    // ignore these dominant colors and make every voxel a completely random color.
    unsigned char r1, g1, b1, r2, g2, b2;

    if (wantNaturalColor) {
        r1 = r2 = b2 = g1 = 0;
        b1 = g2 = 255;
    } else if (!wantColorRandomizer) {
        unsigned char dominantColor1 = randIntInRange(1, 3); //1=r, 2=g, 3=b dominant
        unsigned char dominantColor2 = randIntInRange(1, 3);

        if (dominantColor1 == dominantColor2) {
            dominantColor2 = dominantColor1 + 1 % 3;
        }

        r1 = (dominantColor1 == 1) ? randIntInRange(200, 255) : randIntInRange(40, 100);
        g1 = (dominantColor1 == 2) ? randIntInRange(200, 255) : randIntInRange(40, 100);
        b1 = (dominantColor1 == 3) ? randIntInRange(200, 255) : randIntInRange(40, 100);
        r2 = (dominantColor2 == 1) ? randIntInRange(200, 255) : randIntInRange(40, 100);
        g2 = (dominantColor2 == 2) ? randIntInRange(200, 255) : randIntInRange(40, 100);
        b2 = (dominantColor2 == 3) ? randIntInRange(200, 255) : randIntInRange(40, 100);
    }

    // We initialize our rgb to be either "grey" in case of randomized surface, or
    // the average of the gradient, in the case of the gradient sphere.
    unsigned char red   = wantColorRandomizer ? 128 : (r1 + r2) / 2; // average of the colors
    unsigned char green = wantColorRandomizer ? 128 : (g1 + g2) / 2;
    unsigned char blue  = wantColorRandomizer ? 128 : (b1 + b2) / 2;

    // I want to do something smart like make these inside circles with bigger voxels, but this doesn't seem to work.
    float thisVoxelSize = voxelSize; // radius / 2.0f;
    float thisRadius = 0.0;
    if (!solid) {
        thisRadius = radius; // just the outer surface
        thisVoxelSize = voxelSize;
    }

    // If you also iterate form the interior of the sphere to the radius, making
    // larger and larger spheres you'd end up with a solid sphere. And lots of voxels!
    bool lastLayer = false;
    while (!lastLayer) {
        lastLayer = (thisRadius + (voxelSize * 2.0) >= radius);

        // We want to make sure that as we "sweep" through our angles we use a delta angle that voxelSize
        // small enough to not skip any voxels we can calculate theta from our desired arc length
        //      lenArc = ndeg/360deg * 2pi*R  --->  lenArc = theta/2pi * 2pi*R
        //      lenArc = theta*R ---> theta = lenArc/R ---> theta = g/r
        float angleDelta = (thisVoxelSize / thisRadius);

        if (debug) {
            int percentComplete = 100 * (thisRadius/radius);
            qDebug("percentComplete=%d\n",percentComplete);
        }

        for (float theta=0.0; theta <= 2 * M_PI; theta += angleDelta) {
            for (float phi=0.0; phi <= M_PI; phi += angleDelta) {
                bool naturalSurfaceRendered = false;
                float x = xc + thisRadius * cos(theta) * sin(phi);
                float y = yc + thisRadius * sin(theta) * sin(phi);
                float z = zc + thisRadius * cos(phi);

                // if we're on the outer radius, then we do a couple of things differently.
                // 1) If we're in NATURAL mode we will actually draw voxels from our surface outward (from the surface) up
                //    some random height. This will give our sphere some contours.
                // 2) In all modes, we will use our "outer" color to draw the voxels. Otherwise we will use the average color
                if (lastLayer) {
                    if (false && debug) {
                        qDebug("adding candy shell: theta=%f phi=%f thisRadius=%f radius=%f\n",
                                 theta, phi, thisRadius,radius);
                    }
                    switch (mode) {
                    case RANDOM: {
                        red   = randomColorValue(165);
                        green = randomColorValue(165);
                        blue  = randomColorValue(165);
                    } break;
                    case GRADIENT: {
                        float gradient = (phi / M_PI);
                        red   = r1 + ((r2 - r1) * gradient);
                        green = g1 + ((g2 - g1) * gradient);
                        blue  = b1 + ((b2 - b1) * gradient);
                    } break;
                    case NATURAL: {
                        glm::vec3 position = glm::vec3(theta,phi,radius);
                        float perlin = glm::perlin(position) + .25f * glm::perlin(position * 4.f)
                                + .125f * glm::perlin(position * 16.f);
                        float gradient = (1.0f + perlin)/ 2.0f;
                        red   = (unsigned char)std::min(255, std::max(0, (int)(r1 + ((r2 - r1) * gradient))));
                        green = (unsigned char)std::min(255, std::max(0, (int)(g1 + ((g2 - g1) * gradient))));
                        blue  = (unsigned char)std::min(255, std::max(0, (int)(b1 + ((b2 - b1) * gradient))));
                        if (debug) {
                            qDebug("perlin=%f gradient=%f color=(%d,%d,%d)\n",perlin, gradient, red, green, blue);
                        }
                        } break;
                    }
                    if (wantNaturalSurface) {
                        // for natural surfaces, we will render up to 16 voxel's above the surface of the sphere
                        glm::vec3 position = glm::vec3(theta,phi,radius);
                        float perlin = glm::perlin(position) + .25f * glm::perlin(position * 4.f)
                                + .125f * glm::perlin(position * 16.f);
                        float gradient = (1.0f + perlin)/ 2.0f;

                        int height = (4 * gradient)+1; // make it at least 4 thick, so we get some averaging
                        float subVoxelScale = thisVoxelSize;
                        for (int i = 0; i < height; i++) {
                            x = xc + (thisRadius + i * subVoxelScale) * cos(theta) * sin(phi);
                            y = yc + (thisRadius + i * subVoxelScale) * sin(theta) * sin(phi);
                            z = zc + (thisRadius + i * subVoxelScale) * cos(phi);
                            this->createVoxel(x, y, z, subVoxelScale, red, green, blue, destructive);
                        }
                        naturalSurfaceRendered = true;
                    }
                }
                if (!naturalSurfaceRendered) {
                    this->createVoxel(x, y, z, thisVoxelSize, red, green, blue, destructive);
                }
            }
        }
        thisRadius += thisVoxelSize;
        thisVoxelSize = std::max(voxelSize, thisVoxelSize / 2.0f);
    }
}


class NodeChunkArgs {
public:
    VoxelTree* thisVoxelTree;
    glm::vec3 nudgeVec;
    VoxelEditPacketSender* voxelEditSenderPtr;
    int childOrder[NUMBER_OF_CHILDREN];
};

float findNewLeafSize(const glm::vec3& nudgeAmount, float leafSize) {
    // we want the smallest non-zero and non-negative new leafSize
    float newLeafSizeX = fabs(fmod(nudgeAmount.x, leafSize));
    float newLeafSizeY = fabs(fmod(nudgeAmount.y, leafSize));
    float newLeafSizeZ = fabs(fmod(nudgeAmount.z, leafSize));

    float newLeafSize = leafSize;
    if (newLeafSizeX) {
        newLeafSize = fmin(newLeafSize, newLeafSizeX);
    }
    if (newLeafSizeY) {
        newLeafSize = fmin(newLeafSize, newLeafSizeY);
    }
    if (newLeafSizeZ) {
        newLeafSize = fmin(newLeafSize, newLeafSizeZ);
    }
    return newLeafSize;
}

void reorderChildrenForNudge(void* extraData) {
    NodeChunkArgs* args = (NodeChunkArgs*)extraData;
    glm::vec3 nudgeVec = args->nudgeVec;
    int lastToNudgeVote[NUMBER_OF_CHILDREN] = { 0 };

    const int POSITIVE_X_ORDERING[4] = {0, 1, 2, 3};
    const int NEGATIVE_X_ORDERING[4] = {4, 5, 6, 7};
    const int POSITIVE_Y_ORDERING[4] = {0, 1, 4, 5};
    const int NEGATIVE_Y_ORDERING[4] = {2, 3, 6, 7};
    const int POSITIVE_Z_ORDERING[4] = {0, 2, 4, 6};
    const int NEGATIVE_Z_ORDERING[4] = {1, 3, 5, 7};

    if (nudgeVec.x > 0) {
        for (int i = 0; i < NUMBER_OF_CHILDREN / 2; i++) {
            lastToNudgeVote[POSITIVE_X_ORDERING[i]]++;
        }
    } else if (nudgeVec.x < 0) {
        for (int i = 0; i < NUMBER_OF_CHILDREN / 2; i++) {
            lastToNudgeVote[NEGATIVE_X_ORDERING[i]]++;
        }
    }
    if (nudgeVec.y > 0) {
        for (int i = 0; i < NUMBER_OF_CHILDREN / 2; i++) {
            lastToNudgeVote[POSITIVE_Y_ORDERING[i]]++;
        }
    } else if (nudgeVec.y < 0) {
        for (int i = 0; i < NUMBER_OF_CHILDREN / 2; i++) {
            lastToNudgeVote[NEGATIVE_Y_ORDERING[i]]++;
        }
    }
    if (nudgeVec.z > 0) {
        for (int i = 0; i < NUMBER_OF_CHILDREN / 2; i++) {
            lastToNudgeVote[POSITIVE_Z_ORDERING[i]]++;
        }
    } else if (nudgeVec.z < 0) {
        for (int i = 0; i < NUMBER_OF_CHILDREN / 2; i++) {
            lastToNudgeVote[NEGATIVE_Z_ORDERING[i]]++;
        }
    }

    int nUncountedVotes = NUMBER_OF_CHILDREN;

    while (nUncountedVotes > 0) {
        int maxNumVotes = 0;
        int maxVoteIndex = -1;
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            if (lastToNudgeVote[i] > maxNumVotes) {
                maxNumVotes = lastToNudgeVote[i];
                maxVoteIndex = i;
            } else if (lastToNudgeVote[i] == maxNumVotes && maxVoteIndex == -1) {
                maxVoteIndex = i;
            }
        }
        lastToNudgeVote[maxVoteIndex] = -1;
        args->childOrder[nUncountedVotes - 1] = maxVoteIndex;
        nUncountedVotes--;
    }
}

bool VoxelTree::nudgeCheck(OctreeElement* element, void* extraData) {
    VoxelTreeElement* voxel = (VoxelTreeElement*)element;
    if (voxel->isLeaf()) {
        // we have reached the deepest level of elements/voxels
        // now there are two scenarios
        // 1) this element's size is <= the minNudgeAmount
        //      in which case we will simply call nudgeLeaf on this leaf
        // 2) this element's size is still not <= the minNudgeAmount
        //      in which case we need to break this leaf down until the leaf sizes are <= minNudgeAmount

        NodeChunkArgs* args = (NodeChunkArgs*)extraData;

        // get octal code of this element
        const unsigned char* octalCode = element->getOctalCode();

        // get voxel position/size
        VoxelPositionSize unNudgedDetails;
        voxelDetailsForCode(octalCode, unNudgedDetails);

        // find necessary leaf size
        float newLeafSize = findNewLeafSize(args->nudgeVec, unNudgedDetails.s);

        // check to see if this unNudged element can be nudged
        if (unNudgedDetails.s <= newLeafSize) {
            args->thisVoxelTree->nudgeLeaf(voxel, extraData);
            return false;
        } else {
            // break the current leaf into smaller chunks
            args->thisVoxelTree->chunkifyLeaf(voxel);
        }
    }
    return true;
}

void VoxelTree::chunkifyLeaf(VoxelTreeElement* element) {
    // because this function will continue being called recursively
    // we only need to worry about breaking this specific leaf down
    if (!element->isColored()) {
        return;
    }
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        element->addChildAtIndex(i);
        element->getChildAtIndex(i)->setColor(element->getColor());
    }
}

// This function is called to nudge the leaves of a tree, given that the
// nudge amount is >= to the leaf scale.
void VoxelTree::nudgeLeaf(VoxelTreeElement* element, void* extraData) {
    NodeChunkArgs* args = (NodeChunkArgs*)extraData;

    // get octal code of this element
    const unsigned char* octalCode = element->getOctalCode();

    // get voxel position/size
    VoxelPositionSize unNudgedDetails;
    voxelDetailsForCode(octalCode, unNudgedDetails);
    
    VoxelDetail voxelDetails;
    voxelDetails.x = unNudgedDetails.x;
    voxelDetails.y = unNudgedDetails.y;
    voxelDetails.z = unNudgedDetails.z;
    voxelDetails.s = unNudgedDetails.s;
    voxelDetails.red = element->getColor()[RED_INDEX];
    voxelDetails.green = element->getColor()[GREEN_INDEX];
    voxelDetails.blue = element->getColor()[BLUE_INDEX];
    glm::vec3 nudge = args->nudgeVec;

    // delete the old element
    args->voxelEditSenderPtr->sendVoxelEditMessage(PACKET_TYPE_VOXEL_ERASE, voxelDetails);

    // nudge the old element
    voxelDetails.x += nudge.x;
    voxelDetails.y += nudge.y;
    voxelDetails.z += nudge.z;

    // create a new voxel in its stead
    args->voxelEditSenderPtr->sendVoxelEditMessage(PACKET_TYPE_VOXEL_SET_DESTRUCTIVE, voxelDetails);
}

// Recurses voxel element with an operation function
void VoxelTree::recurseNodeForNudge(VoxelTreeElement* element, RecurseOctreeOperation operation, void* extraData) {
    NodeChunkArgs* args = (NodeChunkArgs*)extraData;
    if (operation(element, extraData)) {
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            // XXXBHG cleanup!!
            VoxelTreeElement* child = (VoxelTreeElement*)element->getChildAtIndex(args->childOrder[i]);
            if (child) {
                recurseNodeForNudge(child, operation, extraData);
            }
        }
    }
}

void VoxelTree::nudgeSubTree(VoxelTreeElement* elementToNudge, const glm::vec3& nudgeAmount, VoxelEditPacketSender& voxelEditSender) {
    if (nudgeAmount == glm::vec3(0, 0, 0)) {
        return;
    }

    NodeChunkArgs args;
    args.thisVoxelTree = this;
    args.nudgeVec = nudgeAmount;
    args.voxelEditSenderPtr = &voxelEditSender;
    reorderChildrenForNudge(&args);

    recurseNodeForNudge(elementToNudge, nudgeCheck, &args);
}





bool VoxelTree::readFromSquareARGB32Pixels(const char* filename) {
    emit importProgress(0);
    int minAlpha = INT_MAX;

    QImage pngImage = QImage(filename);

    for (int i = 0; i < pngImage.width(); ++i) {
        for (int j = 0; j < pngImage.height(); ++j) {
            minAlpha = std::min(qAlpha(pngImage.pixel(i, j)) , minAlpha);
        }
    }

    int maxSize = std::max(pngImage.width(), pngImage.height());

    int scale = 1;
    while (maxSize > scale) {scale *= 2;}
    float size = 1.0f / scale;

    emit importSize(size * pngImage.width(), 1.0f, size * pngImage.height());

    QRgb pixel;
    int minNeighborhoodAlpha;

    for (int i = 0; i < pngImage.width(); ++i) {
        for (int j = 0; j < pngImage.height(); ++j) {
            emit importProgress((100 * (i * pngImage.height() + j)) /
                                (pngImage.width() * pngImage.height()));

            pixel = pngImage.pixel(i, j);
            minNeighborhoodAlpha = qAlpha(pixel) - 1;

            if (i != 0) {
                minNeighborhoodAlpha = std::min(minNeighborhoodAlpha, qAlpha(pngImage.pixel(i - 1, j)));
            }
            if (j != 0) {
                minNeighborhoodAlpha = std::min(minNeighborhoodAlpha, qAlpha(pngImage.pixel(i, j - 1)));
            }
            if (i < pngImage.width() - 1) {
                minNeighborhoodAlpha = std::min(minNeighborhoodAlpha, qAlpha(pngImage.pixel(i + 1, j)));
            }
            if (j < pngImage.height() - 1) {
                minNeighborhoodAlpha = std::min(minNeighborhoodAlpha, qAlpha(pngImage.pixel(i, j + 1)));
            }

            while (qAlpha(pixel) > minNeighborhoodAlpha) {
                ++minNeighborhoodAlpha;
                createVoxel(i * size,
                            (minNeighborhoodAlpha - minAlpha) * size,
                            j * size,
                            size,
                            qRed(pixel),
                            qGreen(pixel),
                            qBlue(pixel),
                            true);
            }

        }
    }

    emit importProgress(100);
    return true;
}

bool VoxelTree::readFromSchematicFile(const char *fileName) {
    _stopImport = false;
    emit importProgress(0);

    std::stringstream ss;
    int err = retrieveData(std::string(fileName), ss);
    if (err && ss.get() != TAG_Compound) {
        qDebug("[ERROR] Invalid schematic file.\n");
        return false;
    }

    ss.get();
    TagCompound schematics(ss);
    if (!schematics.getBlocksId() || !schematics.getBlocksData()) {
        qDebug("[ERROR] Invalid schematic data.\n");
        return false;
    }

    int max = (schematics.getWidth() > schematics.getLength()) ? schematics.getWidth() : schematics.getLength();
    max = (max > schematics.getHeight()) ? max : schematics.getHeight();

    int scale = 1;
    while (max > scale) {scale *= 2;}
    float size = 1.0f / scale;

    emit importSize(size * schematics.getWidth(),
                    size * schematics.getHeight(),
                    size * schematics.getLength());

    int create = 1;
    int red = 128, green = 128, blue = 128;
    int count = 0;

    for (int y = 0; y < schematics.getHeight(); ++y) {
        for (int z = 0; z < schematics.getLength(); ++z) {
            emit importProgress((int) 100 * (y * schematics.getLength() + z) / (schematics.getHeight() * schematics.getLength()));

            for (int x = 0; x < schematics.getWidth(); ++x) {
                if (_stopImport) {
                    qDebug("[DEBUG] Canceled import at %d voxels.\n", count);
                    _stopImport = false;
                    return true;
                }

                int pos  = ((y * schematics.getLength()) + z) * schematics.getWidth() + x;
                int id   = schematics.getBlocksId()[pos];
                int data = schematics.getBlocksData()[pos];

                create = 1;
                computeBlockColor(id, data, red, green, blue, create);

                switch (create) {
                    case 1:
                        createVoxel(size * x, size * y, size * z, size, red, green, blue, true);
                        ++count;
                        break;
                    case 2:
                        switch (data) {
                            case 0:
                                createVoxel(size * x + size / 2, size * y + size / 2, size * z           , size / 2, red, green, blue, true);
                                createVoxel(size * x + size / 2, size * y + size / 2, size * z + size / 2, size / 2, red, green, blue, true);
                                break;
                            case 1:
                                createVoxel(size * x           , size * y + size / 2, size * z           , size / 2, red, green, blue, true);
                                createVoxel(size * x           , size * y + size / 2, size * z + size / 2, size / 2, red, green, blue, true);
                                break;
                            case 2:
                                createVoxel(size * x           , size * y + size / 2, size * z + size / 2, size / 2, red, green, blue, true);
                                createVoxel(size * x + size / 2, size * y + size / 2, size * z + size / 2, size / 2, red, green, blue, true);
                                break;
                            case 3:
                                createVoxel(size * x           , size * y + size / 2, size * z           , size / 2, red, green, blue, true);
                                createVoxel(size * x + size / 2, size * y + size / 2, size * z           , size / 2, red, green, blue, true);
                                break;
                        }
                        count += 2;
                        // There's no break on purpose.
                    case 3:
                        createVoxel(size * x           , size * y, size * z           , size / 2, red, green, blue, true);
                        createVoxel(size * x + size / 2, size * y, size * z           , size / 2, red, green, blue, true);
                        createVoxel(size * x           , size * y, size * z + size / 2, size / 2, red, green, blue, true);
                        createVoxel(size * x + size / 2, size * y, size * z + size / 2, size / 2, red, green, blue, true);
                        count += 4;
                        break;
                }
            }
        }
    }

    emit importProgress(100);
    qDebug("Created %d voxels from minecraft import.\n", count);

    return true;
}

class ReadCodeColorBufferToTreeArgs {
public:
    const unsigned char* codeColorBuffer;
    int lengthOfCode;
    bool destructive;
    bool pathChanged;
};

void VoxelTree::readCodeColorBufferToTree(const unsigned char* codeColorBuffer, bool destructive) {
    ReadCodeColorBufferToTreeArgs args;
    args.codeColorBuffer = codeColorBuffer;
    args.lengthOfCode = numberOfThreeBitSectionsInCode(codeColorBuffer);
    args.destructive = destructive;
    args.pathChanged = false;
    VoxelTreeElement* node = getRoot();
    readCodeColorBufferToTreeRecursion(node, args);
}

void VoxelTree::readCodeColorBufferToTreeRecursion(VoxelTreeElement* node, ReadCodeColorBufferToTreeArgs& args) {
    int lengthOfNodeCode = numberOfThreeBitSectionsInCode(node->getOctalCode());

    // Since we traverse the tree in code order, we know that if our code
    // matches, then we've reached  our target node.
    if (lengthOfNodeCode == args.lengthOfCode) {
        // we've reached our target -- we might have found our node, but that node might have children.
        // in this case, we only allow you to set the color if you explicitly asked for a destructive
        // write.
        if (!node->isLeaf() && args.destructive) {
            // if it does exist, make sure it has no children
            for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
                node->deleteChildAtIndex(i);
            }
        } else {
            if (!node->isLeaf()) {
                qDebug("WARNING! operation would require deleting children, add Voxel ignored!\n ");
            }
        }

        // If we get here, then it means, we either had a true leaf to begin with, or we were in
        // destructive mode and we deleted all the child trees. So we can color.
        if (node->isLeaf()) {
            // give this node its color
            int octalCodeBytes = bytesRequiredForCodeLength(args.lengthOfCode);

            nodeColor newColor;
            memcpy(newColor, args.codeColorBuffer + octalCodeBytes, SIZE_OF_COLOR_DATA);
            newColor[SIZE_OF_COLOR_DATA] = 1;
            node->setColor(newColor);

            // It's possible we just reset the node to it's exact same color, in
            // which case we don't consider this to be dirty...
            if (node->isDirty()) {
                // track our tree dirtiness
                _isDirty = true;
                // track that path has changed
                args.pathChanged = true;
            }
        }
        return;
    }

    // Ok, we know we haven't reached our target node yet, so keep looking
    //printOctalCode(args.codeColorBuffer);
    int childIndex = branchIndexWithDescendant(node->getOctalCode(), args.codeColorBuffer);
    VoxelTreeElement* childNode = node->getChildAtIndex(childIndex);

    // If the branch we need to traverse does not exist, then create it on the way down...
    if (!childNode) {
        childNode = node->addChildAtIndex(childIndex);
    }

    // recurse...
    readCodeColorBufferToTreeRecursion(childNode, args);

    // Unwinding...

    // If the lower level did some work, then we need to let this node know, so it can
    // do any bookkeeping it wants to, like color re-averaging, time stamp marking, etc
    if (args.pathChanged) {
        node->handleSubtreeChanged(this);
    }
}

bool VoxelTree::handlesEditPacketType(PACKET_TYPE packetType) const {
    // we handle these types of "edit" packets
    switch (packetType) {
        case PACKET_TYPE_VOXEL_SET:
        case PACKET_TYPE_VOXEL_SET_DESTRUCTIVE:
        case PACKET_TYPE_VOXEL_ERASE:
            return true;
    }
    return false;
}

int VoxelTree::processEditPacketData(PACKET_TYPE packetType, unsigned char* packetData, int packetLength,
                    unsigned char* editData, int maxLength, Node* senderNode) {

    int processedBytes = 0;
    // we handle these types of "edit" packets
    switch (packetType) {
        case PACKET_TYPE_VOXEL_SET:
        case PACKET_TYPE_VOXEL_SET_DESTRUCTIVE: {
            bool destructive = (packetType == PACKET_TYPE_VOXEL_SET_DESTRUCTIVE);
            int octets = numberOfThreeBitSectionsInCode(editData, maxLength);
            
            if (octets == OVERFLOWED_OCTCODE_BUFFER) {
                printf("WARNING! Got voxel edit record that would overflow buffer in numberOfThreeBitSectionsInCode(), ");
                printf("bailing processing of packet!\n");
                return processedBytes;
            }

            const int COLOR_SIZE_IN_BYTES = 3;
            int voxelCodeSize = bytesRequiredForCodeLength(octets);
            int voxelDataSize = voxelCodeSize + COLOR_SIZE_IN_BYTES;

            if (voxelDataSize > maxLength) {
                printf("WARNING! Got voxel edit record that would overflow buffer, bailing processing of packet!\n");
                printf("bailing processing of packet!\n");
                return processedBytes;
            }
            
            readCodeColorBufferToTree(editData, destructive);
            
            return voxelDataSize;
        } break;
            
        case PACKET_TYPE_VOXEL_ERASE:
            processRemoveOctreeElementsBitstream((unsigned char*)packetData, packetLength);
            return maxLength;
    }
    return processedBytes;
}


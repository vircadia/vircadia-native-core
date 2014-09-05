//
//  VoxelTree.cpp
//  libraries/voxels/src
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <algorithm>

#include <QDebug>
#include <QImage>
#include <QRgb>

#include "VoxelTree.h"
#include "Tags.h"

// Voxel Specific operations....

VoxelTree::VoxelTree(bool shouldReaverage) : Octree(shouldReaverage) 
{
    _rootElement = createNewElement();
}

VoxelTreeElement* VoxelTree::createNewElement(unsigned char * octalCode) {
    VoxelSystem* voxelSystem = NULL;
    if (_rootElement) {
        voxelSystem = (static_cast<VoxelTreeElement*>(_rootElement))->getVoxelSystem();
    }
    VoxelTreeElement* newElement = new VoxelTreeElement(octalCode);
    newElement->setVoxelSystem(voxelSystem);
    return newElement;
}


void VoxelTree::deleteVoxelAt(float x, float y, float z, float s) {
    deleteOctreeElementAt(x, y, z, s);
}

VoxelTreeElement* VoxelTree::getVoxelAt(float x, float y, float z, float s) const {
    return static_cast<VoxelTreeElement*>(getOctreeElementAt(x, y, z, s));
}

VoxelTreeElement* VoxelTree::getEnclosingVoxelAt(float x, float y, float z, float s) const {
    return static_cast<VoxelTreeElement*>(getOctreeEnclosingElementAt(x, y, z, s));
}

void VoxelTree::createVoxel(float x, float y, float z, float s,
                            unsigned char red, unsigned char green, unsigned char blue, bool destructive) {

    unsigned char* voxelData = pointToVoxel(x,y,z,s,red,green,blue);
    lockForWrite();
    readCodeColorBufferToTree(voxelData, destructive);
    unlock();
    delete[] voxelData;
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
        newLeafSize = std::min(newLeafSize, newLeafSizeX);
    }
    if (newLeafSizeY) {
        newLeafSize = std::min(newLeafSize, newLeafSizeY);
    }
    if (newLeafSizeZ) {
        newLeafSize = std::min(newLeafSize, newLeafSizeZ);
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
    args->voxelEditSenderPtr->sendVoxelEditMessage(PacketTypeVoxelErase, voxelDetails);

    // nudge the old element
    voxelDetails.x += nudge.x;
    voxelDetails.y += nudge.y;
    voxelDetails.z += nudge.z;

    // create a new voxel in its stead
    args->voxelEditSenderPtr->sendVoxelEditMessage(PacketTypeVoxelSetDestructive, voxelDetails);
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
        qDebug("[ERROR] Invalid schematic file.");
        return false;
    }

    ss.get();
    TagCompound schematics(ss);
    if (!schematics.getBlocksId() || !schematics.getBlocksData()) {
        qDebug("[ERROR] Invalid schematic data.");
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
                    qDebug("Canceled import at %d voxels.", count);
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
    qDebug("Created %d voxels from minecraft import.", count);

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
                qDebug("WARNING! operation would require deleting children, add Voxel ignored!");
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

bool VoxelTree::handlesEditPacketType(PacketType packetType) const {
    // we handle these types of "edit" packets
    switch (packetType) {
        case PacketTypeVoxelSet:
        case PacketTypeVoxelSetDestructive:
        case PacketTypeVoxelErase:
            return true;
        default:
            return false;
            
    }
}

const unsigned int REPORT_OVERFLOW_WARNING_INTERVAL = 100;
unsigned int overflowWarnings = 0;
int VoxelTree::processEditPacketData(PacketType packetType, const unsigned char* packetData, int packetLength,
                    const unsigned char* editData, int maxLength, const SharedNodePointer& node) {
    
    // we handle these types of "edit" packets
    switch (packetType) {
        case PacketTypeVoxelSet:
        case PacketTypeVoxelSetDestructive: {
            bool destructive = (packetType == PacketTypeVoxelSetDestructive);
            int octets = numberOfThreeBitSectionsInCode(editData, maxLength);

            if (octets == OVERFLOWED_OCTCODE_BUFFER) {
                overflowWarnings++;
                if (overflowWarnings % REPORT_OVERFLOW_WARNING_INTERVAL == 1) {
                    qDebug() << "WARNING! Got voxel edit record that would overflow buffer in numberOfThreeBitSectionsInCode()"
                                " [NOTE: this is warning number" << overflowWarnings << ", the next" << 
                                (REPORT_OVERFLOW_WARNING_INTERVAL-1) << "will be suppressed.]";
                    
                    QDebug debug = qDebug();
                    debug << "edit data contents:";
                    outputBufferBits(editData, maxLength, &debug);
                }
                return maxLength;
            }

            const int COLOR_SIZE_IN_BYTES = 3;
            int voxelCodeSize = bytesRequiredForCodeLength(octets);
            int voxelDataSize = voxelCodeSize + COLOR_SIZE_IN_BYTES;

            if (voxelDataSize > maxLength) {
                overflowWarnings++;
                if (overflowWarnings % REPORT_OVERFLOW_WARNING_INTERVAL == 1) {
                    qDebug() << "WARNING! Got voxel edit record that would overflow buffer."
                                " [NOTE: this is warning number" << overflowWarnings << ", the next" << 
                                (REPORT_OVERFLOW_WARNING_INTERVAL-1) << "will be suppressed.]";
                    
                    QDebug debug = qDebug();
                    debug << "edit data contents:";
                    outputBufferBits(editData, maxLength, &debug);
                }
                return maxLength;
            }

            readCodeColorBufferToTree(editData, destructive);

            return voxelDataSize;
        } break;

        case PacketTypeVoxelErase:
            processRemoveOctreeElementsBitstream((unsigned char*)packetData, packetLength);
            return maxLength;
        default:
            return 0;
    }
}

class VoxelTreeDebugOperator : public RecurseOctreeOperator {
public:
    virtual bool preRecursion(OctreeElement* element);
    virtual bool postRecursion(OctreeElement* element) { return true; }
};

bool VoxelTreeDebugOperator::preRecursion(OctreeElement* element) {
    VoxelTreeElement* treeElement = static_cast<VoxelTreeElement*>(element);
    qDebug() << "VoxelTreeElement [" << treeElement << ":" << treeElement->getAACube() << "]";
    qDebug() << "    isLeaf:" << treeElement->isLeaf();
    qDebug() << "    color:" << treeElement->getColor()[0] << ", "
                             << treeElement->getColor()[1] << ", " 
                             << treeElement->getColor()[2];
    return true;
}

void VoxelTree::dumpTree() {
    // First, look for the existing entity in the tree..
    VoxelTreeDebugOperator theOperator;
    recurseTreeWithOperator(&theOperator);
}


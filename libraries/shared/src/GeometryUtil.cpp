//
//  GeometryUtil.cpp
//  libraries/shared/src
//
//  Created by Andrzej Kapolka on 5/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

const float PolygonClip::TOP_OF_CLIPPING_WINDOW    =  1.0f;
const float PolygonClip::BOTTOM_OF_CLIPPING_WINDOW = -1.0f;
const float PolygonClip::LEFT_OF_CLIPPING_WINDOW   = -1.0f;
const float PolygonClip::RIGHT_OF_CLIPPING_WINDOW  =  1.0f;

const glm::vec2 PolygonClip::TOP_LEFT_CLIPPING_WINDOW       ( LEFT_OF_CLIPPING_WINDOW , TOP_OF_CLIPPING_WINDOW    );
const glm::vec2 PolygonClip::TOP_RIGHT_CLIPPING_WINDOW      ( RIGHT_OF_CLIPPING_WINDOW, TOP_OF_CLIPPING_WINDOW    );
const glm::vec2 PolygonClip::BOTTOM_LEFT_CLIPPING_WINDOW    ( LEFT_OF_CLIPPING_WINDOW , BOTTOM_OF_CLIPPING_WINDOW );
const glm::vec2 PolygonClip::BOTTOM_RIGHT_CLIPPING_WINDOW   ( RIGHT_OF_CLIPPING_WINDOW, BOTTOM_OF_CLIPPING_WINDOW );

void PolygonClip::clipToScreen(const glm::vec2* inputVertexArray, int inLength, glm::vec2*& outputVertexArray, int& outLength) {
    int tempLengthA = inLength;
    int tempLengthB;
    int maxLength = inLength * 2;
    glm::vec2* tempVertexArrayA = new glm::vec2[maxLength];
    glm::vec2* tempVertexArrayB = new glm::vec2[maxLength];
    
    // set up our temporary arrays
    memcpy(tempVertexArrayA, inputVertexArray, sizeof(glm::vec2) * inLength);
    
    // Left edge
    LineSegment2 edge;
    edge[0] = TOP_LEFT_CLIPPING_WINDOW;
    edge[1] = BOTTOM_LEFT_CLIPPING_WINDOW;
    // clip the array from tempVertexArrayA and copy end result to tempVertexArrayB
    sutherlandHodgmanPolygonClip(tempVertexArrayA, tempVertexArrayB, tempLengthA, tempLengthB, edge);
    // clean the array from tempVertexArrayA and copy cleaned result to tempVertexArrayA
    copyCleanArray(tempLengthA, tempVertexArrayA, tempLengthB, tempVertexArrayB);
    
    // Bottom Edge
    edge[0] = BOTTOM_LEFT_CLIPPING_WINDOW;
    edge[1] = BOTTOM_RIGHT_CLIPPING_WINDOW;
    // clip the array from tempVertexArrayA and copy end result to tempVertexArrayB
    sutherlandHodgmanPolygonClip(tempVertexArrayA, tempVertexArrayB, tempLengthA, tempLengthB, edge);
    // clean the array from tempVertexArrayA and copy cleaned result to tempVertexArrayA
    copyCleanArray(tempLengthA, tempVertexArrayA, tempLengthB, tempVertexArrayB);
    
    // Right Edge
    edge[0] = BOTTOM_RIGHT_CLIPPING_WINDOW;
    edge[1] = TOP_RIGHT_CLIPPING_WINDOW;
    // clip the array from tempVertexArrayA and copy end result to tempVertexArrayB
    sutherlandHodgmanPolygonClip(tempVertexArrayA, tempVertexArrayB, tempLengthA, tempLengthB, edge);
    // clean the array from tempVertexArrayA and copy cleaned result to tempVertexArrayA
    copyCleanArray(tempLengthA, tempVertexArrayA, tempLengthB, tempVertexArrayB);
    
    // Top Edge
    edge[0] = TOP_RIGHT_CLIPPING_WINDOW;
    edge[1] = TOP_LEFT_CLIPPING_WINDOW;
    // clip the array from tempVertexArrayA and copy end result to tempVertexArrayB
    sutherlandHodgmanPolygonClip(tempVertexArrayA, tempVertexArrayB, tempLengthA, tempLengthB, edge);
    // clean the array from tempVertexArrayA and copy cleaned result to tempVertexArrayA
    copyCleanArray(tempLengthA, tempVertexArrayA, tempLengthB, tempVertexArrayB);
    
    // copy final output to outputVertexArray
    outputVertexArray = tempVertexArrayA;
    outLength         = tempLengthA;

    // cleanup our unused temporary buffer...
    delete[] tempVertexArrayB;
    
    // Note: we don't delete tempVertexArrayA, because that's the caller's responsibility
}

void PolygonClip::sutherlandHodgmanPolygonClip(glm::vec2* inVertexArray, glm::vec2* outVertexArray, 
                                               int inLength, int& outLength,  const LineSegment2& clipBoundary) {
    glm::vec2 start, end; // Start, end point of current polygon edge
    glm::vec2 intersection;   // Intersection point with a clip boundary

    outLength = 0;
    start = inVertexArray[inLength - 1]; // Start with the last vertex in inVertexArray
    for (int j = 0; j < inLength; j++) {
        end = inVertexArray[j]; // Now start and end correspond to the vertices
        
        // Cases 1 and 4 - the endpoint is inside the boundary 
        if (pointInsideBoundary(end,clipBoundary)) {
            // Case 1 - Both inside
            if (pointInsideBoundary(start, clipBoundary)) {
                appendPoint(end, outLength, outVertexArray);
            } else { // Case 4 - end is inside, but start is outside
                segmentIntersectsBoundary(start, end, clipBoundary, intersection);
                appendPoint(intersection, outLength, outVertexArray);
                appendPoint(end, outLength, outVertexArray);
            }
        } else { // Cases 2 and 3 - end is outside
            if (pointInsideBoundary(start, clipBoundary))  { 
                // Cases 2 - start is inside, end is outside
                segmentIntersectsBoundary(start, end, clipBoundary, intersection);
                appendPoint(intersection, outLength, outVertexArray);
            } else {
                // Case 3 - both are outside, No action
            }
       } 
       start = end;  // Advance to next pair of vertices
    }
}

bool PolygonClip::pointInsideBoundary(const glm::vec2& testVertex, const LineSegment2& clipBoundary) {
    // bottom edge
    if (clipBoundary[1].x > clipBoundary[0].x) {
        if (testVertex.y >= clipBoundary[0].y) {
            return true;
        }
    }
    // top edge
    if (clipBoundary[1].x < clipBoundary[0].x) {
        if (testVertex.y <= clipBoundary[0].y) {
            return true;
        }
    }
    // right edge
    if (clipBoundary[1].y > clipBoundary[0].y) {
        if (testVertex.x <= clipBoundary[1].x) {
            return true;
        }
    }
    // left edge
    if (clipBoundary[1].y < clipBoundary[0].y) {
        if (testVertex.x >= clipBoundary[1].x) {
            return true;
        }
    }
    return false;
}

void PolygonClip::segmentIntersectsBoundary(const glm::vec2& first, const glm::vec2&  second,
                                           const LineSegment2& clipBoundary, glm::vec2& intersection) {
    // horizontal
    if (clipBoundary[0].y==clipBoundary[1].y) {
        intersection.y = clipBoundary[0].y;
        intersection.x = first.x + (clipBoundary[0].y - first.y) * (second.x - first.x) / (second.y - first.y);
    } else { // Vertical
        intersection.x = clipBoundary[0].x;
        intersection.y = first.y + (clipBoundary[0].x - first.x) * (second.y - first.y) / (second.x - first.x);
    }
}

void PolygonClip::appendPoint(glm::vec2 newVertex, int& outLength, glm::vec2* outVertexArray) {
    outVertexArray[outLength].x = newVertex.x;
    outVertexArray[outLength].y = newVertex.y;
    outLength++;
}

// The copyCleanArray() function sets the resulting polygon of the previous step up to be the input polygon for next step of the
// clipping algorithm. As the Sutherland-Hodgman algorithm is a polygon clipping algorithm, it does not handle line 
// clipping very well. The modification so that lines may be clipped as well as polygons is included in this function.
// when completed vertexArrayA will be ready for output and/or next step of clipping 
void PolygonClip::copyCleanArray(int& lengthA, glm::vec2* vertexArrayA, int& lengthB, glm::vec2* vertexArrayB) {
    // Fix lines: they will come back with a length of 3, from an original of length of 2
    if ((lengthA == 2) && (lengthB == 3)) {
        // The first vertex should be copied as is. 
        vertexArrayA[0] = vertexArrayB[0]; 
        // If the first two vertices of the "B" array are same, then collapse them down to be the 2nd vertex
        if (vertexArrayB[0].x == vertexArrayB[1].x)  {
            vertexArrayA[1] = vertexArrayB[2];
        } else { 
            // Otherwise the first vertex should be the same as third vertex
            vertexArrayA[1] = vertexArrayB[1];
        }
        lengthA=2;
    } else { 
        // for all other polygons, then just copy the vertexArrayB to vertextArrayA for next step
        lengthA = lengthB;
        for (int i = 0; i < lengthB; i++) {
            vertexArrayA[i] = vertexArrayB[i];
        }
    }
}

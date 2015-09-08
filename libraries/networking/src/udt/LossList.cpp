//
//  LossList.cpp
//  libraries/networking/src/udt
//
//  Created by Clement on 7/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LossList.h"

#include "ControlPacket.h"

using namespace udt;
using namespace std;

void LossList::append(SequenceNumber seq) {
    Q_ASSERT_X(_lossList.empty() || (_lossList.back().second < seq), "LossList::append(SequenceNumber)",
               "SequenceNumber appended is not greater than the last SequenceNumber in the list");
    
    if (getLength() > 0 && _lossList.back().second + 1 == seq) {
        ++_lossList.back().second;
    } else {
        _lossList.push_back(make_pair(seq, seq));
    }
    _length += 1;
}

void LossList::append(SequenceNumber start, SequenceNumber end) {
    Q_ASSERT_X(_lossList.empty() || (_lossList.back().second < start),
               "LossList::append(SequenceNumber, SequenceNumber)",
               "SequenceNumber range appended is not greater than the last SequenceNumber in the list");
    Q_ASSERT_X(start <= end,
               "LossList::append(SequenceNumber, SequenceNumber)", "Range start greater than range end");

    if (getLength() > 0 && _lossList.back().second + 1 == start) {
        _lossList.back().second = end;
    } else {
        _lossList.push_back(make_pair(start, end));
    }
    _length += seqlen(start, end);
}

void LossList::insert(SequenceNumber start, SequenceNumber end) {
    Q_ASSERT_X(start <= end,
               "LossList::insert(SequenceNumber, SequenceNumber)", "Range start greater than range end");
    
    auto it = find_if_not(_lossList.begin(), _lossList.end(), [&start](pair<SequenceNumber, SequenceNumber> pair){
        return pair.second < start;
    });
    
    if (it == _lossList.end() || end < it->first) {
        // No overlap, simply insert
        _length += seqlen(start, end);
        _lossList.insert(it, make_pair(start, end));
    } else {
        // If it starts before segment, extend segment
        if (start < it->first) {
            _length += seqlen(start, it->first - 1);
            it->first = start;
        }
        
        // If it ends after segment, extend segment
        if (end > it->second) {
            _length += seqlen(it->second + 1, end);
            it->second = end;
        }
        
        auto it2 = it;
        ++it2;
        // For all ranges touching the current range
        while (it2 != _lossList.end() && it->second >= it2->first - 1) {
            // extend current range if necessary
            if (it->second < it2->second) {
                _length += seqlen(it->second + 1, it2->second);
                it->second = it2->second;
            }
            
            // Remove overlapping range
            _length -= seqlen(it2->first, it2->second);
            it2 = _lossList.erase(it2);
        }
    }
}

bool LossList::remove(SequenceNumber seq) {
    auto it = find_if(_lossList.begin(), _lossList.end(), [&seq](pair<SequenceNumber, SequenceNumber> pair) {
        return pair.first <= seq && seq <= pair.second;
    });
    
    if (it != end(_lossList)) {
        if (it->first == it->second) {
            _lossList.erase(it);
        } else if (seq == it->first) {
            ++it->first;
        } else if (seq == it->second) {
            --it->second;
        } else {
            auto temp = it->second;
            it->second = seq - 1;
            _lossList.insert(++it, make_pair(seq + 1, temp));
        }
        _length -= 1;
        
        // this sequence number was found in the loss list, return true
        return true;
    } else {
        // this sequence number was not found in the loss list, return false
        return false;
    }
}

void LossList::remove(SequenceNumber start, SequenceNumber end) {
    Q_ASSERT_X(start <= end,
               "LossList::remove(SequenceNumber, SequenceNumber)", "Range start greater than range end");
    // Find the first segment sharing sequence numbers
    auto it = find_if(_lossList.begin(), _lossList.end(), [&start, &end](pair<SequenceNumber, SequenceNumber> pair) {
        return (pair.first <= start && start <= pair.second) || (start <= pair.first && pair.first <= end);
    });
    
    // If we found one
    if (it != _lossList.end()) {
        
        // While the end of the current segment is contained, either shorten it (first one only - sometimes)
        // or remove it altogether since it is fully contained it the range
        while (it != _lossList.end() && end >= it->second) {
            if (start <= it->first) {
                // Segment is contained, update new length and erase it.
                _length -= seqlen(it->first, it->second);
                it = _lossList.erase(it);
            } else {
                // Beginning of segment not contained, modify end of segment.
                // Will only occur sometimes one the first loop
                _length -= seqlen(start, it->second);
                it->second = start - 1;
                ++it;
            }
        }
        
        // There might be more to remove
        if (it != _lossList.end() && it->first <= end) {
            if (start <= it->first) {
                // Truncate beginning of segment
                _length -= seqlen(it->first, end);
                it->first = end + 1;
            } else {
                // Cut it in half if the range we are removing is contained within one segment
                _length -= seqlen(start, end);
                auto temp = it->second;
                it->second = start - 1;
                _lossList.insert(++it, make_pair(end + 1, temp));
            }
        }
    }
}

SequenceNumber LossList::getFirstSequenceNumber() const {
    Q_ASSERT_X(getLength() > 0, "LossList::getFirstSequenceNumber()", "Trying to get first element of an empty list");
    return _lossList.front().first;
}

SequenceNumber LossList::popFirstSequenceNumber() {
    auto front = getFirstSequenceNumber();
    remove(front);
    return front;
}

void LossList::write(ControlPacket& packet, int maxPairs) {
    int writtenPairs = 0;
    
    for (const auto& pair : _lossList) {
        packet.writePrimitive(pair.first);
        packet.writePrimitive(pair.second);
        
        ++writtenPairs;
        
        // check if we've written the maximum number we were told to write
        if (maxPairs != -1 && writtenPairs >= maxPairs) {
            break;
        }
    }
}

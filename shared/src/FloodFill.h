//
//  FloodFill.h
//  hifi
//
//  Created by Tobias Schwinger 3/26/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__FloodFill__
#define __hifi__FloodFill__

/**
 * Line scanning, iterative flood fill algorithm.
 *
 * The strategy must obey the following contract:
 *
 *   There is an associated cursor that represents a position on the image. 
 *   The member functions 'left(C&)', 'right(C&)', 'up(C&)', and 'down(C&)' 
 *   move it.
 *   The state of a cursor can be deferred to temporary storage (typically a
 *   stack or a queue) using the 'defer(C const&)' member function. 
 *   Calling 'deferred(C&)' restores a cursor's state from temporary storage
 *   and removes it there.
 *   The 'select(C const&)' and 'process(C const&)' functions control the 
 *   algorithm. The former is called to determine where to go. It may be
 *   called multiple times but does not have to (and should not) return 
 *   'true' more than once for a pixel to be selected (will cause memory
 *   overuse, otherwise). The latter will never be called for a given pixel
 *   unless previously selected. It may be called multiple times, in which
 *   case it should return 'true' upon successful processing and 'false'
 *   when an already processed pixel has been visited. 
 *
 * Note: The terms "image" and "pixel" are used for illustratory purposes
 * and mean "undirected graph with 4-connected 2D grid topology" and "node",
 * respectively.
 *
 */
template< class Strategy, typename Cursor >
void floodFill(Cursor const& position,
        Strategy const& strategy = Strategy());


template< class Strategy, typename Cursor > 
struct floodFill_impl : Strategy {

    floodFill_impl(Strategy const& s) : Strategy(s) { }

    using Strategy::select;
    using Strategy::process;

    using Strategy::left;
    using Strategy::right;
    using Strategy::up;
    using Strategy::down;

    using Strategy::defer;
    using Strategy::deferred;

    void go(Cursor position) {

        if (! select(position)) {
            return;
        }

        Cursor higher, lower, h,l, i;
        bool higherFound, lowerFound, hf, lf;
        do {

            if (! process(position)) {
                continue;
            }

            higher = position; higherFound = false;
            up(higher); yTest(higher, higherFound);
            lower = position; lowerFound = false;
            down(lower); yTest(lower, lowerFound);

            i = position, h = higher, l = lower;
            hf = higherFound, lf = lowerFound;
            do {
                right(i), right(h), right(l); yTest(h,hf); yTest(l,lf); 

            } while (selectAndProcess(i));

            i = position, h = higher, l = lower;
            hf = higherFound, lf = lowerFound;
            do {
                left(i); left(h); left(l); yTest(h,hf); yTest(l,lf);

            } while (selectAndProcess(i));

        } while (deferred(position));
    }

    bool selectAndProcess(Cursor const& i) {

        if (select(i)) {

            process(i);
            return true;
        }
        return false;
    }

    void yTest(Cursor const& i, bool& state) {

        if (! select(i)) {

            state = false;

        } else if (! state) {

            state = true;
            defer(i);
        }
    }
};

template< class Strategy, typename Cursor >
void floodFill(Cursor const& p, Strategy const& s) {

    floodFill_impl<Strategy,Cursor>(s).go(p);
}


#endif /* defined(__hifi__FloodFill__) */


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

        Cursor higher, lower, h,l, i;
        bool higherFound, lowerFound, hf, lf;
        do {

            if (! select(position)) {
                continue;
            }

            process(position);

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


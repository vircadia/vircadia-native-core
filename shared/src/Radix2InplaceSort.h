//
// Radix2InplaceSort.h
// hifi
//
// Created by Tobias Schwinger on 3/22/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__Radix2InplaceSort__
#define __hifi__Radix2InplaceSort__

#include <algorithm>


/**
 * Sorts the range between two iterators in linear time.
 *
 * A Radix2Scanner must be provided to decompose the sorting
 * criterion into a fixed number of bits.
 */
template< class Radix2Scanner, typename BidiIterator >
void radix2InplaceSort( BidiIterator from, BidiIterator to,
        Radix2Scanner const& scanner = Radix2Scanner() );



template< class Scanner, typename Iterator > 
struct radix2InplaceSort_impl : Scanner {

    radix2InplaceSort_impl(Scanner const& s) : Scanner(s) { }

    using Scanner::advance;
    using Scanner::bit;

    void go(Iterator& from, Iterator& to, typename Scanner::state_type s) {

        Iterator l(from), r(to);
        unsigned cl, cr;

        using std::swap;

        while (true) {
            // scan from left for set bit
            for (cl = cr = 0u; l != r ; ++l, ++cl)
                if (bit(*l, s)) {
                    // scan from the right for unset bit
                    for (++cr; --r != l ;++cr)
                        if (! bit(*r, s)) {
                            // swap, continue scanning from left
                            swap(*l, *r);
                            break;
                        }
                    if (l == r)
                        break;
                }

            // on to the next digit, if any
            if (! advance(s)) {
                return; 
            }

            // recurse into smaller branch and prepare iterative
            // processing of the other
            if (cl < cr) {

                if (cl > 1u) go(from, l, s);
                else if (cr <= 1u)
                    return;

                l = from = r;
                r = to;

            } else {

                if (cr > 1u) go(r, to, s);
                else if (cl <= 1u)
                    return;

                r = to = l;
                l = from;
            }
        }
    }
};

template< class Radix2Scanner, typename BidiIterator >
void radix2InplaceSort( BidiIterator from, BidiIterator to,
        Radix2Scanner const& scanner) {

    radix2InplaceSort_impl<Radix2Scanner, BidiIterator>(scanner)
        .go(from, to, scanner.initial_state());
}

#endif /* defined(__hifi__Radix2InplaceSort__) */

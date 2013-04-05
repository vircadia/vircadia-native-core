//
// Radix2IntegerScanner.h
// hifi
//
// Created by Tobias Schwinger on 3/23/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__Radix2IntegerScanner__
#define __hifi__Radix2IntegerScanner__

#include <stddef.h>
#include <stdint.h>

namespace type_traits { // those are needed for the declaration, see below

    // Note: There are better / more generally appicable implementations 
    // in C++11, make_signed is missing in TR1 too - so I just use C++98
    // hacks that get the job done...

    template< typename T > struct is_signed 
    { static bool const value = T(-1) < T(0); };

    template< typename T, size_t S = sizeof(T) > struct make_unsigned;
    template< typename T > struct make_unsigned< T, 1 > { typedef uint8_t type; };
    template< typename T > struct make_unsigned< T, 2 > { typedef uint16_t type; };
    template< typename T > struct make_unsigned< T, 4 > { typedef uint32_t type; };
    template< typename T > struct make_unsigned< T, 8 > { typedef uint64_t type; };
}


/**
 * Bit decomposition facility for integers.
 */
template< typename T, 
        bool _Signed = type_traits::is_signed<T>::value  >
class Radix2IntegerScanner;



template< typename UInt >
class Radix2IntegerScanner< UInt, false > {

        UInt valMsb;
    public:

        Radix2IntegerScanner()
            : valMsb(~UInt(0) &~ (~UInt(0) >> 1)) { }

        explicit Radix2IntegerScanner(int bits)
            : valMsb(UInt(1u) << (bits - 1)) {
        }

        typedef UInt state_type;

        state_type initial_state()  const   { return valMsb; }
        bool advance(state_type& s) const   { return (s >>= 1) != 0u; }

        bool bit(UInt const& v, state_type const& s) const { return !!(v & s); }
};

template< typename Int >
class Radix2IntegerScanner< Int, true >
{
        typename type_traits::make_unsigned<Int>::type valMsb;
    public:

        Radix2IntegerScanner()
            : valMsb(~state_type(0u) &~ (~state_type(0u) >> 1)) {
        }

        explicit Radix2IntegerScanner(int bits)
            : valMsb(state_type(1u) << (bits - 1)) {
        }


        typedef typename type_traits::make_unsigned<Int>::type state_type;
        
        state_type initial_state()  const   { return valMsb; }
        bool advance(state_type& s) const   { return (s >>= 1) != 0u; }

        bool bit(Int const& v, state_type const& s) const { return !!((v-valMsb) & s); }
};

#endif /* defined(__hifi__Radix2IntegerScanner__) */


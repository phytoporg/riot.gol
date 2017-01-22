#pragma once

//
// Hashing utility functions.
//

#include <functional>

namespace Utility
{
    // 
    // Directly borrowed from boost's hash.hpp to avoid having to bloat
    // this git repository by including a git submodule.
    //
    // Full implementation/file can be found at:
    // http://www.boost.org/doc/libs/1_34_1/boost/functional/hash/hash.hpp
    //

    template <class T>
    inline void hash_combine(std::size_t& seed, T const& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template <class A, class B>
    std::size_t hash_value(std::pair<A, B> const& v)
    {
        std::size_t seed = 0;
        hash_combine(seed, v.first);
        hash_combine(seed, v.second);
        return seed;
    }
}

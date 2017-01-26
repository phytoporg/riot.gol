#pragma once

#include "AlignedBuffer.h"

namespace Utility
{
    template <unsigned int N>
    class AlignedMemoryPool
    {
    public:
        AlignedMemoryPool(/* max amount of storage?*/)
        {
            // Allocate starting buffers
        }

        AlignedBuffer<N> Allocate()
        {
            // TODO
        }

        void Free(const AlignedBuffer<N>& buffer)
        {
            // TODO
        }

    private:
        // No precise plans for this just yet
    };
}

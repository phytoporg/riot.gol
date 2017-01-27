#pragma once

//
// Aligned buffer wrapper. Is this even necessary??
//

namespace Utility
{
    template <size_t N> class AlignedMemoryPool;

    template <size_t N>
    class AlignedBuffer
    {
        static_assert(N && !(N & (N - 1)), "N must be a power of two");

    public:
        AlignedBuffer() : m_pMemoryPool(nullptr), m_pBuffer(nullptr) {}
        AlignedBuffer(uint8_t* pBuffer, AlignedMemoryPool<N>& pool)
            : m_pBuffer(pBuffer), m_pMemoryPool(&pool)
        {
            if (reinterpret_cast<size_t>(pBuffer) & (N - 1))
            {
                throw "pBuffer is not properly aligned";
            }
        }

        uint8_t* Get()             { return m_pBuffer; }
        uint8_t const* Get() const { return m_pBuffer; }

    private:
        AlignedMemoryPool<64>* m_pMemoryPool;
        uint8_t* m_pBuffer;
    };
}
#pragma once

//
// RAII aligned buffer wrapper.
//

namespace Utility
{
    template <unsigned int N> class AlignedMemoryPool;

    template <unsigned int N>
    class AlignedBuffer
    {
        static_assert(N && !(N & (N - 1)), "N must be a power of two");

    public:
        AlignedBuffer(uint8_t* pBuffer, const AlignedMemoryPool<N>& pool)
            : m_pBuffer(pBuffer), m_pMemoryPool(&pool)
        {
            if (pBuffer & (N - 1))
            {
                throw "pBuffer is not properly aligned";
            }
        }

        ~AlignedBuffer()
        {
            m_pMemoryPool->Free(*this);
        }

        uint8_t* Get() { return m_pBuffer; }

    private:
        AlignedMemoryPool* m_pMemoryPool;
        uint8_t* m_pBuffer;
    };
}
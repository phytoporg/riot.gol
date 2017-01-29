#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

namespace Utility
{
    template <size_t N>
    class AlignedMemoryPool
    {
    private:
        //
        // Contains a chunk of sub-blocks, allocated on the N-boundary.
        //
        struct SuperBlock
        {
            //
            // Pointer to the actual base of this block.
            //
            uint8_t* pBase;

            //
            // Pointer to the aligned base.
            //
            uint8_t* pAligned;

            //
            // For auto-cleanup on aligned buffer removal.
            //
            size_t Refcount;
        };

        std::vector<SuperBlock> m_superblocks;

        typedef typename std::vector<SuperBlock>::iterator SuperblockIterator;

    public:
        AlignedMemoryPool() = default;

        AlignedMemoryPool(size_t sublockSize, size_t superblockLength)
            : m_sublockSize(sublockSize),
              m_superblockLength(superblockLength),
              m_length(m_sublockSize * m_superblockLength)
        {
            if (sublockSize % N)
            {
                throw "Sublock size must be a multiple of alignment.";
            }
        }

        ~AlignedMemoryPool()
        {
            for (auto& superblock : m_superblocks)
            {
                assert(!superblock.Refcount);
                delete[] superblock.pBase;
            }
        }

        uint8_t* Allocate()
        {
            SuperblockIterator superIt;
            uint8_t* pAligned = nullptr;
            if (m_freeBuffers.empty())
            {
                superIt = AllocateSuperblock();
                pAligned = m_freeBuffers.back();
            }
            else
            {
                pAligned = m_freeBuffers.back();
                superIt = FindParentSuperblock(pAligned);
            }

            m_freeBuffers.pop_back();
            m_usedBuffers.push_back(pAligned);
            superIt->Refcount++;

            memset(pAligned, 0, m_sublockSize);

            return pAligned;
        }

        void Free(uint8_t* pBuffer)
        {
            auto usedIt = std::find(m_usedBuffers.begin(), m_usedBuffers.end(), pBuffer);
            if (usedIt == m_usedBuffers.end())
            {
                throw "Buffer to free could not be found in used list.";
            }

            m_usedBuffers.erase(usedIt);
            m_freeBuffers.push_back(pBuffer);

            // 
            // TODO: Update superblock refcount
            //
            auto superIt = FindParentSuperblock(pBuffer);
            assert(superIt != m_superblocks.end());
            assert(superIt->Refcount > 0);

            superIt->Refcount--;
            if (!superIt->Refcount && m_superblocks.size() > 1)
            {
                m_superblocks.erase(superIt);
            }
        }

    private:
        SuperblockIterator FindParentSuperblock(uint8_t* pAligned)
        {
            return
                std::find_if(
                    m_superblocks.begin(),
                    m_superblocks.end(),
                    [pAligned, this](const SuperBlock& superblock)
                    {
                        return pAligned >= superblock.pAligned &&
                               pAligned <  superblock.pAligned + m_length;
                    });
        }

        SuperblockIterator AllocateSuperblock()
        {
            //
            // What to do in OOM?
            //
            uint8_t* pSuperblock = new uint8_t[m_length + (N - 1)];
            memset(pSuperblock, 0, m_length + (N - 1));

            size_t base = reinterpret_cast<size_t>(pSuperblock);
            if (base & (N - 1))
            {
                base &= ~(N - 1);
                base += N;
            }

            uint8_t* pAligned = reinterpret_cast<uint8_t*>(base);
            assert(!(base & (N - 1)));
            assert(pAligned >= pSuperblock);
            assert(pSuperblock + N >= pAligned);

            uint8_t* pSublock = pAligned;
            for (size_t i = 0; i < m_superblockLength; ++i)
            {
                m_freeBuffers.push_back(pSublock);
                pSublock += m_sublockSize;
            }

            m_superblocks.push_back({ pSuperblock, pAligned, 0 });

            //
            // I know this seems redundant, but all it takes is a clumsy refactoring...
            //
            assert(!m_superblocks.empty());
            return m_superblocks.end() - 1;
        }

        std::vector<uint8_t*> m_freeBuffers;
        std::vector<uint8_t*> m_usedBuffers;

        size_t m_sublockSize;
        size_t m_superblockLength;;
        size_t m_length;
    };
}

#include "allocator.h"

#include <cstdint>
#include <memory>

void* LinearAllocator::allocate(const std::size_t allocationSize, std::size_t alignment)
{
    if (alignment == 0) alignment = alignof(std::max_align_t);
    std::size_t remainingSize = region_ - usedAmount_;

    void* alignedPointer = std::align(alignment, allocationSize, offsetPointer_, remainingSize);
    if (alignedPointer != nullptr)
    {
        const unsigned long alignmentSize = reinterpret_cast<std::uintptr_t>(alignedPointer) - reinterpret_cast<std::uintptr_t>(offsetPointer_);
        usedAmount_ += static_cast<int>(alignmentSize + allocationSize);
        offsetPointer_ = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(alignedPointer) + allocationSize);
    }

    return alignedPointer;
}

void LinearAllocator::free(void* pointer) {}

void LinearAllocator::reset()
{
    usedAmount_ = 0;
    offsetPointer_ = basePointer_;
}
#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include <cstddef>

class Allocator
{
public:
    explicit Allocator(const std::size_t buffer): region_(buffer) {}
    virtual ~Allocator() = default;

    virtual void* allocate(std::size_t size, std::size_t alignment) = 0;
    virtual void free(void *pointer) = 0;

protected:
    std::size_t region_;
};

class LinearAllocator final : public Allocator
{
public:
    explicit LinearAllocator(const std::size_t buffer, void* basePointer):
        Allocator(buffer),
        basePointer_(basePointer),
        offsetPointer_(basePointer),
        usedAmount_(0) {}
    void* allocate(std::size_t allocationSize, std::size_t alignment) override;
    void free(void *pointer) override;
    void reset();
private:
    void* basePointer_;
    void *offsetPointer_;
    int usedAmount_;
};

#endif // ALLOCATOR_H

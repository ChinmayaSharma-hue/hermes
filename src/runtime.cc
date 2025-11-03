#include "runtime.h"
#include <cstdio>
#include <cstring>
#include <linux/userfaultfd.h>
#include <memory>
#include <poll.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>

namespace dsm {
    void* Runtime::reserveMemory(const size_t region) const {
        // TODO: Should the amount of space to reserve be taken as a parameter?
        // TODO: Use MAP_FIXED to allow passing raw pointers across the nodes
        logger_->info("Reserving {:#d} amount of memory", region_);
        void* base = mmap(nullptr, region, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (base == MAP_FAILED) {
            // TODO: Error handling standardization across the library
            logger_->error("mmap failed: ", strerror(errno));
            exit(-1);
        }

        return base;
    }

    Runtime::Runtime():
        logger_(spdlog::stdout_color_mt("hermes_runtime")),
        region_(1UL << 26),
        base_(reserveMemory(region_)),
        pageTable_(Map<constructs::pte>(base_, sysconf(_SC_PAGESIZE))),
        ownerInformation_(
              Map<constructs::owner_information>(base_, sysconf(_SC_PAGESIZE))),
        grpcServer_(RPCServer("localhost", 9876)),
        allocator_(std::make_unique<LinearAllocator>(region_, base_))
    {
        startPageFaultHandler();
        startGrpcServer();
        // barrier();
    }

    Runtime::~Runtime() {
        stopGrpcServer();
        stopPageFaultHandler();
    }

    void Runtime::startGrpcServer() {
        logger_->info("Starting grpc server");
        grpcServer_.start();
    }

    void Runtime::stopGrpcServer() {
        logger_->info("Stopping grpc server");
        grpcServer_.stop();
    }

    void Runtime::startPageFaultHandler()
    {
        logger_->info("Starting page fault_handler");
        // Getting the file descriptor for reading the faults
        pageFaultHandlerFd_ = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
        if (pageFaultHandlerFd_ == -1) {
            logger_->error("Failed to register page fault_handler: {:#s}", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Using UFFD API to perform specific operations on the file descriptor
        uffdio_api api = {
            .api = UFFD_API,
            .features = 0
        };
        if (ioctl(static_cast<int>(pageFaultHandlerFd_), UFFDIO_API, &api) == -1) {
            logger_->error("Failed to register page fault_handler: {:#s}", strerror(errno));
            exit(EXIT_FAILURE);
        }


        // Registering the mmapped region with userfaultfd [?]
        uffdio_register reg = {
            .range = {
                .start = reinterpret_cast<unsigned long>(base_),
                .len = region_
            },
            .mode = UFFDIO_REGISTER_MODE_MISSING
        };
        if (ioctl(static_cast<int>(pageFaultHandlerFd_), UFFDIO_REGISTER, &reg) == -1) {
            logger_->error("Failed to register page fault_handler: {:#s}", strerror(errno));
            exit(EXIT_FAILURE);
        }

        pageFaultHandlerThread_ = std::thread(&Runtime::pageFaultHandlerLoop, this);
    }

    void Runtime::pageFaultHandlerLoop() {
        uffd_msg msg{};
        while (true)
        {
            {
                std::unique_lock lock(pageFaultHandlerMutex_);
                if (stopPageFaultHandler_) break;
            }

            if (const size_t error = read(pageFaultHandlerFd_, &msg, sizeof(msg));
                error == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                if (errno == EINTR) {
                    continue;
                }

                perror("read");
                break;
            }

            if (msg.event == UFFD_EVENT_PAGEFAULT)
            {
                unsigned long addr = msg.arg.pagefault.address;
                const int pageSize = sysconf(_SC_PAGESIZE);

                logger_->info("Page fault address: {:#x}", addr);

                struct uffdio_zeropage zeroPage;
                zeroPage.range.start = addr;
                zeroPage.range.len = pageSize;
                zeroPage.mode = 0;

                if (ioctl(pageFaultHandlerFd_, UFFDIO_ZEROPAGE, &zeroPage) == -1) {
                    logger_->error("Failed to read page fault address: {:#x}", strerror(errno));
                } else {
                    logger_->info("Zeroed page at address {:#x}", addr);
                }
            }
        }
    }


    void Runtime::stopPageFaultHandler()
    {
        logger_->info("Stopping page fault_handler");
        {
            std::unique_lock lock(pageFaultHandlerMutex_);
            stopPageFaultHandler_ = true;
        }

        if (pageFaultHandlerFd_ != -1) {
            close(pageFaultHandlerFd_);
            pageFaultHandlerFd_ = -1;
        }

        if (pageFaultHandlerThread_.joinable()) {
            pageFaultHandlerThread_.join();
        }
    }

}

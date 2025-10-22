#ifndef DSM_H
#define DSM_H
#include <map>
#include <mutex>
#include <thread>

#include "server.h"
#include "allocator.h"
#include "spdlog/spdlog.h"

namespace dsm {
    namespace constructs {
        enum accessSpecifier {
            READ = 0,
            WRITE = 1,
        };

        struct pte {
            accessSpecifier access;
            // TODO: Find out how to store the copyset
            u_int8_t copyset;
            std::mutex mutex;

            pte() : access(READ), copyset(0) {}
        };

        struct owner_information {
            u_int8_t owner;

            // TODO: By default, the owner should be the current node
            owner_information() : owner(0) {}
        };
    }


    // Class functionality
    // 1. Function as a wrapper class of std::map
    // 2. Overloading all the map functions, for providing a seamless conversion of void pointer to page ID
    template <typename T>
    class Map {
        public:
            Map(void* base, const size_t page_size): base_(reinterpret_cast<uintptr_t>(base)), page_size_(page_size) {};

            T& operator[](void* addr) {
                const int page_id = addrToPageId(addr);
                return map_[page_id];
            }

            const T& at(void* addr) const {
                const int page_id = addrToPageId(addr);
                return map_.at(page_id);
            }

            void erase(void* addr) {
                const int page_id = addrToPageId(addr);
                map_.erase(page_id);
            }

            bool contains(void* addr) const {
                const int page_id = addrToPageId(addr);
                return map_.find(page_id) != map_.end();
            }

        private:
            int addrToPageId(void* addr) const {
                auto uaddr = reinterpret_cast<uintptr_t>(addr);
                return static_cast<int>((uaddr - base_) / page_size_);
            }

            uintptr_t base_;
            size_t page_size_;
            std::map<int, T> map_;
    };


    // Class functionality
    // 1. Maintain the data structures needed to point to the pages appropriately based on faults and requests
    // 2. Manage the threads where the gRPC servers run to serve read/write requests
    // 3. Responsible for registering the page fault handlers
    // 4. Responsible for synchronization of set up across the processors in the initial stage
    class Runtime {
        public:
            Runtime();
            ~Runtime();

            void* acquireShared(size_t allocationSize, size_t alignment) const;

        private:
            std::shared_ptr<spdlog::logger> logger_;
            size_t region_;
            void * base_;
            long int pageFaultHandlerFd_;
            Map<constructs::pte> pageTable_;
            Map<constructs::owner_information> ownerInformation_;
            RPCServer grpcServer_;
            std::unique_ptr<Allocator> allocator_;
            std::thread rpcThread_;
            std::thread pageFaultHandlerThread_;
            bool stopPageFaultHandler_{false};
            std::mutex pageFaultHandlerMutex_;

            [[nodiscard]] void* reserveMemory(size_t region) const;
            void startGrpcServer();
            void stopGrpcServer();
            void startPageFaultHandler();
            void stopPageFaultHandler();
            void pageFaultHandlerLoop();
            // void barrier();
    };
}

#endif // DSM_H

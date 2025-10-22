#ifndef GRPC_H
#define GRPC_H
#include <thread>
#include <grpcpp/grpcpp.h>

#include "genproto/protos/page.pb.h"          // messages
#include "genproto/protos/page.grpc.pb.h"     // service (PageService::Service)

namespace dsm
{
    class PageServiceImpl final : public page_service::v1::PageService::Service
    {
        grpc::Status ReadPage(grpc::ServerContext* context, const page_service::v1::PageRequest* request, page_service::v1::ReadPageResponse* response) override;
        grpc::Status WritePage(grpc::ServerContext* context, const page_service::v1::PageRequest* request, page_service::v1::WritePageResponse* response) override;
    };

    class RPCServer
    {
    public:
        explicit RPCServer(std::string host, int port);

        void start();

        void stop();

    private:
        void serverThread() const;

        PageServiceImpl service_;
        grpc::ServerBuilder builder_;
        std::unique_ptr<grpc::Server> server_;
        std::thread server_thread_;
        std::string host_;
        int port_;
    };
}

#endif // GRPC_H

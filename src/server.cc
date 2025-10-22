#include "server.h"

namespace dsm
{
    grpc::Status PageServiceImpl::ReadPage(grpc::ServerContext* context, const page_service::v1::PageRequest* request, page_service::v1::ReadPageResponse* response)
    {
        return grpc::Status::OK;
    }

    grpc::Status PageServiceImpl::WritePage(grpc::ServerContext* context, const page_service::v1::PageRequest* request, page_service::v1::WritePageResponse* response)
    {
        return grpc::Status::OK;
    }

    RPCServer::RPCServer(std::string host, const int port):
        host_(std::move(host)),
        port_(port)
    {
        const std::string address = host_ + ":" + std::to_string(port_);
        builder_.AddListeningPort(address, grpc::InsecureServerCredentials());

        builder_.RegisterService(&service_);
    }

    // Must be run on a thread different from the main thread
    void RPCServer::start()
    {
        server_ = builder_.BuildAndStart();
        server_thread_ = std::thread(&RPCServer::serverThread, this);
    }

    // Must be run on the main thread
    // Can store the thread ID to perform a check to make sure the thread that the start was called on is different from the one shutdown was called on
    void RPCServer::stop()
    {
        server_->Shutdown();
        if (server_thread_.joinable()) server_thread_.join();
    }

    void RPCServer::serverThread() const
    {
        server_->Wait();
    }

}


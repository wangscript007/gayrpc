#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include <vector>
#include <gayrpc/core/GayRpcInterceptor.h>

TEST_CASE("interceptor are computed", "[interceptor]")
{
    using namespace gayrpc::core;
    // normal
    {
        auto interceptor = gayrpc::core::makeInterceptor();
        gayrpc::core::RpcMeta meta;
        google::protobuf::Message* message = nullptr;

        int v = 0;

        interceptor(
            std::move(meta),
            *message, 
            [&v](gayrpc::core::RpcMeta&&, const google::protobuf::Message&, InterceptorContextType context) {
                v++;
            },
            InterceptorContextType());

        REQUIRE(v == 1);
    }

    // normal
    {
        std::vector<int> vlist;

        auto interceptor = gayrpc::core::makeInterceptor(
            [&vlist](gayrpc::core::RpcMeta&& meta, const google::protobuf::Message& message, const gayrpc::core::UnaryHandler& next,
                InterceptorContextType context){
                vlist.push_back(1);
                next(std::move(meta), message, std::move(context));
            },
            [&vlist](gayrpc::core::RpcMeta&& meta, const google::protobuf::Message& message, const gayrpc::core::UnaryHandler& next,
                InterceptorContextType context){
                vlist.push_back(2);
                next(std::move(meta), message, std::move(context));
            });

        gayrpc::core::RpcMeta meta;
        google::protobuf::Message* message = nullptr;

        interceptor(
            std::move(meta),
            *message, 
            [&vlist](const gayrpc::core::RpcMeta&, const google::protobuf::Message&, InterceptorContextType context) {
                vlist.push_back(3);
            }, std::move(InterceptorContextType()));

        std::vector<int> tmp = {1, 2, 3};
        REQUIRE(vlist == tmp);
    }

    // yield interceptor
    {
        std::vector<int> vlist;

        auto interceptor = gayrpc::core::makeInterceptor(
            [&vlist](const gayrpc::core::RpcMeta& meta, const google::protobuf::Message& message, const gayrpc::core::UnaryHandler& next,
                InterceptorContextType context) {
                vlist.push_back(1);
            },
            [&vlist](gayrpc::core::RpcMeta&& meta, const google::protobuf::Message& message, const gayrpc::core::UnaryHandler& next,
                InterceptorContextType&& context) {
                vlist.push_back(2);
                next(std::move(meta), message, std::move(context));
            });

        gayrpc::core::RpcMeta meta;
        google::protobuf::Message* message = nullptr;

        interceptor(
            std::move(meta),
            *message,
            [&vlist](gayrpc::core::RpcMeta&&, const google::protobuf::Message&, InterceptorContextType context) {
                vlist.push_back(3);
            }, std::move(InterceptorContextType()));

        std::vector<int> tmp = {1};
        REQUIRE(vlist == tmp);
    }

    // resume interceptor
    {
        std::vector<int> vlist;

        gayrpc::core::UnaryHandler tmpHandler;
        gayrpc::core::RpcMeta meta;
        google::protobuf::Message* message = nullptr;

        {
            auto interceptor = gayrpc::core::makeInterceptor(
                [&](gayrpc::core::RpcMeta&& meta, const google::protobuf::Message& message, const gayrpc::core::UnaryHandler& next,
                    InterceptorContextType&& context) {
                    vlist.push_back(1);
                    tmpHandler = next;
                },
                [&](gayrpc::core::RpcMeta&& meta, const google::protobuf::Message& message, const gayrpc::core::UnaryHandler& next,
                    InterceptorContextType&& context) {
                    vlist.push_back(2);
                    next(std::move(meta), message, std::move(context));
                });

            auto metaCopy = meta;
            interceptor(
                std::move(metaCopy),
                *message,
                [&vlist](gayrpc::core::RpcMeta&&, const google::protobuf::Message&, InterceptorContextType&& context) {
                    vlist.push_back(3);
                }, std::move(InterceptorContextType()));
        }

        REQUIRE(vlist == std::vector<int>{1});
        tmpHandler(std::move(meta), *message, InterceptorContextType());
        REQUIRE(vlist == std::vector<int>{1,2,3});
    }
}
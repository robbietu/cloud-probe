/**
* Netis Agent REST APIs
* Netis Agent 管理平台由 **Agent**, **Daemon**, **Manager** 三个组件组成。   * Agent: 部署在用户环境中采集数据，当前支持 Packet Agent 采集网络数据   * Daemon: 部署在用户环境中通过 REST APIs 管理 Agent   * Manager: 部署在监控环境中通过 REST APIs 管理 Daemon 和 Agent 
*
* OpenAPI spec version: 0.1.0
* 
*
* NOTE: This class is auto generated by the swagger code generator program.
* https://github.com/swagger-api/swagger-codegen.git
* Do not edit the class manually.
*/

#include "DaemonAgentApi.h"

namespace io {
    namespace swagger {
        namespace server {
            namespace api {

                using namespace io::swagger::server::model;

                DaemonAgentApi::DaemonAgentApi(Pistache::Address addr)
                        : httpEndpoint(std::make_shared<Pistache::Http::Endpoint>(addr)) {};

                void DaemonAgentApi::init(size_t thr) {
                    auto opts = Pistache::Http::Endpoint::options().threads(thr);
                    httpEndpoint->init(opts);
                    setupRoutes();
                }

                void DaemonAgentApi::start() {
                    httpEndpoint->setHandler(router.handler());
                    std::thread{[this](){
                        httpEndpoint->serve();
                    }}.detach();
                }

                void DaemonAgentApi::shutdown() {
                    httpEndpoint->shutdown();
                }

                void DaemonAgentApi::setupRoutes() {
                    using namespace Pistache::Rest;

                    Routes::Delete(router, base_ + "/daemon/agents/:agentId",
                                   Routes::bind(&DaemonAgentApi::del_agent_by_id_handler, this));
                    Routes::Get(router, base_ + "/daemon/agents",
                                Routes::bind(&DaemonAgentApi::list_agents_handler, this));
                    Routes::Get(router, base_ + "/daemon/agents/:agentId",
                                Routes::bind(&DaemonAgentApi::query_agent_by_id_handler, this));
                    Routes::Post(router, base_ + "/daemon/agents",
                                 Routes::bind(&DaemonAgentApi::start_agent_handler, this));
                    Routes::Post(router, base_ + "/daemon/agents/:agentId",
                                 Routes::bind(&DaemonAgentApi::update_agent_by_id_handler, this));
                    Routes::Get(router, base_ + "/daemon",
                                Routes::bind(&DaemonAgentApi::get_status_handler, this));

                    // Default handler, called when a route is not found
                    router.addCustomHandler(Routes::bind(&DaemonAgentApi::daemon_agent_api_default_handler, this));
                }

                void DaemonAgentApi::del_agent_by_id_handler(const Pistache::Rest::Request& request,
                                                             Pistache::Http::ResponseWriter response) {
                    // Getting the path params
                    auto agentId = request.param(":agentId").as<int32_t>();

                    try {
                        this->del_agent_by_id(response);
                    } catch (std::runtime_error& e) {
                        //send a 400 error
                        response.send(Pistache::Http::Code::Bad_Request, e.what());
                        return;
                    }

                }

                void DaemonAgentApi::list_agents_handler(const Pistache::Rest::Request& request,
                                                         Pistache::Http::ResponseWriter response) {

                    try {
                        this->list_agents(response);
                    } catch (std::runtime_error& e) {
                        //send a 400 error
                        response.send(Pistache::Http::Code::Bad_Request, e.what());
                        return;
                    }

                }

                void DaemonAgentApi::query_agent_by_id_handler(const Pistache::Rest::Request& request,
                                                               Pistache::Http::ResponseWriter response) {
                    // Getting the path params
                    auto agentId = request.param(":agentId").as<int32_t>();

                    try {
                        this->query_agent_by_id(response);
                    } catch (std::runtime_error& e) {
                        //send a 400 error
                        response.send(Pistache::Http::Code::Bad_Request, e.what());
                        return;
                    }

                }

                void DaemonAgentApi::start_agent_handler(const Pistache::Rest::Request& request,
                                                         Pistache::Http::ResponseWriter response) {

                    // Getting the body param
                    Agent body;

                    try {
                        nlohmann::json request_body = nlohmann::json::parse(request.body());
                        body.fromJson(request_body);
                        this->start_agent(body, response);
                    } catch (std::runtime_error& e) {
                        //send a 400 error
                        response.send(Pistache::Http::Code::Bad_Request, e.what());
                        return;
                    }

                }

                void DaemonAgentApi::update_agent_by_id_handler(const Pistache::Rest::Request& request,
                                                                Pistache::Http::ResponseWriter response) {
                    // Getting the path params
                    auto agentId = request.param(":agentId").as<int32_t>();
                    const auto action = request.query().get("action");

                    // Getting the body param
                    Agent body;

                    try {
                        nlohmann::json request_body = nlohmann::json::parse(request.body());
                        body.fromJson(request_body);
                        this->update_agent_by_id(action.getOrElse(""), body, response);
                    } catch (std::runtime_error& e) {
                        //send a 400 error
                        response.send(Pistache::Http::Code::Bad_Request, e.what());
                        return;
                    }

                }

                void DaemonAgentApi::get_status_handler(const Pistache::Rest::Request& request,
                                                        Pistache::Http::ResponseWriter response) {
                    try {
                        this->get_status(response);
                    } catch (std::runtime_error& e) {
                        //send a 400 error
                        response.send(Pistache::Http::Code::Bad_Request, e.what());
                        return;
                    }

                }

                void DaemonAgentApi::daemon_agent_api_default_handler(const Pistache::Rest::Request& request,
                                                                      Pistache::Http::ResponseWriter response) {
                    response.send(Pistache::Http::Code::Not_Found, "The requested method does not exist");
                }

            }
        }
    }
}
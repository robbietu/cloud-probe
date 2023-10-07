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
/*
 * PacketAgentLogs.h
 *
 */

#ifndef PacketAgentLogs_H_
#define PacketAgentLogs_H_


#include "ModelBase.h"

#include <string>

namespace io {
namespace swagger {
namespace server {
namespace model {

/// <summary>
/// Packet 通道指标
/// </summary>
class  PacketAgentLogs
    : public ModelBase
{
public:
    PacketAgentLogs();
    virtual ~PacketAgentLogs();

    /////////////////////////////////////////////
    /// ModelBase overrides

    void validate() override;

    nlohmann::json toJson() const override;
    void fromJson(nlohmann::json& json) override;

    /////////////////////////////////////////////
    /// PacketChannelMetrics members

    /// <summary>
    /// log 时间
    /// </summary>
    long getLogTime() const;
    void setLogTime(struct timeval value);
    void setLogTimestamp(long value);
    void setLogMicroTimestamp(long value);
    bool logTimeIsSet() const;
    void unsetLogTime();
    
    /// <summary>
    /// log level
    /// </summary>
    std::string getLogLevel() const;
    void setLogLevel(std::string value);
    bool logLevelIsSet() const;
    void unsetLogLevel();

    /// <summary>
    /// log details
    /// </summary>
    std::string getLogDetails() const;
    void setLogDetails(std::string value);
    bool logDetailsIsSet() const;
    void unsetLogDetails();

protected:
    long m_LogTimestamp;
    long m_LogMicroTimestamp;
    bool m_LogTimeIsSet;
    std::string m_LogLevel;
    bool m_LogLevelIsSet;
    std::string m_LogDetails;
    bool m_LogDetailsIsSet;
};

}
}
}
}

#endif /* PacketChannelMetrics_H_ */
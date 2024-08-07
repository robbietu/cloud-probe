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


#include "Label.h"

namespace io {
namespace swagger {
namespace server {
namespace model {

Label::Label()
{
    m_Value = "";
    m_ValueIsSet = false;
}

Label::~Label()
{
}

void Label::validate()
{
    // TODO: implement validation
}

nlohmann::json Label::toJson() const
{
    nlohmann::json val = nlohmann::json::object();

    if(m_ValueIsSet)
    {
        val["value"] = ModelBase::toJson(m_Value);
    }

    return val;
}

void Label::fromJson(nlohmann::json& val)
{

    MB_FSET(value, Value)
}

std::string Label::getValue() const
{
    return m_Value;
}
void Label::setValue(std::string value)
{
    m_Value = value;
    m_ValueIsSet = true;
}
bool Label::valueIsSet() const
{
    return m_ValueIsSet;
}
void Label::unsetValue()
{
    m_ValueIsSet = false;
}

}
}
}
}


#include <iostream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include "pcaphandler.h"
#include "scopeguard.h"
#include "agent_status.h"
#include "vlan.h"

bool replaceWithIfIp(std::string& expression, std::vector<std::string> &ips) {
    std::string name = expression.substr(strlen("nic."));
    expression = "";
    pcap_if_t *alldevs;
    pcap_if_t *d;
    struct pcap_addr *addr;
    char err_buf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&alldevs, err_buf) < 0)
        return false;
    for (d = alldevs; d; d = d->next) {
        if (strcmp(d->name, (char*)name.data()) == 0) {
            for (addr = d->addresses; addr; addr = addr->next) {
                if (!addr->addr) {
                    continue;
                }

                if (addr->addr->sa_family == AF_INET) {
                    char str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(((sockaddr_in *) addr->addr)->sin_addr), str, sizeof(str));
                    expression +=std::string(str);
                    ips.push_back(std::string(str));
                }
                else if (addr->addr->sa_family == AF_INET6) {
                    char str[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, &(((sockaddr_in6 *) addr->addr)->sin6_addr), str, sizeof(str));
                    expression += std::string(str);
                }
                else {
                    continue;
                }
                if (addr->next != nullptr) {
                    expression += " or host ";
                }
            }
            pcap_freealldevs(alldevs);
            return true;
        }
    }

    pcap_freealldevs(alldevs);
    return false;
}

PcapHandler::PcapHandler(std::string dumpDir, int16_t dumpInterval):
    _dumpDir(dumpDir),
    _dumpInterval(dumpInterval) {
    _gre_count = 0;
    _gre_drop_count = 0;
    _pcap_handle = NULL;
    _pcap_dumpter = NULL;

    if (dumpInterval != -1) {
        _dumpDir = dumpDir + "/";
        _timeStamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if(!boost::filesystem::is_directory(_dumpDir))
            boost::filesystem::create_directories(_dumpDir);
    }

    std::memset(_errbuf, 0, sizeof(_errbuf));
}

PcapHandler::~PcapHandler() {
    closePcapDumper();
    closePcap();
}

int PcapHandler::openPcapDumper(pcap_t* pcap_handle) {
    closePcapDumper();
    char date[60] = {0};
    std::string filepath;
    if (_dumpInterval >0) {
        struct tm* ptm = localtime(&_timeStamp);
        sprintf(date, "%d%02d%02d%02d%02d%02d",
                (int)ptm->tm_year + 1900,(int)ptm->tm_mon + 1,(int)ptm->tm_mday,
                (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
        filepath = _dumpDir + "pktminerg_dump_"+std::string(date) + ".pcap";
    }
    else {
        filepath = _dumpDir + "pktminerg_dump.pcap";
    }
    if (boost::filesystem::exists(filepath)) {
        boost::filesystem::remove(filepath);
    }
    _pcap_dumpter = pcap_dump_open(pcap_handle, filepath.c_str());
    if (_pcap_dumpter == NULL) {
        std::cerr << StatisLogContext::getTimeString() << "Call pcap_dump_open failed, error is "
                  << pcap_geterr(pcap_handle) << "." << std::endl;
        return -1;
    }
    return 0;
}

void PcapHandler::closePcapDumper() {
    if (_pcap_dumpter != NULL) {
        pcap_dump_close(_pcap_dumpter);
        _pcap_dumpter = NULL;
    }
}

void PcapHandler::closePcap() {
    if (_pcap_handle != NULL) {
        pcap_close(_pcap_handle);
        _pcap_handle = NULL;
    }
}

void PcapHandler::packetHandler(const struct pcap_pkthdr* header, const uint8_t* pkt_data) {
    ether_header* eth;
    iphdr* ip;
    ip6_hdr* ipv6;
    uint16_t eth_type;
    int direct;
    vlan_tag *vlan_hdr;

    if(header->caplen < sizeof(ether_header))
        return;
    eth = (ether_header*)pkt_data;
    eth_type = ntohs(eth->ether_type);
    direct = PKTD_UNKNOWN;

    switch(eth_type) {
        case ETHERTYPE_IP:
            ip = (iphdr*)(pkt_data + sizeof(ether_header));
            direct = checkPktDirectionV4((const in_addr*)&ip->saddr, (const in_addr*)&ip->daddr);
            break;
        case ETHERTYPE_IPV6:
            ipv6 = (ip6_hdr*)(pkt_data + sizeof(ether_header));
            direct = checkPktDirectionV6(&ipv6->ip6_src, &ipv6->ip6_dst);
            break;

        case ETHERTYPE_VLAN: {
            vlan_hdr = (vlan_tag *) (pkt_data + sizeof(ether_header));
            uint16_t vlan_type = ntohs(vlan_hdr->vlan_tci);
            switch (vlan_type) {
                case ETHERTYPE_IP:
                    ip = (iphdr *) (pkt_data + sizeof(ether_header) + sizeof(vlan_tag));
                    direct = checkPktDirectionV4((const in_addr *) &ip->saddr, (const in_addr *) &ip->daddr);
                    break;
            }
        }
        default:
            break;
    }
    
    std::for_each(_exports.begin(), _exports.end(),
                  [header, pkt_data, this, direct](std::shared_ptr<PcapExportBase> pcapExport) {
                      int ret = pcapExport->exportPacket(header, pkt_data, direct);
                      if (pcapExport->getExportType() == exporttype::gre) {
                          if (ret == 0) {
                              this->_gre_count++;
                          } else {
                              this->_gre_drop_count++;
                          }
                      }
                  });
    if (_pcap_dumpter) {
        auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if( _dumpInterval > 0 && tt-_timeStamp > _dumpInterval ) {
            _timeStamp = tt;

            if (openPcapDumper(_pcap_handle) != 0) {
                std::cerr << StatisLogContext::getTimeString() << "Call openPcapDumper failed." << std::endl;
            }
        }

        pcap_dump(reinterpret_cast<u_char*>(_pcap_dumpter), header, pkt_data);
    }
    if (_statislog == nullptr) {
        _statislog = std::make_shared<GreSendStatisLog>(false);
        _statislog->initSendLog("pktminerg");
    }
    _statislog->logSendStatis((uint64_t) (header->ts.tv_sec), header->caplen, _gre_count, _gre_drop_count, 0,
                              _pcap_handle);
    if (_need_update_status) {
        AgentStatus::get_instance()->update_capture_status((uint64_t) (header->ts.tv_sec), header->caplen,
                                                           _gre_count, _gre_drop_count, _pcap_handle);
    }
}

void PcapHandler::addExport(std::shared_ptr<PcapExportBase> pcapExport) {
    _exports.push_back(pcapExport);
}

int PcapHandler::startPcapLoop(int count) {
    if (_pcap_handle == NULL) {
        std::cerr << StatisLogContext::getTimeString() << "The pcap has not created." << std::endl;
        return -1;
    }
    int ret = pcap_loop(_pcap_handle, count, [](uint8_t* user, const struct pcap_pkthdr* h, const uint8_t* data) {
        PcapHandler* p = static_cast<PcapHandler*>(static_cast<void*>(user));
        p->packetHandler(h, data);
    }, reinterpret_cast<uint8_t*>(this));
    if (_statislog == nullptr) {
        _statislog = std::make_shared<GreSendStatisLog>(false);
        _statislog->initSendLog("pktminerg");
    }
    _statislog->logSendStatisGre(std::time(NULL), (uint64_t) std::time(NULL), _gre_count, _gre_drop_count, 0,
                                 _pcap_handle);
    return ret;
}

void PcapHandler::stopPcapLoop() {
    if (_pcap_handle == NULL) {
        std::cerr << StatisLogContext::getTimeString() << "The pcap has not created." << std::endl;
        return;
    }
    pcap_breakloop(_pcap_handle);
}

int PcapHandler::checkPktDirectionV4(const in_addr *sip, const in_addr *dip) {
    for(auto& ipv4 : _ipv4s)
    {
        if(ipv4.s_addr == sip->s_addr)
            return PKTD_OG;
        else if(ipv4.s_addr == dip->s_addr)
            return PKTD_IC;
    }
    return PKTD_UNKNOWN;
}

int PcapHandler::checkPktDirectionV6(const in6_addr *sip, const in6_addr *dip) {
    for(auto& ipv6 : _ipv6s)
    {
        if(ipv6.s6_addr32[0] == sip->s6_addr32[0] &&
           ipv6.s6_addr32[1] == sip->s6_addr32[1] &&
           ipv6.s6_addr32[2] == sip->s6_addr32[2] &&
           ipv6.s6_addr32[3] == sip->s6_addr32[3])
            return PKTD_OG;
        else if(ipv6.s6_addr32[0] == dip->s6_addr32[0] &&
                ipv6.s6_addr32[1] == dip->s6_addr32[1] &&
                ipv6.s6_addr32[2] == dip->s6_addr32[2] &&
                ipv6.s6_addr32[3] == dip->s6_addr32[3])
            return PKTD_IC;
    }
    return PKTD_UNKNOWN;
}

int PcapOfflineHandler::openPcap(const std::string& dev, const pcap_init_t& param, const std::string& expression,
                                 bool dumpfile) {
    pcap_t* pcap_handle = pcap_open_offline(dev.c_str(), _errbuf);
    if (!pcap_handle) {
        std::cerr << StatisLogContext::getTimeString() << "Call pcap_open_offline failed, error is " << _errbuf << "."
                  << std::endl;
        return -1;
    }
    auto pcapGuard = MakeGuard([pcap_handle]() {
        pcap_close(pcap_handle);
    });
    _need_update_status = param.need_update_status;

    if (dumpfile) {
        if (openPcapDumper(pcap_handle) != 0) {
            std::cerr << StatisLogContext::getTimeString() << "Call openPcapDumper failed." << std::endl;
            return -1;
        }
    }
    pcapGuard.Dismiss();
    _pcap_handle = pcap_handle;
    return 0;
}

int PcapLiveHandler::openPcap(const std::string& dev, const pcap_init_t& param, const std::string& expression,
                              bool dumpfile) {
    int ret;
    struct bpf_program filter;
    bpf_u_int32 mask = 0;
    bpf_u_int32 net = 0;
    _need_update_status = param.need_update_status;

    {
        struct ifaddrs* ifaddr;

        _ipv4s.clear();
        _ipv6s.clear();
        if (::getifaddrs(&ifaddr) < 0) {
            return -1;
        }

        for (auto ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if(!ifa->ifa_name || dev != ifa->ifa_name || !ifa->ifa_addr)
                continue;
            if(ifa->ifa_addr->sa_family == AF_INET)
            {
                _ipv4s.push_back(((sockaddr_in*)ifa->ifa_addr)->sin_addr);
            }
            else if(ifa->ifa_addr->sa_family == AF_INET6)
            {
                _ipv6s.push_back(((sockaddr_in6*)ifa->ifa_addr)->sin6_addr);
            }
        }
        freeifaddrs(ifaddr);
    }
    pcap_t* pcap_handle = pcap_create(dev.c_str(), _errbuf);
    if (!pcap_handle) {
        std::cerr << StatisLogContext::getTimeString() << "Call pcap_create failed, error is " << _errbuf << "."
                  << std::endl;
        return -1;
    }
    auto pcapGuard = MakeGuard([pcap_handle]() {
        pcap_close(pcap_handle);
    });

    pcap_set_snaplen(pcap_handle, param.snaplen);
    pcap_set_timeout(pcap_handle, param.timeout);
    pcap_set_promisc(pcap_handle, param.promisc);
    ret = pcap_set_buffer_size(pcap_handle, param.buffer_size);
    if (ret != 0) {
        std::cerr << StatisLogContext::getTimeString() << "Call pcap_set_buffer_size to " << param.buffer_size
                  << " failed." << std::endl;
        return -1;
    }
    ret = pcap_activate(pcap_handle);
    if (ret != 0) {
        std::cerr << StatisLogContext::getTimeString() << "Call pcap_activate failed  error is "
                  << pcap_statustostr(ret) << "." << std::endl;
        return -1;
    }

    if (expression.length() > 0) {
        std::cout << StatisLogContext::getTimeString() << "Set pcap filter as \"" << expression << "\"." << std::endl;
        if (pcap_lookupnet(dev.c_str(), &net, &mask, _errbuf) == -1) {
            std::cerr << StatisLogContext::getTimeString() << " Call pcap_lookupnet failed, error is " << _errbuf << "."
                      << std::endl;
            return -1;
        }

        ret = pcap_compile(pcap_handle, &filter, expression.c_str(), 0, net);
        if (ret != 0) {
            std::cerr << StatisLogContext::getTimeString() << "Call pcap_compile failed, error is "
                      << pcap_statustostr(ret) << "." << std::endl;
            return -1;
        }

        ret = pcap_setfilter(pcap_handle, &filter);
        if (ret != 0) {
            std::cerr << StatisLogContext::getTimeString() << "Call pcap_setfilter failed, error is "
                      << pcap_statustostr(ret) << "." << std::endl;
            return -1;
        }
    }

    if (dumpfile) {
        if (openPcapDumper(pcap_handle) != 0) {
            std::cerr << StatisLogContext::getTimeString() << "Call openPcapDumper failed." << std::endl;
            return -1;
        }
    }
    pcapGuard.Dismiss();
    _pcap_handle = pcap_handle;
    return 0;
}

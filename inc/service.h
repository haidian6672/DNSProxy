#pragma once

#ifndef __DNS_SERVICE_H__
#define __DNS_SERVICE_H__

#include "socketlite.h"
#include "filter.h"

#include "json/json.h"
#include "json/json-forwards.h"

#include <list>
#include <algorithm>
#include <unordered_map>

class clnd_config_service {
protected:
    unordered_map<uint32_t, bool>   a_records_cache_;
    mutex                           a_records_mutex_;
    clnd_protocol_t                 service_protocol_;
    uint16_t                        port_;
    string                          logpath_;
    cp_log_level                    loglv_;
    bool                            daemon_;
    string                          pidfile_;
    uint16_t                        control_port_;

    cp_log_level _loglv_from_string(const string& loglv_string);
public:

    // const reference
    unordered_map<uint32_t, bool>&  a_records_cache;
    const clnd_protocol_t &         service_protocol;
    const uint16_t &                port;
    const string &                  logpath;
    const cp_log_level &            loglv;
    const bool &                    daemon;
    const string &                  pidfile;
    const uint16_t &                control_port;

    clnd_config_service( );
    virtual ~clnd_config_service();

    clnd_config_service( const Json::Value& config_node );

    void start_log() const;
};

typedef shared_ptr<clnd_config_service> service_t;
extern service_t _g_service_config;

#endif

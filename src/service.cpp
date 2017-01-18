#include "service.h"
#include "json-utility.h"

cp_log_level clnd_config_service::_loglv_from_string(const string& loglv_string) {
    string _lowercase = loglv_string;
    std::transform(_lowercase.begin(), _lowercase.end(), _lowercase.begin(), ::tolower);
    cp_log_level _lv = log_info;    // default is info
    if ( loglv_string == "emergancy" ) {
        _lv = log_emergancy;
    } else if ( loglv_string == "alert" ) {
        _lv = log_alert;
    } else if ( loglv_string == "critical" ) {
        _lv = log_critical;
    } else if ( loglv_string == "error" ) {
        _lv = log_error;
    } else if ( loglv_string == "warning" ) {
        _lv = log_warning;
    } else if ( loglv_string == "notice" ) {
        _lv = log_notice;
    } else if ( loglv_string == "info" ) {
        _lv = log_info;
    } else if ( loglv_string == "debug" ) {
        _lv = log_debug;
    }
    return _lv;
}
clnd_config_service::clnd_config_service( ) :
    a_records_cache(a_records_cache_),
    service_protocol(service_protocol_),
    port(port_),
    logpath(logpath_),
    loglv(loglv_),
    daemon(daemon_),
    pidfile(pidfile_), 
    control_port(control_port_)
    { /* nothing */ }
clnd_config_service::~clnd_config_service() { /* nothing */ }

clnd_config_service::clnd_config_service( const Json::Value& config_node ) :
    a_records_cache(a_records_cache_),
    service_protocol(service_protocol_),
    port(port_),
    logpath(logpath_),
    loglv(loglv_),
    daemon(daemon_),
    pidfile(pidfile_),
    control_port(control_port_)
{
    // Service
    daemon_ = check_key_with_default(config_node, "daemon", true).asBool();
    port_ = check_key_with_default(config_node, "port", 53).asUInt();
    loglv_ = _loglv_from_string(
        check_key_with_default(config_node, "loglv", "info").asString());
    service_protocol_ = clnd_protocol_from_string(
        check_key_with_default(config_node, "protocol", "all").asString());
    logpath_ = check_key_with_default(config_node, "logpath", "syslog").asString();
    pidfile_ = check_key_with_default(config_node, "pidfile", "/var/run/cleandns/pid").asString();
}

void clnd_config_service::start_log() const {
    // Stop the existed log
    // cp_log_stop();
    // Check system log path
    if ( logpath_ == "syslog" ) {
        log_arguments::instance().start(loglv_, "cleandns");
    } else if ( logpath_ == "stdout" ) {
        log_arguments::instance().start(stdout, loglv_);
    } else if ( logpath_ == "stderr" ) {
        log_arguments::instance().start(stderr, loglv_);
    } else {
        log_arguments::instance().start(logpath_, loglv_);
    }
}

service_t _g_service_config;

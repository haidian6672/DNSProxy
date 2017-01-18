#include "json/json.h"
#include "json/json-forwards.h"
#include "socketlite.h"

#include "json-utility.h"
#include "filter.h"
#include "service.h"

#include <list>
#include <algorithm>

using namespace cpputility;

#if DEBUG
#define DUMP_HEX(...)       dump_hex(__VA_ARGS__)
#else
#define DUMP_HEX(...)
#endif

int main( int argc, char *argv[] )
{
    Json::Value _config_service;
    Json::Value _config_default_filter;

    if ( argc >= 2 )
    {
        int _arg = 1;
        for ( ; _arg < argc; ++_arg )
        {
            string _command = argv[_arg];
            if ( _command == "-c" || _command == "--config" )
            {
                // read config from json file  
                Json::Value _config_root;
                Json::Reader _config_reader;
                string _config_path = argv[++_arg];
                ifstream _config_stream(_config_path, std::ifstream::binary);
                if ( !_config_stream )
                {
                    cout << "cannot read the file " << _config_path << endl;
                    return 1;
                }
                if ( !_config_reader.parse(_config_stream, _config_root, false ) )
                {
                    cout << _config_reader.getFormattedErrorMessages() << endl;
                    return 1;
                }
                
                _config_service = check_key_and_get_value(_config_root, "service");
                _config_default_filter = check_key_and_get_value(_config_root, "default");
                
                if ( _config_root.isMember("filters") )
                {
                    Json::Value _filter_nodes = _config_root["filters"];
                    if ( _filter_nodes.isArray() == false )
                    {
                        cout << "Invalidate config for filters in config file" << endl;
                        return 3;
                    }
                    for ( Json::ArrayIndex i = 0; i < _filter_nodes.size(); ++i )
                    {
                        lp_clnd_filter _f = create_filter_from_config(_filter_nodes[i]);
                        if ( !_f || !(*_f) )
                        {
                            cout << "failed to load the filter " << i << endl;
                            return 3;
                        }
                        _g_filter_array.push_back(_f);
                    }
                }
                continue;
            }
            
            // only supports config from json file 
            cerr << "Invalidate argument: " << _command << "." << endl;
            return 1;
        }
    }
    else
    {
        cout << "dnsProxy -c [config_file]" << endl;
	return 1;
    }

    // Create the default filter
    _g_default_filter = create_filter_from_config(_config_default_filter);
    if ( !_g_default_filter || !(*_g_default_filter) )
    {
        cout << "Invalidate default filter config" << endl;
        return 3;
    }

    // Start service
    _g_service_config = service_t(new clnd_config_service(_config_service));

    if ( _g_service_config->daemon )
    {
        pid_t _pid = fork();
        if ( _pid < 0 )
        {
            cerr << "Failed to create child process." << endl;
            return 1;
        }
        if ( _pid > 0 )
        {
            // Has create the child process.
            return 0;
        }

        if ( setsid() < 0 )
        {
            cerr << "failed to set session leader for child process." << endl;
            return 3;
        }
    }

    // Start the log service at the beginning
    _g_service_config->start_log();

    bool _system_error = false;

    if ( _g_service_config->service_protocol & clnd_protocol_tcp )
    {
        if ( INVALIDATE_SOCKET == sl_tcp_socket_listen(sl_peerinfo(INADDR_ANY, _g_service_config->port), [&](sl_event e) 
        {
            sl_socket_monitor(e.so, 3, [](sl_event e) 
            {
                string _incoming_buf;
                if ( !sl_tcp_socket_read(e.so, _incoming_buf) )
                {
                    lerror << "failed to read data from the incoming socket " << e.so << lend;
                    sl_socket_close(e.so);
                    return;
                }
                sl_dns_packet _dpkt(_incoming_buf, true);
                if ( !_dpkt.is_validate_query() )
                {
                    lwarning << "receive an invalidate dns query packet from " 
                             << sl_peerinfo(e.address) << lend;
                    sl_socket_close(e.so);
                    return;
                }

                string _domain(move(_dpkt.get_query_domain()));

                // Search a filter
                lp_clnd_filter _f = clnd_search_match_filter(_domain);
                
                // if is local filter, generate a response packet
                if ( _f->mode == clnd_filter_mode_local )
                { 
                    shared_ptr<clnd_filter_local> _lf = dynamic_pointer_cast<clnd_filter_local>(_f);
                    vector<string> _lresult;
                    clnd_local_result_type _type;
                    _lf->get_result_for_domain(_domain, _lresult, _type);

                    sl_dns_packet _rpkt(_dpkt);
                    // A Records
                    if ( _type == clnd_local_result_type_A )
                    {
                        vector<sl_ip> _a_records;
                        for ( auto &_ip : _lresult )
                        {
                            _a_records.emplace_back(sl_ip(_ip));
                        }
                        _rpkt.set_A_records(_a_records);
                    }
                    else
                    {
                        lerror << "only support type_A query." << lend;
                    }
                    _rpkt.set_is_recursive_available(true);
                    // Send and close
                    sl_tcp_socket_send(e.so, _rpkt.to_tcp_packet(), [=](sl_event e) 
                    {
                        sl_socket_close(e.so);
                    });
                    // DUMP local result
                    string _tstr = (_type == clnd_local_result_type_A ? "A:[" : "C:[");
                    for ( auto& _r : _lresult )
                    {
                        linfo << "R:[localhost] D:[" << _domain << "] " << _tstr << _r << "]" << lend;
                    }
                }
                else
                {
                    sl_async_redirect_dns_query(_dpkt, _f->parent, sl_peerinfo(), true, [=](const sl_dns_packet &rpkt)
                    {
                        vector<sl_ip> _a_records(move(rpkt.get_A_records()));
                        for ( auto &_ip : _a_records )
                        {
                            linfo << "R:[" << _f->parent << "] D:[" << _domain << "] A:[" << _ip << "]" << lend;
                        }
                        sl_tcp_socket_send(e.so, rpkt.to_tcp_packet(), [=](sl_event e)
                        {
                            sl_socket_close(e.so);
                        });
                    });
                }
            });
        }) ) _system_error = true;
    }

    sl_events::server().setup(50, NULL);

    if ( _g_service_config->service_protocol & clnd_protocol_udp )
    {
        SOCKET_T _so = sl_udp_socket_init(sl_peerinfo(INADDR_ANY, _g_service_config->port));
        sl_udp_socket_listen(_so, [&](sl_event e)
        {
            sl_peerinfo _ipeer(e.address.sin_addr.s_addr, ntohs(e.address.sin_port));
            string _incoming_buf;
            if ( !sl_udp_socket_read(e.so, e.address, _incoming_buf) )
            {
                lerror 
                    << "failed to read data from the incoming socket " 
                    << _ipeer << lend;
                return;
            }
            sl_dns_packet _dpkt(_incoming_buf, false);
            if ( !_dpkt.is_validate_query() )
            {
                lwarning 
                    << "receive an invalidate dns query packet from " 
                    << sl_peerinfo(e.address) << lend;
                return;
            }
            string _domain(move(_dpkt.get_query_domain()));
            linfo << "the incoming request " << _ipeer << " want to query domain: " << _domain << lend;

            // Search a filter
            lp_clnd_filter _f = clnd_search_match_filter(_domain);
            // if is local filter, generate a response packet
            if ( _f->mode == clnd_filter_mode_local )
            {
                shared_ptr<clnd_filter_local> _lf = dynamic_pointer_cast<clnd_filter_local>(_f);
                vector<string> _lresult;
                clnd_local_result_type _type;
                _lf->get_result_for_domain(_domain, _lresult, _type);

                sl_dns_packet _rpkt(_dpkt);
                // A Records
                if ( _type == clnd_local_result_type_A )
                {
                    vector<sl_ip> _a_records;
                    for ( auto &_ip : _lresult )
                    {
                        _a_records.emplace_back(sl_ip(_ip));
                    }
                    _rpkt.set_A_records(_a_records);
                }
                else
                {
                    lerror << "only support type_A query." << lend;
                }
                _rpkt.set_is_recursive_available(true);
                // Send
                sl_udp_socket_send(e.so, sl_peerinfo(e.address.sin_addr.s_addr, ntohs(e.address.sin_port)), _rpkt);
                // DUMP local result
                string _tstr = (_type == clnd_local_result_type_A ? "A:[" : "C:[");
                for ( auto& _r : _lresult )
                {
                    linfo << "R:[localhost] D:[" << _domain << "] " << _tstr << _r << "]" << lend;
                }
            } 
            else
            {
                sl_async_redirect_dns_query(_dpkt, _f->parent, sl_peerinfo(), false, [=](const sl_dns_packet &rpkt)
                {
                    vector<sl_ip> _a_records(move(rpkt.get_A_records()));
                    for ( auto &_ip : _a_records )
                    {
                        linfo << "R:[" << _f->parent << "] D:[" << _domain << "] A:[" << _ip << "]" << lend;
                    }
                    sl_udp_socket_send(e.so, sl_peerinfo(e.address.sin_addr.s_addr, ntohs(e.address.sin_port)), rpkt);
                });
            }
        });
    }

    if ( !_system_error )
    {
        signal_agent _sa([&](){
            remove(_g_service_config->pidfile.c_str());
            linfo << "dnsProxy terminated" << lend;
        });
    }
    else
    {
        return 1;
    }
    return 0;
}

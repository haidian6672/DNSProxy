#pragma once

#ifndef __DNS_FILTER_H__
#define __DNS_FILTER_H__

#include "socketlite.h"

#include <list>
#include <algorithm>

#include "json/json.h"
#include "json/json-forwards.h"

using namespace std;
using namespace cpputility;

typedef enum {
    clnd_protocol_inhiert   = 0,
    clnd_protocol_tcp       = 0x01,
    clnd_protocol_udp       = 0x02,
    clnd_protocol_all       = clnd_protocol_tcp | clnd_protocol_udp,
} clnd_protocol_t;

// Cast protocol with string
clnd_protocol_t clnd_protocol_from_string(const string &protocol_string);
string clnd_protocol_string(clnd_protocol_t protocol);

typedef enum {
    clnd_filter_mode_unknow,
    clnd_filter_mode_local,
    clnd_filter_mode_redirect
} clnd_filter_mode;

// Cast mode with string
clnd_filter_mode clnd_filter_mode_from_string(const string & mode_string);
string clnd_filter_mode_string(clnd_filter_mode mode);

// Pre-definition of the sub class
class clnd_filter_local;
class clnd_filter_redirect;

// Basic Filter class
class clnd_filter {
protected:
    mutable mutex           filter_mutex_;
    string                  name_;
    clnd_protocol_t         protocol_;
    sl_peerinfo             parent_;
    clnd_filter_mode        mode_;
public: 
    const string &                  name;
    const clnd_protocol_t &         protocol;
    const sl_peerinfo &             parent;
    const clnd_filter_mode &        mode;

    clnd_filter();
    clnd_filter( const Json::Value &config_node, clnd_filter_mode md );

    // Check if is a validate filter
    operator bool() const;
    // Output the detail info other than basic properties
    virtual void output_detail_info(ostream &os) const;
    // Key method: check if a domain match the rules
    virtual bool is_match_filter(const string &query_domain) const = 0;
};

// Output the filter info
ostream & operator << (ostream &os, const clnd_filter* filter);

typedef enum {
    clnd_local_result_type_A        = 1,
    // clnd_local_result_type_CName    = 2
} clnd_local_result_type;

// Local Zone Filter
class clnd_filter_local : public clnd_filter {
    string                              domain_;
    map<string, vector<string> >        A_records_;
    map<string, string>                 CName_records_;
public:
    const string&                       domain;

    clnd_filter_local(const Json::Value &config_node, clnd_filter_mode md);
    virtual void output_detail_info(ostream &os) const;
    virtual bool is_match_filter(const string &query_domain) const;

    // Get the local zone info for specified domain.
    void get_result_for_domain(
        const string &query_domain, 
        vector<string> &results, 
        clnd_local_result_type &type) const;
};

// Redirect Filter
class clnd_filter_redirect : public clnd_filter {
    map< string, bool >             rules_;
public:
    clnd_filter_redirect(const Json::Value &config_node, clnd_filter_mode md);
    virtual void output_detail_info(ostream &os) const;
    virtual bool is_match_filter(const string &query_domain) const;
    void add_rule(const string& domain_rule);
    void del_rule(const string& domain_rule);
};

typedef shared_ptr<clnd_filter> lp_clnd_filter;
// Global Filter Factory
lp_clnd_filter create_filter_from_config(const Json::Value &config_node);

// Global Filters
extern vector< lp_clnd_filter > _g_filter_array;
extern lp_clnd_filter _g_default_filter;

// Search first match fitler or return the default one
lp_clnd_filter clnd_search_match_filter(const string &domain);

// Find a filter by its name, if no such filter, then return NULL
lp_clnd_filter clnd_find_filter_by_name(const string& filter_name);

#endif

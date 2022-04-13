
#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include "cc/json.hh"
#include "cc/http_request.h"
#include "cc/any_types.h"
#include "cc/ci_map.h"
#include "cc/detail.h"
#include "cc/body_parser.h"
namespace cc { template <typename Adaptor, typename Handler, typename ... Middlewares> class Connection; struct Res { private: ci_map headers; public: template <typename Adaptor, typename Handler, typename ... Middlewares> friend class cc::Connection; uint16_t code{ 200 }; std::string body; 
#ifdef ENABLE_COMPRESSION
 bool compressed = true; 
#endif
 bool is_head_response = false;  inline void set_header(const std::string& key, std::string value) { headers.erase(key); headers.emplace(key, std::move(value)); }  inline void add_header(const std::string& key, std::string value) { headers.emplace(key, std::move(value)); } inline const std::string& get_header_value(const std::string& key) { return cc::get_header_value(headers, key); } Res() {} ~Res() {} explicit Res(int code) : code(code) {} Res(std::string body) : body(std::move(body)) {} Res(cc::param& p) : body(p.filename) {} template <unsigned short L> Res(cc::Parser<L>&& P) : body("[") { std::vector<cc::param>* p = &P.params; int i = 1, l = p->size(); if (l) body.push_back('"'),body += p->at(0).filename, body.push_back('"'); for (; i < l; ++i) { body.push_back(','); body.push_back('"'); body += p->at(i).filename; body.push_back('"'); } body.push_back(']'); headers.emplace(RES_CT, RES_AJ); } template <unsigned short L> Res(cc::Parser<L>& P) : body("[") { std::vector<cc::param>* p = &P.params; int i = 1, l = p->size(); if (l) body.push_back('"'), body += p->at(0).filename, body.push_back('"'); for (; i < l; ++i) { body.push_back(','); body.push_back('"'); body += p->at(i).filename; body.push_back('"'); } body.push_back(']'); headers.emplace(RES_CT, RES_AJ); } Res(int code, std::string body) : code(code), body(std::move(body)) {} Res(const json&& json_value) : body(std::move(json_value).dump()) { headers.emplace(RES_CT, RES_AJ); } Res(int code, json& json_value) : code(code), body(json_value.dump()) { headers.emplace(RES_CT, RES_AJ); } Res(const char*&& char_value) : body(std::move(char_value)) {} Res(int code, const char*&& char_value) : code(code), body(std::move(char_value)) {} Res(Res&& r) { *this = std::move(r); } Res& operator = (const Res& r) = delete; Res& operator = (Res&& r) noexcept { body = std::move(r.body); code = r.code; headers = std::move(r.headers); path_ = std::move(r.path_); completed_ = r.completed_; return *this; } inline bool is_completed() const noexcept { return completed_; } inline void clear() { completed_ = false; body.clear(); headers.clear(); }   inline void redirect(const std::string& location) { code = 301; headers.erase(RES_Loc); headers.emplace(RES_Loc, std::move(location)); }  inline void redirect_perm(const std::string& location) { code = 303; headers.erase(RES_Loc); headers.emplace(RES_Loc, std::move(location)); } void write(const std::string& body_part) { body += body_part; } inline void end() { if (!completed_) { completed_ = true; if (is_head_response) { set_header(RES_CL, std::to_string(body.size())); body = ""; } if (complete_request_handler_) { complete_request_handler_(); } } } inline void end(const std::string& body_part) { body += body_part; end(); }  void set_static_file_info(std::string path) { struct stat statbuf_; path_ = detail::directory_; path_ += DecodeURL(path); is_file = stat(path_.c_str(), &statbuf_); if (is_file == 0) {  std::string ss = path.substr(path.find_last_of('.') + 1); std::string_view extension(ss.data(), ss.size()); if (content_types.find(extension) != content_types.end()) { if (ss[0] == 'h' && ss[1] == 't') { is_file = 2; } else {
#ifdef ENABLE_COMPRESSION
 compressed = false;
#endif
 is_file = 1; this->add_header(RES_CL, std::to_string(statbuf_.st_size)); ss = content_types[extension]; this->add_header(RES_CT, ss); } } else { code = 404; this->headers.clear(); this->end();  } } else { code = 404; } } private: std::string path_; int is_file{ 0 }; bool completed_{}; std::function<void()> complete_request_handler_; };}

#pragma once

#include <stdio.h>
#include <string.h>

#include <boost/optional.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace cc {int qs_strncmp(const char* s, const char* qs, size_t n);int qs_parse(char* qs, char* qs_kv[], int qs_kv_size);int qs_decode(char* qs);char* qs_k2v(const char* key, char* const* qs_kv, int qs_kv_size, int nth);char* qs_scanvalue(const char* key, const char* qs, char* val, size_t val_len);
#undef _qsSORTING

#define QS_ISHEX(x)  ((((x) >= '0' && (x) <= '9') || ((x) >= 'A' && (x) <= 'F') ||  ((x) >= 'a' && (x) <= 'f'))    ? 1   : 0)

#define QS_HEX2DEC(x)   (((x) >= '0' && (x) <= '9')  ? (x)-48   : ((x) >= 'A' && (x) <= 'F') ? (x)-55   : ((x) >= 'a' && (x) <= 'f') ? (x)-87  : 0)

#define QS_ISQSCHR(x)  ((((x) == '=') || ((x) == '#') || ((x) == '&') || ((x) == '\0')) ? 0 : 1)

inline int qs_strncmp(const char* s, const char* qs, size_t n){ int i = 0; unsigned char u1, u2, unyb, lnyb; while (n-- > 0) { u1 = static_cast<unsigned char>(*s++); u2 = static_cast<unsigned char>(*qs++); if (!QS_ISQSCHR(u1)) { u1 = '\0'; } if (!QS_ISQSCHR(u2)) { u2 = '\0'; } if (u1 == '+') { u1 = ' '; } if (u1 == '%')  { unyb = static_cast<unsigned char>(*s++); lnyb = static_cast<unsigned char>(*s++); if (QS_ISHEX(unyb) && QS_ISHEX(lnyb)) u1 = (QS_HEX2DEC(unyb) * 16) + QS_HEX2DEC(lnyb); else u1 = '\0'; } if (u2 == '+') { u2 = ' '; } if (u2 == '%')  { unyb = static_cast<unsigned char>(*qs++); lnyb = static_cast<unsigned char>(*qs++); if (QS_ISHEX(unyb) && QS_ISHEX(lnyb)) u2 = (QS_HEX2DEC(unyb) * 16) + QS_HEX2DEC(lnyb); else u2 = '\0'; } if (u1 != u2) return u1 - u2; if (u1 == '\0') return 0; ++i; } if (QS_ISQSCHR(*qs)) return -1; else return 0;}inline int qs_parse(char* qs, char* qs_kv[], int qs_kv_size){ int i, j; char* substr_ptr; for (i = 0; i < qs_kv_size; ++i) qs_kv[i] = NULL;  substr_ptr = qs + strcspn(qs, "?#"); if (substr_ptr[0] != '\0') ++substr_ptr; else return 0;  i = 0; while (i < qs_kv_size) { qs_kv[i] = substr_ptr; j = strcspn(substr_ptr, "&"); if (substr_ptr[j] == '\0') { break; } substr_ptr += j + 1; ++i; } ++i;    for (j = 0; j < i; ++j) { substr_ptr = qs_kv[j] + strcspn(qs_kv[j], "=&#"); if (substr_ptr[0] == '&' || substr_ptr[0] == '\0')  substr_ptr[0] = '\0'; else qs_decode(++substr_ptr); }
#ifdef _qsSORTING

#endif
 return i;}inline int qs_decode(char* qs){ int i = 0, j = 0; while (QS_ISQSCHR(qs[j])) { if (qs[j] == '+') { qs[i] = ' '; } else if (qs[j] == '%')  { if (!QS_ISHEX(qs[j + 1]) || !QS_ISHEX(qs[j + 2])) { qs[i] = '\0'; return i; } qs[i] = (QS_HEX2DEC(qs[j + 1]) * 16) + QS_HEX2DEC(qs[j + 2]); j += 2; } else { qs[i] = qs[j]; } ++i; ++j; } qs[i] = '\0'; return i;}inline char* qs_k2v(const char* key, char* const* qs_kv, int qs_kv_size, int nth = 0){ int i; size_t key_len, skip; key_len = strlen(key);
#ifdef _qsSORTING

#else 
 for (i = 0; i < qs_kv_size; ++i) {  if (qs_strncmp(key, qs_kv[i], key_len) == 0) { skip = strcspn(qs_kv[i], "="); if (qs_kv[i][skip] == '=') ++skip;  if (nth == 0) return qs_kv[i] + skip; else --nth; } }
#endif 
 return nullptr;}inline boost::optional<std::pair<std::string, std::string>> qs_dict_name2kv( const char* dict_name, char* const* qs_kv, int qs_kv_size, int nth = 0){ int i; size_t name_len, skip_to_eq, skip_to_brace_open, skip_to_brace_close; name_len = strlen(dict_name);
#ifdef _qsSORTING

#else 
 for (i = 0; i < qs_kv_size; ++i) { if (strncmp(dict_name, qs_kv[i], name_len) == 0) { skip_to_eq = strcspn(qs_kv[i], "="); if (qs_kv[i][skip_to_eq] == '=') ++skip_to_eq; skip_to_brace_open = strcspn(qs_kv[i], "["); if (qs_kv[i][skip_to_brace_open] == '[') ++skip_to_brace_open; skip_to_brace_close = strcspn(qs_kv[i], "]"); if (skip_to_brace_open <= skip_to_brace_close && skip_to_brace_open > 0 && skip_to_brace_close > 0 && nth == 0) { auto key = std::string(qs_kv[i] + skip_to_brace_open,  skip_to_brace_close - skip_to_brace_open); auto value = std::string(qs_kv[i] + skip_to_eq); return boost::make_optional(std::make_pair(key, value)); } else { --nth; } } }
#endif 
 return boost::none;}inline char* qs_scanvalue(const char* key, const char* qs, char* val, size_t val_len){ size_t i, key_len; const char* tmp;  if ((tmp = strchr(qs, '?')) != NULL) qs = tmp + 1; key_len = strlen(key); while (qs[0] != '#' && qs[0] != '\0') { if (qs_strncmp(key, qs, key_len) == 0) break; qs += strcspn(qs, "&") + 1; } if (qs[0] == '\0') return NULL; qs += strcspn(qs, "=&#"); if (qs[0] == '=') { ++qs; i = strcspn(qs, "&=#");
#ifdef _MSC_VER
 strncpy_s(val, val_len, qs, (val_len - 1) < (i + 1) ? (val_len - 1) : (i + 1));
#else
 strncpy(val, qs, (val_len - 1) < (i + 1) ? (val_len - 1) : (i + 1));
#endif
 qs_decode(val); } else { if (val_len > 0) val[0] = '\0'; } return val;}} 
namespace cc {class query_string{ public: static const int MAX_KEY_VALUE_PAIRS_COUNT = 256; query_string() {} query_string(const query_string& qs) : url_(qs.url_) { for (auto p : qs.key_value_pairs_) { key_value_pairs_.push_back((char*)(p - qs.url_.c_str() + url_.c_str())); } } query_string& operator=(const query_string& qs) { url_ = qs.url_; key_value_pairs_.clear(); for (auto p : qs.key_value_pairs_) { key_value_pairs_.push_back((char*)(p - qs.url_.c_str() + url_.c_str())); } return *this; } query_string& operator=(query_string&& qs) { key_value_pairs_ = std::move(qs.key_value_pairs_); char* old_data = (char*)qs.url_.c_str(); url_ = std::move(qs.url_); for (auto& p : key_value_pairs_) { p += (char*)url_.c_str() - old_data; } return *this; } query_string(std::string url) : url_(std::move(url)) { if (url_.empty()) return; key_value_pairs_.resize(MAX_KEY_VALUE_PAIRS_COUNT); int count = qs_parse(&url_[0], &key_value_pairs_[0], MAX_KEY_VALUE_PAIRS_COUNT); key_value_pairs_.resize(count); } void clear() { key_value_pairs_.clear(); url_.clear(); } friend std::ostream& operator<<(std::ostream& os, const query_string& qs) { os << "[ "; for (size_t i = 0; i < qs.key_value_pairs_.size(); ++i) { if (i) os << ", "; os << qs.key_value_pairs_[i]; } os << " ]"; return os; }     char* get(const std::string& name) const { char* ret = qs_k2v(name.c_str(), key_value_pairs_.data(), key_value_pairs_.size()); return ret; }   char* pop(const std::string& name) { char* ret = get(name); if (ret != nullptr) { for (unsigned int i = 0; i < key_value_pairs_.size(); ++i) { std::string str_item(key_value_pairs_[i]); if (str_item.substr(0, name.size() + 1) == name + '=') { key_value_pairs_.erase(key_value_pairs_.begin() + i); break; } } } return ret; }        std::vector<char*> get_list(const std::string& name, bool use_brackets = true) const { std::vector<char*> ret; std::string plus = name + (use_brackets ? "[]" : ""); char* element = nullptr; int count = 0; while (1) { element = qs_k2v(plus.c_str(), key_value_pairs_.data(),  key_value_pairs_.size(), count++); if (!element) break; ret.push_back(element); } return ret; }  std::vector<char*> pop_list(const std::string& name, bool use_brackets = true) { std::vector<char*> ret = get_list(name, use_brackets); if (!ret.empty()) { for (unsigned int i = 0; i < key_value_pairs_.size(); ++i) { std::string str_item(key_value_pairs_[i]); if ((use_brackets  ? (str_item.substr(0, name.size() + 3) == name + "[]=")  : (str_item.substr(0, name.size() + 1) == name + '='))) { key_value_pairs_.erase(key_value_pairs_.begin() + i--); } } } return ret; }           std::unordered_map<std::string, std::string> get_dict( const std::string& name) const { std::unordered_map<std::string, std::string> ret; int count = 0; while (1) { if (auto element = qs_dict_name2kv(name.c_str(), key_value_pairs_.data(),  key_value_pairs_.size(), count++)) ret.insert(*element); else break; } return ret; }   std::unordered_map<std::string, std::string> pop_dict(const std::string& name) { std::unordered_map<std::string, std::string> ret = get_dict(name); if (!ret.empty()) { for (unsigned int i = 0; i < key_value_pairs_.size(); ++i) { std::string str_item(key_value_pairs_[i]); if (str_item.substr(0, name.size() + 1) == name + '[') { key_value_pairs_.erase(key_value_pairs_.begin() + i--); } } } return ret; } std::vector<std::string> keys() const { std::vector<std::string> ret; for (auto element : key_value_pairs_) { std::string str_element(element); ret.emplace_back(str_element.substr(0, str_element.find('='))); } return ret; } private: std::string url_; std::vector<char*> key_value_pairs_;};} 
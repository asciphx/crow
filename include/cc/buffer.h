
#pragma once
#include <functional>
#include <string_view>
#include <boost/lexical_cast.hpp>
namespace cc { template<unsigned short L = 999> struct Buffer { Buffer(Buffer&& o) : buffer_(o.buffer_), own_buffer_(o.own_buffer_), cursor_(o.cursor_), end_(o.end_), flush_(o.flush_) { o.buffer_ = nullptr; o.own_buffer_ = false; }  Buffer(std::function<void(const char*, int)> flush_ = [this](const char* d, int s) { *this << std::string_view(d, s); }) : buffer_(new char[L]), own_buffer_(true), cursor_(buffer_), end_(buffer_ + L), flush_(flush_) { assert(buffer_); } Buffer(void* buffer, std::function<void(const char*, int)> flush_ = [](const char*, int) {}) : buffer_((char*)buffer), own_buffer_(false), cursor_(buffer_), end_(buffer_ + L), flush_(flush_) { assert(buffer_); } ~Buffer() { if (own_buffer_) delete[] buffer_; } Buffer& operator=(Buffer&& o) { buffer_ = o.buffer_; own_buffer_ = o.own_buffer_; cursor_ = o.cursor_; end_ = o.end_; flush_ = o.flush_; o.buffer_ = nullptr; o.own_buffer_ = false; return *this; } inline void reset() { cursor_ = buffer_; } inline std::size_t size() { return cursor_ - buffer_; } inline bool empty() { return cursor_ == buffer_; } inline void flush() { flush_(buffer_, int(size())); reset(); } Buffer& operator<<(std::string_view s) { if (cursor_ + s.size() >= end_) flush(); assert(cursor_ + s.size() < end_); memcpy(cursor_, s.data(), s.size()); cursor_ += s.size(); return *this; } Buffer& operator<<(const char* s) { return operator<<(std::string_view(s, strlen(s))); } Buffer& operator<<(char v) { cursor_[0] = v; ++cursor_; return *this; } inline Buffer& append(const char c) { return (*this) << c; } inline Buffer& assign(const char* s, const char* e) { size_t l = e - s; if (cursor_ + l >= end_) flush(); for (int i = -1; ++i < l; *cursor_ = s[i], ++cursor_); return *this; } Buffer& operator<<(std::size_t v) { if (v == 0) operator<<('0'); char buffer[10]; char* str_start = buffer; for (int i = 0; i < 10; ++i) { if (v > 0) str_start = buffer + 9 - i; buffer[9 - i] = (v % 10) + '0'; v /= 10; } operator<<(std::string_view(str_start, buffer + 10 - str_start)); return *this; } template <typename I> Buffer& operator<<(I v) { typedef std::array<char, 150> buf_t; buf_t b = boost::lexical_cast<buf_t>(v); return operator<<(std::string_view(b.data(), strlen(b.data()))); } std::string_view to_string_view() { return std::string_view(buffer_, cursor_ - buffer_); } std::string to_string() { std::string s = (const char*)buffer_; s[cursor_ - buffer_] = (char)0; return s; } inline void clear() { delete[] buffer_; buffer_ = new char[L]; cursor_ = buffer_; end_ = buffer_ + L; own_buffer_ = true; } char* buffer_; char* cursor_; std::function<void(const char* s, int d)> flush_; private: char* end_; bool own_buffer_; };}
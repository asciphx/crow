#pragma once
#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/array.hpp>
#include <atomic>
#include <chrono>
#include <vector>

#include "crow/http_parser_merged.h"

#include "crow/parser.h"
#include "crow/http_response.h"
#include "crow/logging.h"
#include "crow/settings.h"
#include "crow/detail.h"
#include "crow/middleware_context.h"
#include "crow/socket_adaptors.h"
#include "crow/compression.h"
static char Res_server_tag[9]="Server: ",Res_keep_alive_tag[23]="Connection: Keep-Alive",Res_content_length_tag[17]="Content-Length: ";
static char Res_date_tag[7]="Date: ",Res_content_length[15]="content-length",Res_seperator[3]=": ",Res_crlf[3]="\r\n";
static const std::unordered_map<int,std::string> Res_statusCodes={
	{200, "HTTP/1.1 200 OK\r\n"},
	{201, "HTTP/1.1 201 Created\r\n"},
	{202, "HTTP/1.1 202 Accepted\r\n"},
	{204, "HTTP/1.1 204 No Content\r\n"},

	{300, "HTTP/1.1 300 Multiple Choices\r\n"},
	{301, "HTTP/1.1 301 Moved Permanently\r\n"},
	{302, "HTTP/1.1 302 Found\r\n"},
	{303, "HTTP/1.1 303 See Other\r\n"},
	{304, "HTTP/1.1 304 Not Modified\r\n"},

	{400, "HTTP/1.1 400 Bad Request\r\n"},
	{401, "HTTP/1.1 401 Unauthorized\r\n"},
	{403, "HTTP/1.1 403 Forbidden\r\n"},
	{404, "HTTP/1.1 404 Not Found\r\n"},
	{405, "HTTP/1.1 405 Method Not Allowed\r\n"},
	{413, "HTTP/1.1 413 Payload Too Large\r\n"},
	{422, "HTTP/1.1 422 Unprocessable Entity\r\n"},
	{429, "HTTP/1.1 429 Too Many Requests\r\n"},

	{500, "HTTP/1.1 500 Internal Server Error\r\n"},
	{501, "HTTP/1.1 501 Not Implemented\r\n"},
	{502, "HTTP/1.1 502 Bad Gateway\r\n"},
	{503, "HTTP/1.1 503 Service Unavailable\r\n"},
};
namespace crow {
  using namespace boost;
  using tcp=asio::ip::tcp;

  namespace detail {
	template <typename MW>
	struct check_before_handle_arity_3_const {
	  template <typename T,
		void (T::*)(Req&,Res&,typename MW::Ctx&) const=&T::before_handle
	  >
		struct get {};
	};

	template <typename MW>
	struct check_before_handle_arity_3 {
	  template <typename T,
		void (T::*)(Req&,Res&,typename MW::Ctx&)=&T::before_handle
	  >
		struct get {};
	};

	template <typename MW>
	struct check_after_handle_arity_3_const {
	  template <typename T,
		void (T::*)(Req&,Res&,typename MW::Ctx&) const=&T::after_handle
	  >
		struct get {};
	};

	template <typename MW>
	struct check_after_handle_arity_3 {
	  template <typename T,
		void (T::*)(Req&,Res&,typename MW::Ctx&)=&T::after_handle
	  >
		struct get {};
};

	template <typename T>
	struct is_before_handle_arity_3_impl {
	  template <typename C>
	  static std::true_type f(typename check_before_handle_arity_3_const<T>::template get<C>*);

	  template <typename C>
	  static std::true_type f(typename check_before_handle_arity_3<T>::template get<C>*);

	  template <typename C>
	  static std::false_type f(...);

	  public:
	  static const bool value=decltype(f<T>(nullptr))::value;
	};

	template <typename T>
	struct is_after_handle_arity_3_impl {
	  template <typename C>
	  static std::true_type f(typename check_after_handle_arity_3_const<T>::template get<C>*);

	  template <typename C>
	  static std::true_type f(typename check_after_handle_arity_3<T>::template get<C>*);

	  template <typename C>
	  static std::false_type f(...);

	  public:
	  static const bool value=decltype(f<T>(nullptr))::value;
	};

	template <typename MW,typename Context,typename ParentContext>
	typename std::enable_if<!is_before_handle_arity_3_impl<MW>::value>::type
	  before_handler_call(MW& mw,Req& req,Res& res,Context& ctx,ParentContext& /*parent_ctx*/) {
	  mw.before_handle(req,res,ctx.template get<MW>(),ctx);
	}

	template <typename MW,typename Context,typename ParentContext>
	typename std::enable_if<is_before_handle_arity_3_impl<MW>::value>::type
	  before_handler_call(MW& mw,Req& req,Res& res,Context& ctx,ParentContext& /*parent_ctx*/) {
	  mw.before_handle(req,res,ctx.template get<MW>());
	}

	template <typename MW,typename Context,typename ParentContext>
	typename std::enable_if<!is_after_handle_arity_3_impl<MW>::value>::type
	  after_handler_call(MW& mw,Req& req,Res& res,Context& ctx,ParentContext& /*parent_ctx*/) {
	  mw.after_handle(req,res,ctx.template get<MW>(),ctx);
	}

	template <typename MW,typename Context,typename ParentContext>
	typename std::enable_if<is_after_handle_arity_3_impl<MW>::value>::type
	  after_handler_call(MW& mw,Req& req,Res& res,Context& ctx,ParentContext& /*parent_ctx*/) {
	  mw.after_handle(req,res,ctx.template get<MW>());
	}

	template <int N,typename Context,typename Container,typename CurrentMW,typename ... Middlewares>
	bool middleware_call_helper(Container& middlewares,Req& req,Res& res,Context& ctx) {
	  using parent_context_t=typename Context::template partial<N-1>;
	  before_handler_call<CurrentMW,Context,parent_context_t>(std::get<N>(middlewares),req,res,ctx,static_cast<parent_context_t&>(ctx));
	  if (res.is_completed()) {
		after_handler_call<CurrentMW,Context,parent_context_t>(std::get<N>(middlewares),req,res,ctx,static_cast<parent_context_t&>(ctx));
		return true;
	  }
	  if (middleware_call_helper<N+1,Context,Container,Middlewares...>(middlewares,req,res,ctx)) {
		after_handler_call<CurrentMW,Context,parent_context_t>(std::get<N>(middlewares),req,res,ctx,static_cast<parent_context_t&>(ctx));
		return true;
	  }
	  return false;
	}

	template <int N,typename Context,typename Container>
	bool middleware_call_helper(Container& /*middlewares*/,Req& /*req*/,Res& /*res*/,Context& /*ctx*/) {
	  return false;
	}

	template <int N,typename Context,typename Container>
	typename std::enable_if<(N<0)>::type
	  after_handlers_call_helper(Container& /*middlewares*/,Context& /*context*/,Req& /*req*/,Res& /*res*/) {}

	template <int N,typename Context,typename Container>
	typename std::enable_if<(N==0)>::type after_handlers_call_helper(Container& middlewares,Context& ctx,Req& req,Res& res) {
	  using parent_context_t=typename Context::template partial<N-1>;
	  using CurrentMW=typename std::tuple_element<N,typename std::remove_reference<Container>::type>::type;
	  after_handler_call<CurrentMW,Context,parent_context_t>(std::get<N>(middlewares),req,res,ctx,static_cast<parent_context_t&>(ctx));
	}

	template <int N,typename Context,typename Container>
	typename std::enable_if<(N>0)>::type after_handlers_call_helper(Container& middlewares,Context& ctx,Req& req,Res& res) {
	  using parent_context_t=typename Context::template partial<N-1>;
	  using CurrentMW=typename std::tuple_element<N,typename std::remove_reference<Container>::type>::type;
	  after_handler_call<CurrentMW,Context,parent_context_t>(std::get<N>(middlewares),req,res,ctx,static_cast<parent_context_t&>(ctx));
	  after_handlers_call_helper<N-1,Context,Container>(middlewares,ctx,req,res);
	}
  }

  /// An HTTP connection.
  template <typename Adaptor,typename Handler,typename ... Middlewares>
  class Connection {
	friend struct crow::Res;
	public:
	Connection(
	  boost::asio::io_service& io_service,
	  Handler* handler,
	  const std::string& server_name,
	  std::tuple<Middlewares...>* middlewares,
	  std::function<std::string()>& get_cached_date_str_f,
	  detail::dumb_timer_queue& timer_queue,
	  typename Adaptor::Ctx* adaptor_ctx_
	)
	  : adaptor_(io_service,adaptor_ctx_),
	  handler_(handler),
	  parser_(this),
	  server_name_(server_name),
	  middlewares_(middlewares),
	  get_cached_date_str(get_cached_date_str_f),
	  timer_queue(timer_queue) {
	}

	~Connection() {
	  res.complete_request_handler_=nullptr;
	  cancel_deadline_timer();
	}

	/// The TCP socket on top of which the connection is established.
	decltype(std::declval<Adaptor>().raw_socket())& socket() {
	  return adaptor_.raw_socket();
	}

	void start() {
	  adaptor_.start([this](const boost::system::error_code& ec) {
		if (!ec) {
		  start_deadline();
		  do_read();
		} else {
		  delete this;
		}
	  });
	}

	void handle_header() {
	  // HTTP 1.1 Expect: 100-continue
	  if (parser_.check_version(1,1)&&parser_.headers.count("expect")&&get_header_value(parser_.headers,"expect")=="100-continue") {
		buffers_.clear();
		static std::string expect_100_continue="HTTP/1.1 100 Continue\r\n\r\n";
		buffers_.emplace_back(expect_100_continue.data(),expect_100_continue.size());
		do_write();
	  }
	}

	void handle() {
	  cancel_deadline_timer();
	  bool is_invalid_request=false;
	  req_=parser_.to_request();
	  req_.remoteIpAddress=adaptor_.remote_endpoint().address().to_string();
	  if (parser_.check_version(1,0)) {// HTTP/1.0
		  close_connection_=true;
	  } else if (parser_.check_version(1,1)) {// HTTP/1.1
		if (req_.headers.count("Connection")&&req_.get_header_value("Connection")=="close") close_connection_=true;
		if (!req_.headers.count("host")) {
		  is_invalid_request=true;res=Res(400);
		}
		if (parser_.is_upgrade()) {
		  if (req_.get_header_value("upgrade")=="h2c") {
			// TODO HTTP/2 currently, ignore upgrade header
		  } else {
			close_connection_=true;
			handler_->handle_upgrade(req_,res,std::move(adaptor_));
			return;
		  }
		}
	  }
	  CROW_LOG_INFO<<"Request: "<<boost::lexical_cast<std::string>(adaptor_.remote_endpoint())<<" "<<this<<" HTTP/"<<parser_.http_major<<"."<<parser_.http_minor<<' '
		<<method_name(req_.method)<<" "<<req_.url;
	  need_to_call_after_handlers_=false;
	  if (!is_invalid_request) {
		res.complete_request_handler_=[] {};
		res.is_alive_helper_=[this]()->bool { return adaptor_.is_open(); };

		ctx_=detail::Ctx<Middlewares...>();
		req_.middleware_context=static_cast<void*>(&ctx_);
		req_.io_service=&adaptor_.get_io_service();
		detail::middleware_call_helper<0,decltype(ctx_),decltype(*middlewares_),Middlewares...>(*middlewares_,req_,res,ctx_);

		if (!res.completed_) {
		  res.complete_request_handler_=[this] { this->complete_request(); };
		  need_to_call_after_handlers_=true;
		  handler_->handle(req_,res);
		} else {
		  complete_request();
		}
	  } else {
		complete_request();
	  }
	}
	/// Call the after handle middleware and send the write the Res to the connection.
	void complete_request() {
	  CROW_LOG_INFO<<"Response: "<<this<<' '<<req_.raw_url<<' '<<res.code<<' '<<close_connection_;
	  if (need_to_call_after_handlers_) {
		need_to_call_after_handlers_=false;

		// call all after_handler of middlewares
		detail::after_handlers_call_helper<
		  (static_cast<int>(sizeof...(Middlewares))-1),
		  decltype(ctx_),
		  decltype(*middlewares_)>
		  (*middlewares_,ctx_,req_,res);
	  }
#ifdef CROW_ENABLE_COMPRESSION
	  std::string accept_encoding=req_.get_header_value("Accept-Encoding");
	  if (!accept_encoding.empty()&&res.compressed) {
		switch (handler_->compression_algorithm()) {
		  case compression::DEFLATE:
		  if (accept_encoding.find("deflate")!=std::string::npos) {
			res.body=compression::compress_string(res.body,compression::algorithm::DEFLATE);
			res.set_header("Content-Encoding","deflate");
		  }
		  break;
		  case compression::GZIP:
		  if (accept_encoding.find("gzip")!=std::string::npos) {
			res.body=compression::compress_string(res.body,compression::algorithm::GZIP);
			res.set_header("Content-Encoding","gzip");
		  }
		  break;
		  default:
		  break;
		}
	  }
#endif
	  //if there is a redirection with a partial URL, treat the URL as a route.
	  std::string location=res.get_header_value("Location");
	  if (!location.empty()&&location.find("://",0)==std::string::npos) {
#ifdef CROW_ENABLE_SSL
		location.insert(0,"https://"+req_.get_header_value("Host"));
#else
		location.insert(0,"http://"+req_.get_header_value("Host"));
#endif
		res.set_header("location",location);
	  }

	  prepare_buffers();
	  if (res.is_file) do_write_static();else do_write_general();
	}

	private:
	void prepare_buffers() {
	  //auto self = this->shared_from_this();
	  res.complete_request_handler_=nullptr;
	  if (res.is_file||!adaptor_.is_open()) {
		//CROW_LOG_DEBUG << this << " delete (socket is closed) " << is_reading << ' ' << is_writing;
		//delete this;
		return;
	  }
	  //if (res.body.empty()) {}//res.body
	  buffers_.clear();buffers_.reserve(4*(res.headers.size()+5)+3);
	 // if (!Res_statusCodes.count(res.code)) res.code=500;
	  std::string status=Res_statusCodes.find(res.code)->second;
	  buffers_.emplace_back(status.data(),status.size());
	  if (res.code>399) res.body=status.substr(9);
	  for (auto& kv:res.headers) {
		buffers_.emplace_back(kv.first.data(),kv.first.size());
		buffers_.emplace_back(Res_seperator,2);
		buffers_.emplace_back(kv.second.data(),kv.second.size());
		buffers_.emplace_back(Res_crlf,2);
	  }
	  content_length_=std::to_string(res.body.size());
	  buffers_.emplace_back(Res_content_length_tag,16);
	  buffers_.emplace_back(content_length_.data(),content_length_.size());
	  buffers_.emplace_back(Res_crlf,2);

	  buffers_.emplace_back(Res_server_tag,8);
	  buffers_.emplace_back(server_name_.data(),server_name_.size());
	  buffers_.emplace_back(Res_crlf,2);

	  date_str_=get_cached_date_str();
	  buffers_.emplace_back(Res_date_tag,6);
	  buffers_.emplace_back(date_str_.data(),date_str_.size());
	  buffers_.emplace_back(Res_crlf,2);

	  buffers_.emplace_back(Res_crlf,2);
	}

	void do_write_static() {
	  boost::asio::write(adaptor_.socket(),buffers_);
	  res.do_stream_file(adaptor_);
	  res.end();
	  res.clear();
	  buffers_.clear();
	}

	void do_write_general() {
	  if (res.body.length()<res_stream_threshold_) {
		res_body_copy_.swap(res.body);
		buffers_.emplace_back(res_body_copy_.data(),res_body_copy_.size());
		do_write();
		if (need_to_start_read_after_complete_) {
		  need_to_start_read_after_complete_=false;
		  start_deadline();
		  do_read();
		}
	  } else {
		is_writing=true;
		boost::asio::write(adaptor_.socket(),buffers_);
		res.do_stream_body(adaptor_);
		res.end();
		res.clear();
		buffers_.clear();
	  }
	}

	void do_read() {
	  //auto self = this->shared_from_this();
	  is_reading=true;
	  adaptor_.socket().async_read_some(boost::asio::buffer(buffer_),
										[this](const boost::system::error_code& ec,std::size_t bytes_transferred) {
		bool error_while_reading=true;
		if (!ec) {
		  bool ret=parser_.feed(buffer_.data(),bytes_transferred);
		  if (ret&&adaptor_.is_open()) {
			error_while_reading=false;
		  }
		}

		if (error_while_reading) {
		  cancel_deadline_timer();
		  parser_.done();
		  adaptor_.shutdown_read();
		  adaptor_.close();
		  delete this;
		} else if (close_connection_) {
		  cancel_deadline_timer();
		  parser_.done();
		  is_reading=false;
		  check_destroy();
		  // adaptor will close after write
		} else if (!need_to_call_after_handlers_) {
		  start_deadline();
		  do_read();
		} else {
		  // res will be completed later by user
		  need_to_start_read_after_complete_=true;
		}
	  });
	}

	void do_write() {
	  //auto self = this->shared_from_this();
	  is_writing=true;
	  boost::asio::async_write(adaptor_.socket(),buffers_,
							   [&](const boost::system::error_code& ec,std::size_t /*bytes_transferred*/) {
		is_writing=false;
		res.clear();
		res_body_copy_.clear();
		if (!ec) {
		  if (close_connection_) {
			adaptor_.shutdown_write();
			adaptor_.close();
			CROW_LOG_DEBUG<<this<<" from write(1)";
			check_destroy();
		  }
		} else {
		  delete this;
		}
	  });
	}

	void check_destroy() {
	  CROW_LOG_DEBUG<<this<<" is_reading "<<is_reading<<" is_writing "<<is_writing;
	  if (!is_reading&&!is_writing) {
		CROW_LOG_DEBUG<<this<<" delete (idle) ";
		delete this;
	  }
	  //delete this;
	}

	void cancel_deadline_timer() {
	  CROW_LOG_DEBUG<<this<<" timer cancelled: "<<timer_cancel_key_.first<<' '<<timer_cancel_key_.second;
	  timer_queue.cancel(timer_cancel_key_);
	}

	void start_deadline(/*int timeout = 5*/) {
	  cancel_deadline_timer();

	  timer_cancel_key_=timer_queue.add([this] {
		if (!adaptor_.is_open()) {
		  return;
		}
		adaptor_.shutdown_readwrite();
		adaptor_.close();
	  });
	  CROW_LOG_DEBUG<<this<<" timer added: "<<timer_cancel_key_.first<<' '<<timer_cancel_key_.second;
	}

	private:
	Adaptor adaptor_;
	Handler* handler_;

	boost::array<char,4096> buffer_;

	const unsigned res_stream_threshold_=1048576;

	HTTPParser<Connection> parser_;
	Req req_;
	Res res;

	bool close_connection_=false;

	const std::string& server_name_;
	std::vector<boost::asio::const_buffer> buffers_;

	std::string content_length_;
	std::string date_str_;
	std::string res_body_copy_;

	//boost::asio::deadline_timer deadline_;
	detail::dumb_timer_queue::key timer_cancel_key_;

	bool is_reading{};
	bool is_writing{};
	bool need_to_call_after_handlers_{};
	bool need_to_start_read_after_complete_{};
	std::tuple<Middlewares...>* middlewares_;
	detail::Ctx<Middlewares...> ctx_;

	std::function<std::string()>& get_cached_date_str;
	detail::dumb_timer_queue& timer_queue;
	};

  }

//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_HPP
#define BOOST_HTTP_HPP

#include <boost/http/bcrypt.hpp>
#include <boost/http/error.hpp>
#include <boost/http/field.hpp>
#include <boost/http/fields.hpp>
#include <boost/http/fields_base.hpp>
#include <boost/http/file.hpp>
#include <boost/http/file_mode.hpp>
#include <boost/http/header_limits.hpp>
#include <boost/http/message_base.hpp>
#include <boost/http/metadata.hpp>
#include <boost/http/method.hpp>
#include <boost/http/parser.hpp>
#include <boost/http/request.hpp>
#include <boost/http/request_base.hpp>
#include <boost/http/request_parser.hpp>
#include <boost/http/response.hpp>
#include <boost/http/response_base.hpp>
#include <boost/http/serializer.hpp>
#include <boost/http/static_request.hpp>
#include <boost/http/static_response.hpp>
#include <boost/http/status.hpp>
#include <boost/http/string_body.hpp>
#include <boost/http/version.hpp>
 
#include <boost/http/rfc/combine_field_values.hpp>
#include <boost/http/rfc/list_rule.hpp>
//#include <boost/http/rfc/media_type.hpp>
#include <boost/http/rfc/parameter.hpp>
#include <boost/http/rfc/quoted_token_rule.hpp>
#include <boost/http/rfc/quoted_token_view.hpp>
#include <boost/http/rfc/token_rule.hpp>
#include <boost/http/rfc/upgrade_rule.hpp>

#include <boost/http/server/cors.hpp>
#include <boost/http/server/router.hpp>
#include <boost/http/server/basic_router.hpp>
#include <boost/http/server/router_types.hpp>

#endif

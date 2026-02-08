//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_RFC_COMBINE_FIELD_VALUES_HPP
#define BOOST_HTTP_RFC_COMBINE_FIELD_VALUES_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/fields_base.hpp>
#include <boost/url/grammar/recycled.hpp>
#include <boost/core/detail/string_view.hpp>
#include <string>

namespace boost {
namespace http {

//------------------------------------------------

/** Return a comma-separated list of field values

    This function combines a set of zero or
    fields with the same name into a single
    comma-separated string. The algorithm
    sometimes needs temporary storage, which
    is obtained if necessary from `temp`.
    <br>
    Ownership is not transferred; the caller
    is responsible for ensuring that the
    lifetime of both the character buffer
    underlying the fields and the temporary
    recycled string extend until the string
    returned by this function is no longer
    referenced.

    @par Example
    This combines any Content-Length fields
    present in `h` into a single string.
    @code
    grammar::recycled_ptr< std::string > temp( nullptr );

    string_view s = combine_field_values( h.find_all( field::content_length ), temp );
    @endcode

    @param vr The subrange of fields to combine.

    @param temp A temporary recycled string
    which may be used if necessary. The pointer
    may be empty.
*/
BOOST_HTTP_DECL
core::string_view
combine_field_values(
    fields_base::subrange const& vr,
    grammar::recycled_ptr<std::string>& temp);

} // http
} // boost

#endif

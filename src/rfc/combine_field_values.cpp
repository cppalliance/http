//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/rfc/combine_field_values.hpp>

namespace boost {
namespace http {

core::string_view
combine_field_values(
    fields_base::subrange const& vr,
    grammar::recycled_ptr<std::string>& temp)
{
    core::string_view result;
    bool acquired = false;
    for(auto s : vr)
    {
        if(s.empty())
            continue;
        if(result.empty())
        {
            result = s;
        }
        else if(! acquired)
        {
            acquired = true;
            if(temp.empty())
                temp.acquire();
            temp->clear();
            temp->reserve(
                result.size() +
                1 + s.size());
            *temp = result;
            temp->push_back(',');
            temp->append(
                s.data(), s.size());
            result = *temp;
        }
        else
        {
            temp->reserve(
                temp->size() +
                1 + s.size());
            temp->push_back(',');
            temp->append(
                s.data(), s.size());
            result = *temp;
        }
    }
    return result;
}

} // http
} // boost

//
// Copyright (c) 2026 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/zstd/error.hpp>

namespace boost {
namespace http {
namespace zstd {
namespace detail {

const char*
error_cat_type::
name() const noexcept
{
    return "boost.http.zstd";
}

bool
error_cat_type::
failed(int ev) const noexcept
{
    return ev != 0;
}

std::string
error_cat_type::
message(int ev) const
{
    return message(ev, nullptr, 0);
}

char const*
error_cat_type::
message(
    int ev,
    char*,
    std::size_t) const noexcept
{
    switch(static_cast<error>(ev))
    {
    case error::no_error: return "no_error";
    case error::generic: return "generic";
    case error::prefix_unknown: return "prefix_unknown";
    case error::version_unsupported: return "version_unsupported";
    case error::frame_parameter_unsupported: return "frame_parameter_unsupported";
    case error::frame_parameter_window_too_large: return "frame_parameter_window_too_large";
    case error::corruption_detected: return "corruption_detected";
    case error::checksum_wrong: return "checksum_wrong";
    case error::literals_header_wrong: return "literals_header_wrong";
    case error::dictionary_corrupted: return "dictionary_corrupted";
    case error::dictionary_wrong: return "dictionary_wrong";
    case error::dictionary_creation_failed: return "dictionary_creation_failed";
    case error::parameter_unsupported: return "parameter_unsupported";
    case error::parameter_combination_unsupported: return "parameter_combination_unsupported";
    case error::parameter_out_of_bound: return "parameter_out_of_bound";
    case error::table_log_too_large: return "table_log_too_large";
    case error::max_symbol_value_too_large: return "max_symbol_value_too_large";
    case error::max_symbol_value_too_small: return "max_symbol_value_too_small";
    case error::cannot_produce_uncompressed_block: return "cannot_produce_uncompressed_block";
    case error::stability_condition_not_respected: return "stability_condition_not_respected";
    case error::stage_wrong: return "stage_wrong";
    case error::init_missing: return "init_missing";
    case error::memory_allocation: return "memory_allocation";
    case error::work_space_too_small: return "work_space_too_small";
    case error::dst_size_too_small: return "dst_size_too_small";
    case error::src_size_wrong: return "src_size_wrong";
    case error::dst_buffer_null: return "dst_buffer_null";
    case error::no_forward_progress_dest_full: return "no_forward_progress_dest_full";
    case error::no_forward_progress_input_empty: return "no_forward_progress_input_empty";
    default:
        return "unknown";
    }
}

// msvc 14.0 has a bug that warns about inability
// to use constexpr construction here, even though
// there's no constexpr construction
#if defined(_MSC_VER) && _MSC_VER <= 1900
# pragma warning( push )
# pragma warning( disable : 4592 )
#endif

#if defined(__cpp_constinit) && __cpp_constinit >= 201907L
constinit error_cat_type error_cat;
#else
error_cat_type error_cat;
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1900
# pragma warning( pop )
#endif

} // detail
} // zstd
} // http
} // boost

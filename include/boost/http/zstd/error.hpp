//
// Copyright (c) 2026 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_ZSTD_ERROR_HPP
#define BOOST_HTTP_ZSTD_ERROR_HPP

#include <boost/http/detail/config.hpp>

namespace boost {
namespace http {
namespace zstd {

/** Error codes returned from compression and decompression functions.

    Zstandard functions returning `std::size_t` signal failure
    by returning an error code encoded in the result. Use the
    service's `is_error` function to test a result and its
    `get_error_code` function to convert it to one of these
    values. Only `no_error` is a success; every other value
    is a failure.
*/
enum class error
{
    no_error = 0,
    generic  = 1,

    /* Errors caused by invalid input */
    prefix_unknown                   = 10,
    version_unsupported              = 12,
    frame_parameter_unsupported      = 14,
    frame_parameter_window_too_large = 16,
    corruption_detected              = 20,
    checksum_wrong                   = 22,
    literals_header_wrong            = 24,

    /* Dictionary errors */
    dictionary_corrupted       = 30,
    dictionary_wrong           = 32,
    dictionary_creation_failed = 34,

    /* Parameter errors */
    parameter_unsupported             = 40,
    parameter_combination_unsupported = 41,
    parameter_out_of_bound            = 42,
    table_log_too_large               = 44,
    max_symbol_value_too_large        = 46,
    max_symbol_value_too_small        = 48,
    cannot_produce_uncompressed_block = 49,
    stability_condition_not_respected = 50,

    /* Usage errors */
    stage_wrong          = 60,
    init_missing         = 62,
    memory_allocation    = 64,
    work_space_too_small = 66,

    /* Buffer errors */
    dst_size_too_small = 70,
    src_size_wrong     = 72,
    dst_buffer_null    = 74,

    /* Streaming progress errors */
    no_forward_progress_dest_full   = 80,
    no_forward_progress_input_empty = 82
};

} // zstd
} // http
} // boost

#include <boost/http/zstd/impl/error.hpp>

#endif

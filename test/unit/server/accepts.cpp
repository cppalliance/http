//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/server/accepts.hpp>

#include <boost/http/field.hpp>
#include <boost/http/request.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http {

struct accepts_test
{
    void
    testType()
    {
        // No Accept header returns first offered
        {
            request req( method::get, "/" );
            accepts ac( req );
            BOOST_TEST( ac.type({ "html", "json" })
                == "html" );
        }

        // Accept: text/html
        {
            request req( method::get, "/" );
            req.set( field::accept, "text/html" );
            accepts ac( req );
            BOOST_TEST( ac.type({ "html" })
                == "html" );
            BOOST_TEST( ac.type({ "text/html" })
                == "text/html" );
            BOOST_TEST( ac.type({ "json" }).empty() );
        }

        // Accept: text/*, application/json
        {
            request req( method::get, "/" );
            req.set( field::accept,
                "text/*, application/json" );
            accepts ac( req );
            BOOST_TEST( ac.type({ "html" })
                == "html" );
            BOOST_TEST( ac.type({ "text/html" })
                == "text/html" );
            BOOST_TEST( ac.type({ "json", "text" })
                == "json" );
            BOOST_TEST( ac.type({ "application/json" })
                == "application/json" );
        }

        // Accept: text/*, application/json - no match
        {
            request req( method::get, "/" );
            req.set( field::accept,
                "text/*, application/json" );
            accepts ac( req );
            BOOST_TEST( ac.type({ "png" }).empty() );
        }

        // Accept: text/*;q=.5, application/json
        {
            request req( method::get, "/" );
            req.set( field::accept,
                "text/*;q=.5, application/json" );
            accepts ac( req );
            BOOST_TEST( ac.type({ "html", "json" })
                == "json" );
        }

        // Accept: */*
        {
            request req( method::get, "/" );
            req.set( field::accept, "*/*" );
            accepts ac( req );
            BOOST_TEST( ac.type({ "json" })
                == "json" );
        }

        // Empty offered list
        {
            request req( method::get, "/" );
            req.set( field::accept, "text/html" );
            accepts ac( req );
            BOOST_TEST( ac.type({}).empty() );
        }
    }

    void
    testTypes()
    {
        // No Accept header
        {
            request req( method::get, "/" );
            accepts ac( req );
            auto v = ac.types();
            BOOST_TEST( v.empty() );
        }

        // Single type
        {
            request req( method::get, "/" );
            req.set( field::accept, "text/html" );
            accepts ac( req );
            auto v = ac.types();
            BOOST_TEST( v.size() == 1 );
            if( ! v.empty() )
                BOOST_TEST( v[0] == "text/html" );
        }

        // Multiple types sorted by quality
        {
            request req( method::get, "/" );
            req.set( field::accept,
                "text/plain;q=0.5, "
                "application/json, "
                "text/html;q=0.9" );
            accepts ac( req );
            auto v = ac.types();
            BOOST_TEST( v.size() == 3 );
            if( v.size() >= 3 )
            {
                BOOST_TEST( v[0] == "application/json" );
                BOOST_TEST( v[1] == "text/html" );
                BOOST_TEST( v[2] == "text/plain" );
            }
        }

        // q=0 entries filtered out
        {
            request req( method::get, "/" );
            req.set( field::accept,
                "text/html, text/plain;q=0" );
            accepts ac( req );
            auto v = ac.types();
            BOOST_TEST( v.size() == 1 );
            if( ! v.empty() )
                BOOST_TEST( v[0] == "text/html" );
        }
    }

    void
    testEncoding()
    {
        // No Accept-Encoding returns first offered
        {
            request req( method::get, "/" );
            accepts ac( req );
            BOOST_TEST( ac.encoding({ "gzip", "deflate" })
                == "gzip" );
        }

        // Accept-Encoding: gzip, deflate
        {
            request req( method::get, "/" );
            req.set( field::accept_encoding,
                "gzip, deflate" );
            accepts ac( req );
            BOOST_TEST( ac.encoding({ "gzip", "deflate" })
                == "gzip" );
            BOOST_TEST( ac.encoding({ "deflate" })
                == "deflate" );
            BOOST_TEST( ac.encoding({ "br" }).empty() );
        }

        // Accept-Encoding with quality
        {
            request req( method::get, "/" );
            req.set( field::accept_encoding,
                "gzip;q=0.5, br" );
            accepts ac( req );
            BOOST_TEST( ac.encoding({ "gzip", "br" })
                == "br" );
        }

        // Wildcard
        {
            request req( method::get, "/" );
            req.set( field::accept_encoding, "*" );
            accepts ac( req );
            BOOST_TEST( ac.encoding({ "br" })
                == "br" );
        }
    }

    void
    testEncodings()
    {
        // No header
        {
            request req( method::get, "/" );
            accepts ac( req );
            BOOST_TEST( ac.encodings().empty() );
        }

        // Sorted by quality
        {
            request req( method::get, "/" );
            req.set( field::accept_encoding,
                "deflate;q=0.5, gzip, br;q=0.8" );
            accepts ac( req );
            auto v = ac.encodings();
            BOOST_TEST( v.size() == 3 );
            if( v.size() >= 3 )
            {
                BOOST_TEST( v[0] == "gzip" );
                BOOST_TEST( v[1] == "br" );
                BOOST_TEST( v[2] == "deflate" );
            }
        }
    }

    void
    testCharset()
    {
        // No header returns first offered
        {
            request req( method::get, "/" );
            accepts ac( req );
            BOOST_TEST( ac.charset({ "utf-8", "iso-8859-1" })
                == "utf-8" );
        }

        // Accept-Charset: utf-8, iso-8859-1;q=0.2, utf-7;q=0.5
        {
            request req( method::get, "/" );
            req.set( field::accept_charset,
                "utf-8, iso-8859-1;q=0.2, utf-7;q=0.5" );
            accepts ac( req );
            BOOST_TEST( ac.charset({ "utf-8", "utf-7" })
                == "utf-8" );
            BOOST_TEST( ac.charset({ "utf-7", "iso-8859-1" })
                == "utf-7" );
        }
    }

    void
    testCharsets()
    {
        request req( method::get, "/" );
        req.set( field::accept_charset,
            "utf-8, iso-8859-1;q=0.2, utf-7;q=0.5" );
        accepts ac( req );
        auto v = ac.charsets();
        BOOST_TEST( v.size() == 3 );
        if( v.size() >= 3 )
        {
            BOOST_TEST( v[0] == "utf-8" );
            BOOST_TEST( v[1] == "utf-7" );
            BOOST_TEST( v[2] == "iso-8859-1" );
        }
    }

    void
    testLanguage()
    {
        // No header returns first offered
        {
            request req( method::get, "/" );
            accepts ac( req );
            BOOST_TEST( ac.language({ "en", "fr" })
                == "en" );
        }

        // Accept-Language: en;q=0.8, es, pt
        {
            request req( method::get, "/" );
            req.set( field::accept_language,
                "en;q=0.8, es, pt" );
            accepts ac( req );
            BOOST_TEST( ac.language({ "es", "en" })
                == "es" );
            BOOST_TEST( ac.language({ "en" })
                == "en" );
        }

        // Prefix matching: Accept-Language: en
        // should match en-US
        {
            request req( method::get, "/" );
            req.set( field::accept_language, "en" );
            accepts ac( req );
            BOOST_TEST( ac.language({ "en-US" })
                == "en-US" );
        }

        // Prefix matching: Accept-Language: en-US
        // should match en
        {
            request req( method::get, "/" );
            req.set( field::accept_language, "en-US" );
            accepts ac( req );
            BOOST_TEST( ac.language({ "en" })
                == "en" );
        }

        // Exact match has higher specificity than prefix
        {
            request req( method::get, "/" );
            req.set( field::accept_language, "en" );
            accepts ac( req );
            // "en" matches exactly (s=4), "en-US" only via prefix (s=1)
            BOOST_TEST( ac.language({ "en-US", "en" })
                == "en" );
        }
    }

    void
    testLanguages()
    {
        request req( method::get, "/" );
        req.set( field::accept_language,
            "en;q=0.8, es, pt" );
        accepts ac( req );
        auto v = ac.languages();
        BOOST_TEST( v.size() == 3 );
        if( v.size() >= 3 )
        {
            BOOST_TEST( v[0] == "es" );
            BOOST_TEST( v[1] == "pt" );
            BOOST_TEST( v[2] == "en" );
        }
    }

    void
    run()
    {
        testType();
        testTypes();
        testEncoding();
        testEncodings();
        testCharset();
        testCharsets();
        testLanguage();
        testLanguages();
    }
};

TEST_SUITE(
    accepts_test,
    "boost.http.server.accepts");

} // http
} // boost

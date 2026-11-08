// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

// Small yyjson wrapper

module;

#define yyjson_api_inline yyjson_inline
#include "yyjson.h"

module jacinth:yyjson;

export {
    using ::unsafe_yyjson_equals_strn;
    using ::unsafe_yyjson_get_next;
    using ::unsafe_yyjson_is_str_noesc;
    using ::unsafe_yyjson_set_tag;

    using ::yyjson_arr_get;
    using ::yyjson_arr_get_first;
    using ::yyjson_arr_iter;
    using ::yyjson_arr_iter_init;
    using ::yyjson_arr_iter_next;
    using ::yyjson_arr_size;
    using ::yyjson_doc;
    using ::yyjson_doc_free;
    using ::yyjson_doc_mut_copy;
    using ::yyjson_get_bool;
    using ::yyjson_get_int;
    using ::yyjson_get_len;
    using ::yyjson_get_num;
    using ::yyjson_get_str;
    using ::yyjson_get_uint;
    using ::yyjson_is_arr;
    using ::yyjson_is_bool;
    using ::yyjson_is_int;
    using ::yyjson_is_null;
    using ::yyjson_is_num;
    using ::yyjson_is_obj;
    using ::yyjson_is_real;
    using ::yyjson_is_str;
    using ::yyjson_is_uint;
    using ::yyjson_mut_arr_append;
    using ::yyjson_mut_arr_clear;
    using ::yyjson_mut_arr_get;
    using ::yyjson_mut_arr_iter;
    using ::yyjson_mut_arr_iter_init;
    using ::yyjson_mut_arr_iter_next;
    using ::yyjson_mut_arr_remove;
    using ::yyjson_mut_arr_remove_first;
    using ::yyjson_mut_arr_remove_last;
    using ::yyjson_mut_arr_remove_range;
    using ::yyjson_mut_arr_size;
    using ::yyjson_mut_arr_with_double;
    using ::yyjson_mut_arr_with_float;
    using ::yyjson_mut_arr_with_sint16;
    using ::yyjson_mut_arr_with_sint32;
    using ::yyjson_mut_arr_with_sint64;
    using ::yyjson_mut_arr_with_sint8;
    using ::yyjson_mut_arr_with_uint16;
    using ::yyjson_mut_arr_with_uint32;
    using ::yyjson_mut_arr_with_uint64;
    using ::yyjson_mut_arr_with_uint8;
    using ::yyjson_mut_doc;
    using ::yyjson_mut_doc_free;
    using ::yyjson_mut_doc_imut_copy;
    using ::yyjson_mut_doc_mut_copy;
    using ::yyjson_mut_doc_new;
    using ::yyjson_mut_doc_set_root;
    using ::yyjson_mut_get_len;
    using ::yyjson_mut_get_str;
    using ::yyjson_mut_is_arr;
    using ::yyjson_mut_is_ctn;
    using ::yyjson_mut_is_null;
    using ::yyjson_mut_is_obj;
    using ::yyjson_mut_null;
    using ::yyjson_mut_obj_add;
    using ::yyjson_mut_obj_add_bool;
    using ::yyjson_mut_obj_add_double;
    using ::yyjson_mut_obj_add_float;
    using ::yyjson_mut_obj_add_sint;
    using ::yyjson_mut_obj_add_strn;
    using ::yyjson_mut_obj_add_strncpy;
    using ::yyjson_mut_obj_add_uint;
    using ::yyjson_mut_obj_clear;
    using ::yyjson_mut_obj_getn;
    using ::yyjson_mut_obj_iter;
    using ::yyjson_mut_obj_iter_get_val;
    using ::yyjson_mut_obj_iter_init;
    using ::yyjson_mut_obj_iter_next;
    using ::yyjson_mut_obj_put;
    using ::yyjson_mut_obj_remove_keyn;
    using ::yyjson_mut_obj_size;
    using ::yyjson_mut_set_arr;
    using ::yyjson_mut_set_bool;
    using ::yyjson_mut_set_double;
    using ::yyjson_mut_set_float;
    using ::yyjson_mut_set_int;
    using ::yyjson_mut_set_null;
    using ::yyjson_mut_set_obj;
    using ::yyjson_mut_set_uint;
    using ::yyjson_mut_strcpy;
    using ::yyjson_mut_strn;
    using ::yyjson_mut_strncpy;
    using ::yyjson_mut_val;
    using ::yyjson_mut_val_imut_copy;
    using ::yyjson_mut_val_write_buf;
    using ::yyjson_mut_val_write_opts;
    using ::yyjson_obj_getn;
    using ::yyjson_obj_iter;
    using ::yyjson_obj_iter_get_val;
    using ::yyjson_obj_iter_init;
    using ::yyjson_obj_iter_next;
    using ::yyjson_obj_size;
    using ::yyjson_read;
    using ::yyjson_read_err;
    using ::yyjson_read_flag;
    using ::yyjson_read_opts;
    using ::yyjson_val;
    using ::yyjson_val_write_buf;
    using ::yyjson_val_write_opts;
    using ::yyjson_write_code;
    using ::yyjson_write_err;
    using ::yyjson_write_flag;


    enum class WriteResult : yyjson_write_code {
        /** Success. */
        SUCCESS = 0,

        /** Invalid parameter, such as NULL document. */
        ERROR_INVALID_PARAMETER = 1,

        /** Memory allocation failed. */
        ERROR_MEMORY_ALLOCATION = 2,

        /** Invalid value type in JSON document. */
        ERROR_INVALID_VALUE_TYPE = 3,

        /** NaN or Infinity number occurs. */
        ERROR_NAN_OR_INF = 4,

        /** Failed to open a file. */
        ERROR_FILE_OPEN = 5,

        /** Failed to write a file. */
        ERROR_FILE_WRITE = 6,

        /** Invalid unicode in string. */
        ERROR_INVALID_STRING = 7,

        /** Nesting depth limit exceeded. */
        ERROR_DEPTH = 8,
    };

    enum class WriteFlag : yyjson_write_flag {
        /** Default option:
            - Write JSON minify.
            - Report error on inf or nan number.
            - Report error on invalid UTF-8 string.
            - Do not escape unicode or slash. */
        NOFLAG = 0,

        /** Write JSON pretty with 4 space indent. */
        PRETTY = 1 << 0,

        /** Escape unicode as `uXXXX`, make the output ASCII only. */
        ESCAPE_UNICODE = 1 << 1,

        /** Escape '/' as '\/'. */
        ESCAPE_SLASHES = 1 << 2,

        /** Write inf and nan number as 'Infinity' and 'NaN' literal (non-standard). */
        ALLOW_INF_AND_NAN = 1 << 3,

        /** Write inf and nan number as null literal.
            This flag will override `ALLOW_INF_AND_NAN` flag. */
        INF_AND_NAN_AS_NULL = 1 << 4,

        /** Allow invalid unicode when encoding string values (non-standard).
            Invalid characters in string value will be copied byte by byte.
            If `ESCAPE_UNICODE` flag is also set, invalid character will be
            escaped as `U+FFFD` (replacement character).
            This flag does not affect the performance of correctly encoded strings. */
        ALLOW_INVALID_UNICODE = 1 << 5,

        /** Write JSON pretty with 2 space indent.
            This flag will override `PRETTY` flag. */
        PRETTY_TWO_SPACES = 1 << 6,

        /** Adds a newline character `\n` at the end of the JSON.
            This can be helpful for text editors or NDJSON. */
        NEWLINE_AT_END = 1 << 7,

        /** Use lowercase hex digits in `\uXXXX` escape sequences instead of the default
            uppercase. Only effective when `ESCAPE_UNICODE` is also set. */
        LOWERCASE_HEX = 1 << 8,
    };

    namespace yyjson
    {
    inline constexpr uint8_t TYPE_STR = uint8_t(5);
    inline constexpr uint8_t SUBTYPE_NOESC = uint8_t((1 << 3));
    }; // namespace yyjson
};
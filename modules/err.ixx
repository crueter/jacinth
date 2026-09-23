// std::error_code, enum, etc. for error handling
module;

#include <system_error>

export module jacinth:err;

export {

    namespace jacinth
    {
    enum class errc {
        ok,
        invalid_json,
        type_mismatch,
        missing_value,
        write_error,
    };

    // std::error_code
    namespace detail
    {

    class error : public std::error_category {
        const char *name() const noexcept override
        {
            return "jacinth";
        }

        std::string message(int c) const override
        {
            switch (errc(c)) {
            [[unlikely]] case errc::ok:
                return "jacinth: OK";
            case errc::invalid_json:
                return "jacinth: JSON text failed to parse";
            case errc::type_mismatch:
                return "jacinth: type mismatch while parsing value";
            case errc::missing_value:
                return "jacinth: required value is missing";
            case errc::write_error:
                return "jacinth: JSON write error";
            }
            return "jacinth: unknown error";
        }
    };

    inline const std::error_category &jacinth_category() noexcept
    {
        static const detail::error cat;
        return cat;
    }

    } // namespace detail

    inline std::error_code make_error_code(errc e) noexcept
    {
        return {int(e), detail::jacinth_category()};
    }

    } // namespace jacinth

    template <>
    struct std::is_error_code_enum<jacinth::errc> : std::true_type {};
}
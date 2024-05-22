#include <cstring>
#include <cstdlib>

#include "config.hpp"

namespace fz {

namespace {

// The fz::Config::ini_handler function is compiled separately
//   so that exception handling code doesn't get pulled in the object file
// Likewise, we implement minimal versions of atoi, atof and std::string_view::substr
//   to avoid pulling in needless bloat in the sysmodule executable (vfiprintf, exceptions, etc)

constexpr std::string_view substr(std::string_view s, std::size_t pos, std::size_t n = std::string_view::npos) {
    return std::string_view(s.data() + pos, std::min(n, s.size() - pos));
}

constexpr int ctoi(char c) {
    auto res = c - '0';
    return (res >= 0 && res <= 9) ? res : 0;
}

constexpr int atoi(std::string_view s) {
    int mult = 1, res = 0;
    switch (s.front()) {
        case '-':
            mult = -1;
        case '+':
            s = substr(s, 1);
            break;
    }

    for (auto c: s)
        res = res * 10 + ctoi(c);

    return mult * res;
}

static_assert(atoi("0") == 0);
static_assert(atoi("+10") == 10);
static_assert(atoi("-0099") == -99);

constexpr double atof(std::string_view s) {
    double mult = 1, res = 0, decimal = 1;
    switch (s.front()) {
        case '-':
            mult = -1;
        case '+':
            s = substr(s, 1);
            break;
    }

    for (auto c: s) {
        if (c == '.')
            decimal = 10;
        else {
            res = res * (decimal > 1 ? 1 : 10) + ctoi(c) / decimal;
            if (decimal > 1)
                decimal *= 10;
        }
    }

    return mult * res;
}

static_assert(atof("0") == 0.0);
static_assert(atof("2.4") == 2.4);
static_assert(atof("+011.11") == 11.11);
static_assert(atof("-00333.444") == -333.444);

} // namespace

#define MATCH(s1, s2)     (std::strcmp(s1, s2) == 0)
#define MATCH_ENTRY(s, n) (MATCH(section, (s)) && MATCH(name, (n)))

int Config::ini_handler(void *user, const char *section, const char *name, const char *value) {
    Config *config = static_cast<Config *>(user);

    auto profile_name_to_id = [](const std::string_view &str) -> FizeauProfileId {
        return static_cast<FizeauProfileId>(str.back() - '0' - 1);
    };

    auto parse_filter = [](const std::string_view &str) -> ColorFilter {
        if (strcasecmp(str.data(), "red") == 0)
            return ColorFilter_Red;
        else if (strcasecmp(str.data(), "green") == 0)
            return ColorFilter_Green;
        else if (strcasecmp(str.data(), "blue") == 0)
            return ColorFilter_Blue;
        return ColorFilter_None;
    };

    auto parse_time = [](const std::string_view &str) -> Time {
        Time t = {};
        auto pos = str.find(':');
        t.h = atoi(substr(str, 0, pos));
        t.m = atoi(substr(str, pos + 1));
        return t;
    };

    static_assert(parse_time("09:02") == Time{9, 2});

    auto parse_range = [](const std::string_view &str) -> ColorRange {
        ColorRange r = {};
        auto pos = str.find('-');
        r.lo = atof(substr(str, 0, pos));
        r.hi = atof(substr(str, pos + 1));
        return r;
    };

    static_assert(parse_range("0.18-0.92") == ColorRange{0.18, 0.92});

    if (MATCH_ENTRY("", "active")) {
        if (MATCH(value, "1") || strcasecmp(value, "true") == 0)
            config->active = true;
        else
            config->active = false;
        config->has_active_override = true;
    } else if (MATCH_ENTRY("", "handheld_profile")) {
        config->internal_profile = profile_name_to_id(value);
    } else if (MATCH_ENTRY("", "docked_profile")) {
        config->external_profile = profile_name_to_id(value);
    } else if (std::strcmp(section, "profile") > 0) {
        auto id = profile_name_to_id(section);
        if (config->cur_profile_id != id && config->parse_profile_switch_action) {
            config->parse_profile_switch_action(config, id);
            config->cur_profile_id = id;
        }

        if (MATCH(name, "dusk_begin"))
            config->profile.dusk_begin                 = parse_time(value);
        else if (MATCH(name, "dusk_end"))
            config->profile.dusk_end                   = parse_time(value);
        else if (MATCH(name, "dawn_begin"))
            config->profile.dawn_begin                 = parse_time(value);
        else if (MATCH(name, "dawn_end"))
            config->profile.dawn_end                   = parse_time(value);
        else if (MATCH(name, "temperature_day"))
            config->profile.day_settings.temperature   = atoi(value);
        else if (MATCH(name, "temperature_night"))
            config->profile.night_settings.temperature = atoi(value);
        else if (MATCH(name, "filter_day"))
            config->profile.day_settings.filter        = parse_filter(value);
        else if (MATCH(name, "filter_night"))
            config->profile.night_settings.filter      = parse_filter(value);
        else if (MATCH(name, "gamma_day"))
            config->profile.day_settings.gamma         = atof(value);
        else if (MATCH(name, "gamma_night"))
            config->profile.night_settings.gamma       = atof(value);
        else if (MATCH(name, "saturation_day"))
            config->profile.day_settings.saturation    = atof(value);
        else if (MATCH(name, "saturation_night"))
            config->profile.night_settings.saturation  = atof(value);
        else if (MATCH(name, "luminance_day"))
            config->profile.day_settings.luminance     = atof(value);
        else if (MATCH(name, "luminance_night"))
            config->profile.night_settings.luminance   = atof(value);
        else if (MATCH(name, "range_day"))
            config->profile.day_settings.range         = parse_range(value);
        else if (MATCH(name, "range_night"))
            config->profile.night_settings.range       = parse_range(value);
        else if (MATCH(name, "dimming_timeout")) {
            auto t = parse_time(value);
            config->profile.dimming_timeout            = { 0, t.h, t.m };
        }
    } else {
        return 0;
    }

    return 1;
}

} // namespace fz

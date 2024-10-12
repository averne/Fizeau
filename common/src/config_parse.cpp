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
    std::string_view v = value;

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
        config->internal_profile = profile_name_to_id(v);
    } else if (MATCH_ENTRY("", "docked_profile")) {
        config->external_profile = profile_name_to_id(v);
    } else if (std::strcmp(section, "profile") > 0) {
        auto id = profile_name_to_id(section);
        if (config->cur_profile_id != id && config->parse_profile_switch_action) {
            config->parse_profile_switch_action(config, id);
            config->cur_profile_id = id;
        }

        void *target = nullptr;
        #define MATCH_SET(s1, s2, t) (target = &t, MATCH(s1, s2))
        #define SET(v) *reinterpret_cast<decltype(v) *>(target) = v

        auto &p = config->profile;
        if (
            MATCH_SET(name, "dusk_begin", p.dusk_begin) ||
            MATCH_SET(name, "dusk_end",   p.dusk_end)   ||
            MATCH_SET(name, "dawn_begin", p.dawn_begin) ||
            MATCH_SET(name, "dawn_end",   p.dawn_end)
        ) {
            SET(parse_time(v));
        } else if (
            MATCH_SET(name, "temperature_day",   p.day_settings  .temperature) ||
            MATCH_SET(name, "temperature_night", p.night_settings.temperature)
        ) {
            SET(atoi(v));
        } else if (
            MATCH_SET(name, "filter_day",   p.day_settings  .filter) ||
            MATCH_SET(name, "filter_night", p.night_settings.filter)
        ) {
            SET(parse_filter(v));
        } else if (
            MATCH_SET(name, "gamma_day",        p.day_settings  .gamma)      ||
            MATCH_SET(name, "gamma_night",      p.night_settings.gamma)      ||
            MATCH_SET(name, "saturation_day",   p.day_settings  .saturation) ||
            MATCH_SET(name, "saturation_night", p.night_settings.saturation) ||
            MATCH_SET(name, "hue_day",          p.day_settings  .hue)        ||
            MATCH_SET(name, "hue_night",        p.night_settings.hue)        ||
            MATCH_SET(name, "luminance_day",    p.day_settings  .luminance)  ||
            MATCH_SET(name, "luminance_night",  p.night_settings.luminance)
        ) {
            float f = atof(v);
            SET(f);
        } else if (
            MATCH_SET(name, "range_day",   p.day_settings  .range) ||
            MATCH_SET(name, "range_night", p.night_settings.range)
        ) {
            SET(parse_range(v));
        } else if (MATCH(name, "dimming_timeout")) {
            auto t = parse_time(v);
            config->profile.dimming_timeout = { 0, t.h, t.m };
        }
    } else {
        return 0;
    }

    return 1;
}

} // namespace fz

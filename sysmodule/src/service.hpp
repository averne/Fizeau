// Copyright (C) 2020 averne
//
// This file is part of Fizeau.
//
// Fizeau is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Fizeau is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Fizeau.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <memory>
#include <stratosphere.hpp>
#include <common.hpp>

#include "profile.hpp"

namespace fz {

class IProfile final: public ams::sf::IServiceObject {
    public:
        enum class CommandId {
            GetDuskTime         = 0,
            SetDuskTime         = 1,
            GetDawnTime         = 2,
            SetDawnTime         = 3,
            GetCmuTemperature   = 4,
            SetCmuTemperature   = 5,
            GetCmuColorFilter   = 6,
            SetCmuColorFilter   = 7,
            GetCmuGamma         = 8,
            SetCmuGamma         = 9,
            GetCmuLuminance     = 10,
            SetCmuLuminance     = 11,
            GetCmuColorRange    = 12,
            SetCmuColorRange    = 13,
            GetScreenBrightness = 14,
            SetScreenBrightness = 15,
            GetDimmingTimeout   = 16,
            SetDimmingTimeout   = 17,
        };

    public:
        IProfile(ProfileId id, Profile &profile): id(id), profile(profile) { }

    protected:
        ams::Result GetDuskTime(ams::sf::Out<Time> time_begin, ams::sf::Out<Time> time_end);
        ams::Result SetDuskTime(Time time_begin, Time time_end);
        ams::Result GetDawnTime(ams::sf::Out<Time> time_begin, ams::sf::Out<Time> time_end);
        ams::Result SetDawnTime(Time time_begin, Time time_end);

        ams::Result GetCmuTemperature(ams::sf::Out<Temperature> temp_day, ams::sf::Out<Temperature> temp_night);
        ams::Result SetCmuTemperature(Temperature temp_day, Temperature temp_night);
        ams::Result GetCmuColorFilter(ams::sf::Out<ColorFilter> filter_day, ams::sf::Out<ColorFilter> filter_night);
        ams::Result SetCmuColorFilter(ColorFilter filter_day, ColorFilter filter_night);
        ams::Result GetCmuGamma(ams::sf::Out<Gamma> gamma_day, ams::sf::Out<Gamma> gamma_night);
        ams::Result SetCmuGamma(Gamma gamma_day, Gamma gamma_night);
        ams::Result GetCmuLuminance(ams::sf::Out<Luminance> luma_day, ams::sf::Out<Luminance> luma_night);
        ams::Result SetCmuLuminance(Luminance luma_day, Luminance luma_night);
        ams::Result GetCmuColorRange(ams::sf::Out<ColorRange> range_day, ams::sf::Out<ColorRange> range_night);
        ams::Result SetCmuColorRange(ColorRange range_day, ColorRange range_night);

        ams::Result GetScreenBrightness(ams::sf::Out<Brightness> brightness_day, ams::sf::Out<Brightness> brightness_night);
        ams::Result SetScreenBrightness(Brightness brightness_day, Brightness brightness_night);

        ams::Result GetDimmingTimeout(ams::sf::Out<Time> timeout);
        ams::Result SetDimmingTimeout(Time timeout);

    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MAKE_SERVICE_COMMAND_META(GetDuskTime),
            MAKE_SERVICE_COMMAND_META(SetDuskTime),
            MAKE_SERVICE_COMMAND_META(GetDawnTime),
            MAKE_SERVICE_COMMAND_META(SetDawnTime),
            MAKE_SERVICE_COMMAND_META(GetCmuTemperature),
            MAKE_SERVICE_COMMAND_META(SetCmuTemperature),
            MAKE_SERVICE_COMMAND_META(GetCmuColorFilter),
            MAKE_SERVICE_COMMAND_META(SetCmuColorFilter),
            MAKE_SERVICE_COMMAND_META(GetCmuGamma),
            MAKE_SERVICE_COMMAND_META(SetCmuGamma),
            MAKE_SERVICE_COMMAND_META(GetCmuLuminance),
            MAKE_SERVICE_COMMAND_META(SetCmuLuminance),
            MAKE_SERVICE_COMMAND_META(GetCmuColorRange),
            MAKE_SERVICE_COMMAND_META(SetCmuColorRange),
            MAKE_SERVICE_COMMAND_META(GetScreenBrightness),
            MAKE_SERVICE_COMMAND_META(SetScreenBrightness),
            MAKE_SERVICE_COMMAND_META(GetDimmingTimeout),
            MAKE_SERVICE_COMMAND_META(SetDimmingTimeout),
        };

    private:
        ProfileId id;
        Profile &profile;
};

class FizeauService final: public ams::sf::IServiceObject {
    public:
        enum class CommandId {
            GetIsActive                = 0,
            SetIsActive                = 1,
            OpenProfile                = 2,
            GetActiveInternalProfileId = 3,
            SetActiveInternalProfileId = 4,
            GetActiveExternalProfileId = 5,
            SetActiveExternalProfileId = 6,
        };

    protected:
        ams::Result GetIsActive(ams::sf::Out<bool> is_active);
        ams::Result SetIsActive(bool is_active);

        ams::Result OpenProfile(ams::sf::Out<std::shared_ptr<IProfile>> out, ProfileId profile);

        ams::Result GetActiveInternalProfileId(ams::sf::Out<ProfileId> id);
        ams::Result SetActiveInternalProfileId(ProfileId id);
        ams::Result GetActiveExternalProfileId(ams::sf::Out<ProfileId> id);
        ams::Result SetActiveExternalProfileId(ProfileId id);

    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MAKE_SERVICE_COMMAND_META(GetIsActive),
            MAKE_SERVICE_COMMAND_META(SetIsActive),
            MAKE_SERVICE_COMMAND_META(OpenProfile),
            MAKE_SERVICE_COMMAND_META(GetActiveInternalProfileId),
            MAKE_SERVICE_COMMAND_META(SetActiveInternalProfileId),
            MAKE_SERVICE_COMMAND_META(GetActiveExternalProfileId),
            MAKE_SERVICE_COMMAND_META(SetActiveExternalProfileId),
        };
};

} // namespace fz


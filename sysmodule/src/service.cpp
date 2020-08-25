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

#include <stratosphere.hpp>
#include <common.hpp>

#include "service.hpp"

namespace fz {

ams::Result FizeauService::GetIsActive(ams::sf::Out<bool> is_active) {
    is_active.SetValue(ProfileManager::get_is_active());
    return ams::ResultSuccess();
}

ams::Result FizeauService::SetIsActive(bool is_active) {
    return ProfileManager::set_is_active(is_active);
}

ams::Result FizeauService::OpenProfile(ams::sf::Out<std::shared_ptr<IProfile>> out, ProfileId profile) {
    out.SetValue(std::make_shared<IProfile>(profile, ProfileManager::get_profile(profile)));
    return ams::ResultSuccess();
}

ams::Result FizeauService::GetActiveInternalProfileId(ams::sf::Out<ProfileId> id) {
    id.SetValue(ProfileManager::get_active_internal_profile_id());
    return ams::ResultSuccess();
}

ams::Result FizeauService::SetActiveInternalProfileId(ProfileId id) {
    ProfileManager::set_active_internal_profile_id(id);
    return ams::ResultSuccess();
}

ams::Result FizeauService::GetActiveExternalProfileId(ams::sf::Out<ProfileId> id) {
    id.SetValue(ProfileManager::get_active_external_profile_id());
    return ams::ResultSuccess();
}

ams::Result FizeauService::SetActiveExternalProfileId(ProfileId id) {
    ProfileManager::set_active_external_profile_id(id);
    return ams::ResultSuccess();
}

ams::Result IProfile::GetDuskTime(ams::sf::Out<Time> time_begin, ams::sf::Out<Time> time_end) {
    time_begin.SetValue(this->profile.dusk_begin), time_end.SetValue(this->profile.dusk_end);
    return ams::ResultSuccess();
}

ams::Result IProfile::SetDuskTime(Time time_begin, Time time_end) {
    this->profile.dusk_begin = time_begin, this->profile.dusk_end = time_end;
    return ProfileManager::on_profile_updated(this->id);
}

ams::Result IProfile::GetDawnTime(ams::sf::Out<Time> time_begin, ams::sf::Out<Time> time_end) {
    time_begin.SetValue(this->profile.dawn_begin), time_end.SetValue(this->profile.dawn_end);
    return ams::ResultSuccess();
}

ams::Result IProfile::SetDawnTime(Time time_begin, Time time_end) {
    this->profile.dawn_begin = time_begin, this->profile.dawn_end = time_end;
    return ProfileManager::on_profile_updated(this->id);
}

ams::Result IProfile::GetCmuTemperature(ams::sf::Out<Temperature> temp_day, ams::sf::Out<Temperature> temp_night) {
    temp_day.SetValue(this->profile.temperature_day), temp_night.SetValue(this->profile.temperature_night);
    return ams::ResultSuccess();
}

ams::Result IProfile::SetCmuTemperature(Temperature temp_day, Temperature temp_night) {
    this->profile.temperature_day = temp_day, this->profile.temperature_night = temp_night;
    return ProfileManager::on_profile_updated(this->id);
}

ams::Result IProfile::GetCmuColorFilter(ams::sf::Out<ColorFilter> filter_day, ams::sf::Out<ColorFilter> filter_night) {
    filter_day.SetValue(this->profile.filter_day), filter_night.SetValue(this->profile.filter_night);
    return ams::ResultSuccess();
}

ams::Result IProfile::SetCmuColorFilter(ColorFilter filter_day, ColorFilter filter_night) {
    this->profile.filter_day = filter_day, this->profile.filter_night = filter_night;
    return ams::ResultSuccess();
}

ams::Result IProfile::GetCmuGamma(ams::sf::Out<Gamma> gamma_day, ams::sf::Out<Gamma> gamma_night) {
    gamma_day.SetValue(this->profile.gamma_day), gamma_night.SetValue(this->profile.gamma_night);
    return ams::ResultSuccess();
}

ams::Result IProfile::SetCmuGamma(Gamma gamma_day, Gamma gamma_night) {
    this->profile.gamma_day = gamma_day, this->profile.gamma_night = gamma_night;
    return ProfileManager::on_profile_updated(this->id);
}

ams::Result IProfile::GetCmuLuminance(ams::sf::Out<Luminance> luminance_day, ams::sf::Out<Luminance> luminance_night) {
    luminance_day.SetValue(this->profile.luminance_day), luminance_night.SetValue(this->profile.luminance_night);
    return ams::ResultSuccess();
}

ams::Result IProfile::SetCmuLuminance(Luminance luminance_day, Luminance luminance_night) {
    this->profile.luminance_day = luminance_day, this->profile.luminance_night = luminance_night;
    return ProfileManager::on_profile_updated(this->id);
}

ams::Result IProfile::GetCmuColorRange(ams::sf::Out<ColorRange> range_day, ams::sf::Out<ColorRange> range_night) {
    range_day.SetValue(this->profile.range_day), range_night.SetValue(this->profile.range_night);
    return ams::ResultSuccess();
}

ams::Result IProfile::SetCmuColorRange(ColorRange range_day, ColorRange range_night) {
    this->profile.range_day = range_day, this->profile.range_night = range_night;
    return ProfileManager::on_profile_updated(this->id);
}

ams::Result IProfile::GetScreenBrightness(ams::sf::Out<Brightness> brightness_day, ams::sf::Out<Brightness> brightness_night) {
    brightness_day.SetValue(this->profile.brightness_day), brightness_night.SetValue(this->profile.brightness_night);
    return ams::ResultSuccess();
}

ams::Result IProfile::SetScreenBrightness(Brightness brightness_day, Brightness brightness_night) {
    this->profile.brightness_day = brightness_day, this->profile.brightness_night = brightness_night;
    return ProfileManager::on_profile_updated(this->id);
}

} // namespace fz

/*  =========================================================================
    range_scan - Perform one range scan

    Copyright (C) 2014 - 2017 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

#pragma once
#include "wrappers/actor.h"
#include <fty_common_nut_types.h>

namespace fty {
class DiscoveredDevices;
}

namespace fty::scan {

/// Perform one range scan
class RangeScan : public Actor<RangeScan>
{
public:
    using Range  = std::pair<std::string, std::string>;
    using Ranges = std::vector<Range>;

public:
    void runWorker(const Ranges& ranges, const DiscoveredDevices& devices, const nut::KeyValues& nutMapping);
};

} // namespace fty::scan

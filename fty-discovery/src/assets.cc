/*  =========================================================================
    assets - Cache of assets

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

#include "assets.h"
#include <fty_proto.h>

void Assets::put(FtyProto&& proto)
{
    if (!proto) {
        return;
    }

    std::string operation = proto.operation();
    std::string iname     = proto.name();

    if (operation.empty() || iname.empty()) {
        // malformed message
        return;
    }

    if (operation == "create" || operation == "update") {
        // create, update
        if (m_map.find(iname) == m_map.end()) {
            // new for us
            m_lastUpdate = zclock_mono();
        }
        m_map[iname] = std::move(proto);
    } else if (operation == "delete") {
        // delete
        m_map.erase(iname);
        m_lastUpdate = zclock_mono();
        return;
    }
}

static bool hasAttribute(const FtyProto& proto, const std::string& key, const std::string& value)
{
    // try whether exact key exists
    std::string avalue = proto.extString(key);

    if (!avalue.empty()) {
        return avalue == value;
    }

    // try indexed key
    for (int i = 1;; ++i) {
        avalue = proto.extString(key + "." + std::to_string(i));

        if (avalue.empty()) {
            return false;
        }

        if (avalue == value) {
            return true;
        }
    }
}

const FtyProto* Assets::find(const std::string& key, const std::string& value) const
{
    if (key.empty() || value.empty()) {
        return nullptr;
    }

    auto found = std::find_if(m_map.begin(), m_map.end(), [&](const auto& pair) {
        return hasAttribute(pair.second, key, value);
    });

    if (found != m_map.end()) {
        return &(found->second);
    }

    return nullptr;
}

int64_t Assets::lastChange() const
{
    return m_lastUpdate;
}

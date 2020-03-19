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

#pragma once
#include "wrappers/ftyproto.h"
#include <map>
#include <string>

typedef struct _fty_proto_t fty_proto_t;

/// Cache of assets
class Assets
{
public:
    /// Put one asset into cache
    void put(FtyProto&& msg);

    /// Find asset by ext attribute
    const FtyProto* find(const std::string& key, const std::string& value) const;

    /// return the zclock_mono time in ms when last change happened (create or delete, not update)
    int64_t lastChange() const;

private:
    int64_t                         m_lastUpdate = 0;
    std::map<std::string, FtyProto> m_map;
};

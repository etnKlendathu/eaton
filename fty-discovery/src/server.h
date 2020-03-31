/*  =========================================================================
    ftydiscovery - Manages discovery requests, provides feedback

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
#include <memory>

class ZMessage;

/// Manages discovery requests, provides feedback
class Discovery
{
public:
    static constexpr const char* Endpoint  = "ipc://@/malamute";
    static constexpr const char* ActorName = "fty-discovery";
    static constexpr const char* CfgFile   = "/etc/fty-discovery/fty-discovery.cfg";

public:
    Discovery();
    ~Discovery();

    void run();

    void write(ZMessage&& msg);
    ZMessage read() const;
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};


/*  =========================================================================
    fty_discovery_server - Manages discovery requests, provides feedback

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

#include "server.h"
#include "discovery/discovery-impl.h"

Discovery::Discovery()
    : m_impl(new Impl)
{
}

Discovery::~Discovery()
{
}

void Discovery::run()
{
    m_impl->run();
}

void Discovery::write(ZMessage&& msg)
{
    m_impl->send(std::move(msg));
}

ZMessage Discovery::read() const
{
    return m_impl->read();
}


// ===========================================================================================================



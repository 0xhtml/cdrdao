/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 2007  Denis Leroy
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __CONFIG_MANAGER_H
#define __CONFIG_MANAGER_H

#include <glibmm.h>
#include <giomm.h>

class ConfigManager
{
 public:
    ConfigManager();
    virtual ~ConfigManager() {}

    void set(const Glib::ustring key, const Glib::ustring value);
    void set(const Glib::ustring key, const bool value);
    void set(const Glib::ustring key, Glib::StringArrayHandle array);

    Glib::ustring get_string(const Glib::ustring key);
    Glib::StringArrayHandle get_string_array(const Glib::ustring key);
    bool get_bool(const Glib::ustring key);

 protected:
    Glib::RefPtr<Gio::Settings> client_;
};

#endif


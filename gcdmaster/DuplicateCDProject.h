/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 2000  Andreas Mueller <mueller@daneb.ping.de>
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

#ifndef __DUPLICATECDPROJECT_H__
#define __DUPLICATECDPROJECT_H__

#include <gtkmm.h>

class RecordCDSource;
class RecordCDTarget;

class DuplicateCDProject : public Project
{
public:
    DuplicateCDProject(Gtk::ApplicationWindow* parent);
    virtual ~DuplicateCDProject();

    static DuplicateCDProject* create(Glib::RefPtr<Gtk::Builder>& builder,
                                      Gtk::ApplicationWindow* parent);
//    bool closeProject();
/* protected: */
/*     virtual void createToolbar() {}; */

private:
    RecordCDSource *CDSource;
    RecordCDTarget *CDTarget;

    Gtk::Label label_;
    Gtk::RadioButton *simulate_rb;
    Gtk::RadioButton *simulateBurn_rb;
    Gtk::RadioButton *burn_rb;

    void start();
    /* void recordToc2CD() {} */
    /* void projectInfo() {} */
    /* void update(unsigned long level); */
};
#endif

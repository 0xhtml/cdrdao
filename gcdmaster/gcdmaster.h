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

#ifndef __GCDMASTER_H__
#define __GCDMASTER_H__

#include <gtkmm.h>

#include <list>

#include "Project.h"
#include "BlankCDWindow.h"
#include "ProjectChooser.h"
#include "PreferencesDialog.h"

/*
 * Application Window class
 */

class GCDWindow : public Gtk::ApplicationWindow
{
public:
    GCDWindow(BaseObjectType* cobject,
              const Glib::RefPtr<Gtk::Builder>& builder);

    enum class What { CHOOSER, AUDIOCD, DUPLICATE, BLANKCD, DUMP };

    static GCDWindow* create(Glib::RefPtr<Gtk::Builder> b, What what,
                             const char*, TocEdit*);

    Project* project() { return project_; }

    void update(unsigned long level);
    void set_menu(Glib::RefPtr<Gio::MenuModel> model);

protected:
    void set_project(Project* project);

private:
    Project* project_;
    Gtk::Notebook* notebook_;
    Gtk::MenuButton* gears_;
};

/* 
 * Main GCDMaster application
 */

class GCDMaster : public Gtk::Application
{
public:
  GCDMaster();

  void newChooserWindow();
  void newDuplicateCDProject();
  void newDumpCDProject();
  void newEmptyAudioCDProject();
  void newAudioCDProject(const char* name = NULL, TocEdit* tocEdit = NULL);

  void update(unsigned long level);

private:
  gint project_number_;
  Glib::RefPtr<Gtk::Builder> builder_;

  // Override default signal handlers.
  void on_startup() override;
  void on_activate() override;
  void on_action_quit();
  void on_action_preferences();
  void on_action_about();
  void on_action_blank_cdrw();
  void on_action_open();
  void on_open(const type_vec_files& files, const Glib::ustring& hint) override;

//  bool closeProject();
//  void closeChooser();
//  bool on_delete_event(GdkEventAny* e);
  bool openNewProject(const char*);
//void openProject();
  /* void newDumpCDProject(); */

  /* void configureDevices(); */
  /* void configurePreferences(); */

  /* void registerStockIcons(); */

  /* static void appClose(); */

//  Project* project_;

  // Dialogs
  Glib::RefPtr<PreferencesDialog> preferencesDialog_;
  Glib::RefPtr<Gtk::AboutDialog> aboutDialog_;

  // Windows
  BlankCDWindow* blankCDWindow_;

  Gtk::FileChooserDialog m_open_file_chooser;
  Glib::RefPtr<Gtk::FileFilter> open_filter_;
  Glib::RefPtr<Gtk::FileFilter> all_filter_;

  int argc_;
  char** argv_;

  /* Gtk::Notebook notebook_; */

  /* Glib::RefPtr<Gtk::UIManager> m_refUIManager; */
  /* Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup; */

  /* //Gnome::UI::AppBar* statusbar_;   */
  /* Gtk::ProgressBar* progressbar_;   */
  /* Gtk::Button* progressButton_;   */

  /* Gtk::FileChooserDialog* readFileSelector_; */
  /* void createMenus(); */
  /* void createStatusbar(); */
};

#endif

/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2000  Andreas Mueller <mueller@daneb.ping.de>
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

#include <gtkmm.h>
#include <glibmm/i18n.h>

#include "AudioCDProject.h"
#include "Toc.h"
#include "SoundIF.h"
#include "AudioCDView.h"
#include "TocEdit.h"
#include "TocEditView.h"
#include "TocInfoDialog.h"
#include "CdTextDialog.h"
#include "guiUpdate.h"
#include "util.h"
#include "gcdmaster.h"
#include "xcdrdao.h"
#include "RecordTocDialog.h"
#include "MessageBox.h"

AudioCDProject*
AudioCDProject::create(Glib::RefPtr<Gtk::Builder>& builder, int number,
                       const char* name, TocEdit* tocEdit, GCDWindow* parent)
{
    AudioCDProject* project = new AudioCDProject(builder,
                                                 number, name, tocEdit, parent);
    return project;
}

AudioCDProject::~AudioCDProject()
{
}

AudioCDProject::AudioCDProject(Glib::RefPtr<Gtk::Builder>& builder,
                               int number, const char *name, TocEdit *tocEdit,
                               GCDWindow *parent) : Project(parent)
{
    tocInfoDialog_ = NULL;
    cdTextDialog_ = NULL;
    soundInterface_ = NULL;
    buttonPlay_ = NULL;
    buttonStop_ = NULL;
    buttonPause_ = NULL;
    audioCDView_ = NULL;

    parent_ = parent;
    pack_start(vbox_);

    projectNumber_ = number;

    playStatus_ = STOPPED;
    playBurst_ = 588 * 10;
    playBuffer_ = new Sample[playBurst_];

    if (tocEdit == NULL)
        tocEdit_ = new TocEdit(NULL, NULL);
    else
        tocEdit_ = tocEdit;

    // Connect TocEdit signals to us.
    tocEdit_->signalStatusMessage.
        connect(sigc::mem_fun(*this, &AudioCDProject::status));
    tocEdit_->signalProgressFraction.
        connect(sigc::mem_fun(*this, &AudioCDProject::progress));
    tocEdit_->signalFullView.
        connect(sigc::mem_fun(*this, &AudioCDProject::fullView));
    tocEdit_->signalSampleSelection.
        connect(sigc::mem_fun(*this, &AudioCDProject::sampleSelect));
    tocEdit_->signalCancelEnable.
        connect(sigc::mem_fun(*this, &AudioCDProject::cancelEnable));
    tocEdit_->signalError.
        connect(sigc::mem_fun(*this, &AudioCDProject::errorDialog));

    if (tocEdit_->isQueueActive())
        cancelEnable(true);

    if (!name || strlen(name) == 0) {
        char buf[20];
        sprintf(buf, "unnamed-%i.toc", projectNumber_);
        tocEdit_->filename(buf);
        new_ = true;
    } else {
        new_ = false; // The project file already exists
    }

    // Load toolbar.
    builder->add_from_resource("/org/gnome/gcdmaster/audiocd.ui");
    Gtk::Toolbar* tbar = NULL;
    builder->get_widget("audio-toolbar", tbar);
    vbox_.pack_start(*tbar, Gtk::PACK_SHRINK);

    add_actions();

    // Load menu.
    builder->add_from_resource("/org/gnome/gcdmaster/gears_audio_menu.ui");
    auto object = builder->get_object("audiocd-menu");
    auto menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(object);
    if (!menu)
        throw std::runtime_error("Menu resource not found");

    parent_->set_menu(menu);

    // Create Viewer.
    audioCDView_ = new AudioCDView(this, m_action_group);
    vbox_.pack_start(*audioCDView_);
    audioCDView_->tocEditView()->sampleViewFull();

    updateWindowTitle();

    guiUpdate(UPD_ALL);
    show_all();
}

void AudioCDProject::add_actions()
{
    m_action_group = Gio::SimpleActionGroup::create();
    m_action_group->add_action("zoom-in",
                               sigc::mem_fun(*this, &AudioCDProject::on_zoom_in_clicked));
    m_action_group->add_action("zoom-out",
                               sigc::mem_fun(*this, &AudioCDProject::on_zoom_out_clicked));
    m_action_group->add_action("save",
                               sigc::mem_fun(*this, &Project::saveProject));
    m_action_group->add_action("save-as",
                               sigc::mem_fun(*this, &Project::saveAsProject));
    m_action_group->add_action("project-info",
                               sigc::mem_fun(*this, &AudioCDProject::projectInfo));
    m_action_group->add_action("cd-text",
                               sigc::mem_fun(*this, &AudioCDProject::cdTextDialog));
    m_action_group->add_action("record",
                               sigc::mem_fun(*this, &AudioCDProject::recordToc2CD));
    m_action_group->add_action("play",
                               sigc::mem_fun(*this, &AudioCDProject::on_play_clicked));
    m_action_group->add_action("stop",
                               sigc::mem_fun(*this, &AudioCDProject::on_stop_clicked));
    m_action_group->add_action("pause",
                               sigc::mem_fun(*this, &AudioCDProject::on_pause_clicked));
    m_action_group->add_action("zoom-fit",
                               sigc::mem_fun(*this, &AudioCDProject::on_zoom_fit_clicked));
    m_action_group->add_action("zoom-mode",
                               sigc::mem_fun(*this, &AudioCDProject::on_zoom_clicked));
    m_action_group->add_action("select-mode",
                               sigc::mem_fun(*this, &AudioCDProject::on_select_clicked));

    insert_action_group("box", m_action_group);

    gcdmaster->set_accel_for_action("box.zoom-in",  "<Primary>plus");
    gcdmaster->set_accel_for_action("box.zoom-out", "<Primary>minus");
    gcdmaster->set_accel_for_action("box.zoom-fit", "<Primary>1");
}

//     // Merge menuitems
//     try
//     {
//         Glib::ustring ui_info =
//             "<ui>"
//             "  <menubar name='MenuBar'>"
//             "    <menu action='FileMenu'>"
//             "      <placeholder name='FileSaveHolder'>"
//             "      <menuitem action='Save'/>"
//             "      <menuitem action='SaveAs'/>"
//             "      </placeholder>"
//             "    </menu>"
//             "    <menu action='EditMenu'>"
//             "      <menuitem action='ProjectInfo'/>"
//             "      <menuitem action='CDTEXT'/>"
//             "    </menu>"
//             "    <menu action='ActionsMenu'>"
//             "      <placeholder name='ActionsRecordHolder'>"
//             "        <menuitem action='Record'/>"
//             "      </placeholder>"
//             "      <menuitem action='Play'/>"
//             "      <menuitem action='Stop'/>"
//             "      <menuitem action='Pause'/>"
//             "      <separator/>"
//             "      <menuitem action='Select'/>"
//             "      <menuitem action='Zoom'/>"
//             "      <separator/>"
//             "      <menuitem action='ZoomIn'/>"
//             "      <menuitem action='ZoomOut'/>"
//             "      <menuitem action='ZoomFit'/>"
//             "    </menu>"
//             "  </menubar>"
//             "  <toolbar name='ToolBar'>"
//             "    <toolitem action='Save'/>"
//             "    <toolitem action='Record'/>"
//             "    <separator/>"
//             "    <toolitem action='Play'/>"
//             "    <toolitem action='Stop'/>"
//             "    <toolitem action='Pause'/>"
//             "    <separator/>"
//             "    <toolitem action='Select'/>"
//             "    <toolitem action='Zoom'/>"
//             "    <separator/>"
//             "    <toolitem action='ZoomIn'/>"
//             "    <toolitem action='ZoomOut'/>"
//             "    <toolitem action='ZoomFit'/>"
//             "    <separator/>"
//             "  </toolbar>"
//             "</ui>";

//         //m_refUIManager->add_ui_from_string(ui_info);
//     }
//     catch(const Glib::Error& ex)
//     {
//         std::cerr << "merging menus failed: " <<  ex.what();
//     }

//     Glib::RefPtr<Gtk::Action> action;
//     action = m_action_group->get_action ("Play");
//     action->set_sensitive(true);
//     action = m_action_group->get_action ("Pause");
//     action->set_sensitive(false);
//     action = m_action_group->get_action ("Stop");
//     action->set_sensitive(false);

//     // audioCDView_->add_menus (m_refUIManager);
//     audioCDView_->signal_tocModified.
//         connect(sigc::mem_fun(*this, &AudioCDProject::update));
// }

// void AudioCDProject::configureAppBar(Gnome::UI::AppBar *s, Gtk::ProgressBar* p,
//                                      Gtk::Button *b)
// {
//     statusbar_ = s;
//     progressbar_ = p;
//     progressButton_ = b;

//     if (tocEdit_) {
//         tocEdit_->signalProgressPulse.
//             connect(sigc::mem_fun(*progressbar_, &Gtk::ProgressBar::pulse));
//         signalCancelClicked.connect(sigc::mem_fun(*tocEdit_,
//                                                   &TocEdit::queueAbort));
//         if (tocEdit_->isQueueActive())
//             progressButton_->set_sensitive(true);
//     }

//     progressButton_->signal_clicked().
//         connect(sigc::mem_fun(*this, &AudioCDProject::on_cancel_clicked));
//     progressbar_->set_pulse_step(0.01);
// };

void AudioCDProject::status(const char* msg)
{
    statusMessage(msg);
}

void AudioCDProject::errorDialog(const char* msg)
{
    Gtk::MessageDialog md(*(get_parent_window()), msg, false, Gtk::MESSAGE_ERROR);
    md.run();
}

void AudioCDProject::progress(double val)
{
//    progressbar_->set_fraction(val);
}

void AudioCDProject::fullView()
{
    if (audioCDView_)
        audioCDView_->fullView();
}

void AudioCDProject::sampleSelect(unsigned long start, unsigned long len)
{
    audioCDView_->tocEditView()->sampleSelect(start, len);
}

void AudioCDProject::cancelEnable(bool enable)
{
    if (progressButton_)
        progressButton_->set_sensitive(enable);
}

bool AudioCDProject::closeProject()
{
    if (tocEdit_->tocDirty()) {

        // Project window might be iconified and user might have forgotten
        // about it (Quit can be called on another project window).
        get_parent_window()->present();

        Glib::ustring message = "Project ";
        message += tocEdit_->filename();
        message += " not saved. Are you sure you want to close it ?";

        Gtk::MessageDialog d(*get_parent_window(), message, false,
                             Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL, true);

        int ret = d.run();
        d.hide();

        if (ret == Gtk::RESPONSE_CANCEL || ret == Gtk::RESPONSE_DELETE_EVENT)
            return false;
    }

    if (tocEdit_ && tocEdit_->isQueueActive()) {
        tocEdit_->queueAbort();
    }

    if (playStatus_ == PLAYING || playStatus_ == PAUSED) {
        playStop();
    }

    if (audioCDView_) {
        delete audioCDView_;
        audioCDView_ = NULL;
    }

    if (tocInfoDialog_)   delete tocInfoDialog_;
    if (cdTextDialog_)    delete cdTextDialog_;
    if (recordTocDialog_) delete recordTocDialog_;

    return true;
}

void AudioCDProject::recordToc2CD()
{
    if (recordTocDialog_ == NULL)
        recordTocDialog_ = new RecordTocDialog(tocEdit_);

    recordTocDialog_->start (parent_);
}

void AudioCDProject::projectInfo()
{
    if (!tocInfoDialog_)
        tocInfoDialog_ = new TocInfoDialog(parent_);

    tocInfoDialog_->start(tocEdit_);
}

void AudioCDProject::cdTextDialog()
{
    if (cdTextDialog_ == 0)
        cdTextDialog_ = new CdTextDialog();

    cdTextDialog_->start(tocEdit_);
}

void AudioCDProject::update(unsigned long level)
{
    Glib::RefPtr<Gio::SimpleAction> action;

    level |= tocEdit_->updateLevel();

    if (level & (UPD_TOC_DIRTY | UPD_TOC_DATA))
        updateWindowTitle();

    audioCDView_->update(level);

    if (tocInfoDialog_)
        tocInfoDialog_->update(level, tocEdit_);

    if (cdTextDialog_ != 0)
        cdTextDialog_->update(level, tocEdit_);
    if (recordTocDialog_ != 0)
        recordTocDialog_->update(level);

    if (level & UPD_PLAY_STATUS) {
        bool sensitivity[3][3] = {
            //PLAY  PAUSE STOP
            { false, true, true }, // Playing
            { true, true, true },  // Paused
            { true, false, false } // Stopped
        };

#define GET_THE_FUCKING_ACTION(s) \
    Glib::RefPtr<Gio::SimpleAction>::cast_dynamic((m_action_group->lookup_action(s)))

        action = GET_THE_FUCKING_ACTION("play");
        action->set_enabled(sensitivity[playStatus_][0]);
        action = GET_THE_FUCKING_ACTION("pause");
        action->set_enabled(sensitivity[playStatus_][1]);
        action = GET_THE_FUCKING_ACTION("stop");
        action->set_enabled(sensitivity[playStatus_][2]);
    }

    if (level & UPD_EDITABLE_STATE) {
        bool editable = tocEdit_->editable();
        action = GET_THE_FUCKING_ACTION("play");
        action->set_enabled(editable);
    }
}

void AudioCDProject::playStart()
{
    unsigned long start, end;
 
    // If we're in paused mode, resume playing.
    if (playStatus_ == PAUSED) {
        playStatus_ = PLAYING;
        Glib::signal_idle().connect(sigc::mem_fun(*this,
                                                  &AudioCDProject::playCallback));
        return;
    } else if (playStatus_ == PLAYING) {
        return;
    }

    if (audioCDView_ && audioCDView_->tocEditView()) {
        if (!audioCDView_->tocEditView()->sampleSelection(&start, &end))
            audioCDView_->tocEditView()->sampleView(&start, &end);

        playStart(start, end);
    }
}

void AudioCDProject::playStart(unsigned long start, unsigned long end)
{
    unsigned long level = 0;

    if (playStatus_ == PLAYING)
        return;

    if (tocEdit_->sample_length() == 0) {
        guiUpdate(UPD_PLAY_STATUS);
        return;
    }

    if (soundInterface_ == NULL) {
        soundInterface_ = new SoundIF;
        if (soundInterface_->init() != 0) {
            delete soundInterface_;
            soundInterface_ = NULL;
            guiUpdate(UPD_PLAY_STATUS);
            statusMessage(_("WARNING: Cannot open \"/dev/dsp\""));
            return;
        }
    }

    if (soundInterface_->start() != 0) {
        statusMessage(_("WARNING: Cannot open sound device"));
        guiUpdate(UPD_PLAY_STATUS);
        return;
    }

    tocReader.init(tocEdit_->toc());
    if (tocReader.openData() != 0) {
        tocReader.init(NULL);
        soundInterface_->end();
        guiUpdate(UPD_PLAY_STATUS);
        return;
    }

    if (tocReader.seekSample(start) != 0) {
        tocReader.init(NULL);
        soundInterface_->end();
        guiUpdate(UPD_PLAY_STATUS);
        return;
    }

    playLength_ = end - start + 1;
    playPosition_ = start;
    playStatus_ = PLAYING;
    playAbort_ = false;

    level |= UPD_PLAY_STATUS;

    //FIXME: Selection / Zooming does not depend
    //       on the Child, but the View.
    //       we should have different blocks!
    tocEdit_->blockEdit();

    guiUpdate(level);

    Glib::signal_idle().connect(sigc::mem_fun(*this,
                                              &AudioCDProject::playCallback));
}

void AudioCDProject::playPause()
{
    if (playStatus_ == PAUSED) {
        playStatus_ = PLAYING;
        Glib::signal_idle().connect(sigc::mem_fun(*this,
                                                  &AudioCDProject::playCallback));
    } else if (playStatus_ == PLAYING) {
        playStatus_ = PAUSED;
    }
}

void AudioCDProject::playStop()
{
    if (playStatus() == PAUSED) {
        soundInterface_->end();
        tocReader.init(NULL);
        playStatus_ = STOPPED;
        tocEdit_->unblockEdit();
        playStatus_ = STOPPED;
        guiUpdate(UPD_PLAY_STATUS|UPD_EDITABLE_STATE);
    } else {
        playAbort_ = true;
    }
}

bool AudioCDProject::playCallback()
{
    unsigned long level = 0;

    long len = playLength_ > playBurst_ ? playBurst_ : playLength_;

    if (playStatus_ == PAUSED)
    {
        level |= UPD_PLAY_STATUS;
        guiUpdate(level);
        return false; // remove idle handler
    }

    if (tocReader.readSamples(playBuffer_, len) != len ||
        soundInterface_->play(playBuffer_, len) != 0) {
        soundInterface_->end();
        tocReader.init(NULL);
        playStatus_ = STOPPED;
        level |= UPD_PLAY_STATUS;
        tocEdit_->unblockEdit();
        guiUpdate(level);
        return false; // remove idle handler
    }

    playLength_ -= len;
    playPosition_ += len;

    unsigned long delay = soundInterface_->getDelay();

    if (delay <= playPosition_)
        level |= UPD_PLAY_STATUS;

    if (len == 0 || playAbort_) {
        soundInterface_->end();
        tocReader.init(NULL);
        playStatus_ = STOPPED;
        level |= UPD_PLAY_STATUS | UPD_EDITABLE_STATE;
        tocEdit_->unblockEdit();
        guiUpdate(level);
        return false; // remove idle handler
    }
    else {
        guiUpdate(level);
        return true; // keep idle handler
    }
}

unsigned long AudioCDProject::playPosition()
{
    return playPosition_;
}

unsigned long AudioCDProject::getDelay()
{
    return soundInterface_->getDelay();
}

void AudioCDProject::on_play_clicked()
{
    playStart();
}

void AudioCDProject::on_stop_clicked()
{
    playStop();
}

void AudioCDProject::on_pause_clicked()
{
    playPause();
}

void AudioCDProject::on_zoom_in_clicked()
{
    printf("zoom in\n");
    if (audioCDView_)
        audioCDView_->zoomx2();
}

void AudioCDProject::on_zoom_out_clicked()
{
    printf("zoom out\n");
    if (audioCDView_)
        audioCDView_->zoomOut();
}

void AudioCDProject::on_zoom_fit_clicked()
{
    printf("zoom fit\n");
    if (audioCDView_)
        audioCDView_->fullView();
}

void AudioCDProject::on_zoom_clicked()
{
    if (audioCDView_)
        audioCDView_->setMode(AudioCDView::ZOOM);
}

void AudioCDProject::on_select_clicked()
{
    if (audioCDView_)
        audioCDView_->setMode(AudioCDView::SELECT);
}

void AudioCDProject::on_cancel_clicked()
{
    signalCancelClicked();
}

bool AudioCDProject::appendTrack(const char* file)
{
    FileExtension type = fileExtension(file);

    switch (type) {

    case FE_M3U: {
        std::list<std::string> list;
        if (parseM3u(file, list)) {
            std::list<std::string>::iterator i = list.begin();
            for (; i != list.end(); i++) {
                tocEdit()->queueAppendTrack((*i).c_str());
            }
        } else {
            std::string msg = "Could not read M3U file \"";
            msg += file;
            msg += "\"";
            errorDialog(msg.c_str());
            return false;
        }
        break;
    }

    default:
        tocEdit()->queueAppendTrack(file);
    }

    return true;
}

bool AudioCDProject::appendTracks(std::list<std::string>& files)
{
    std::list<std::string>::iterator i = files.begin();
    for (; i != files.end(); i++) {
        tocEdit()->queueAppendTrack((*i).c_str());
    }
    return true;
}

bool AudioCDProject::appendFiles(std::list<std::string>& files)
{
    std::list<std::string>::iterator i = files.begin();
    for (; i != files.end(); i++) {
        tocEdit()->queueAppendFile((*i).c_str());
    }
    return true;
}

bool AudioCDProject::insertFiles(std::list<std::string>& files)
{
    unsigned long pos;

    TocEditView* view = audioCDView_->tocEditView();
    if (!view) return false;
    if (!view->sampleMarker(&pos)) pos = 0;

    std::list<std::string>::iterator i = files.end();
    do {
        i--;
        tocEdit()->queueInsertFile((*i).c_str(), pos);
    } while (i != files.begin());

    return true;
}

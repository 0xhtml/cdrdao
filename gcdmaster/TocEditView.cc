/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2000 Andreas Mueller <mueller@daneb.ping.de>
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

#include "TocEditView.h"
#include "TocEdit.h"

TocEditView::TocEditView(TocEdit *t)
{
    tocEdit_ = t;

    sampleMarkerValid_ = false;
    sampleSelectionValid_ = false;
    sampleViewMin_ = sampleViewMax_ = 0;
    trackSelectionValid_ = false;
    indexSelectionValid_ = false;
}

TocEditView::~TocEditView()
{
    tocEdit_ = 0;
}

TocEdit *TocEditView::tocEdit() const
{
    return tocEdit_;
}


void TocEditView::sampleMarker(unsigned long sample)
{
    if (sample < tocEdit_->sample_length()) {
        sampleMarker_ = sample;
        sampleMarkerValid_ = true;
    }
    else {
        sampleMarkerValid_ = false;
    }
}

bool TocEditView::sampleMarker(unsigned long *sample) const
{
    if (sampleMarkerValid_)
        *sample = sampleMarker_;

    return sampleMarkerValid_;
}

void TocEditView::sampleSelectAll()
{
    unsigned long slength = tocEdit_->sample_length();

    if (slength) {
        sampleSelectionMin_ = 0;
        sampleSelectionMax_ = slength - 1;
        sampleSelectionValid_ = true;
    }
}

void TocEditView::sampleSelect(unsigned long smin, unsigned long smax)
{
    unsigned long tmp;

    if (smin > smax) {
        tmp = smin;
        smin = smax;
        smax = tmp;
    }

    if (smax < tocEdit_->sample_length()) {
        sampleSelectionMin_ = smin;
        sampleSelectionMax_ = smax;

        sampleSelectionValid_ = true;
    }
    else {
        sampleSelectionValid_ = false;
    }
}

bool TocEditView::sampleSelectionClear()
{
    if (sampleSelectionValid_) {
        sampleSelectionValid_ = false;
        return true;
    }
    return false;
}

bool TocEditView::sampleSelection(unsigned long *smin,
                                  unsigned long *smax) const
{
    if (sampleSelectionValid_) {
        *smin = sampleSelectionMin_;
        *smax = sampleSelectionMax_;
    }

    return sampleSelectionValid_;
}

bool TocEditView::sampleView(unsigned long smin, unsigned long smax)
{
    if (smin <= smax && smax < tocEdit_->sample_length()) {
        sampleViewMin_ = smin;
        sampleViewMax_ = smax;
        return true;
    }
    return false;
}

void TocEditView::sampleView(unsigned long *smin, unsigned long *smax) const
{
    *smin = sampleViewMin_;
    *smax = sampleViewMax_;
}

void TocEditView::sampleViewFull()
{
    sampleViewMin_ = 0;

    if ((sampleViewMax_ = tocEdit_->sample_length()) > 0)
        sampleViewMax_ -= 1;
}

bool TocEditView::is_sample_view_full()
{
    return (sampleViewMin_ == 0 &&
            sampleViewMax_ == -1 || sampleViewMax_ == tocEdit_->sample_length() - 1);
}

void TocEditView::trackSelection(int tnum)
{
    if (tnum > 0) {
        trackSelection_ = tnum;
        trackSelectionValid_ = true;
    }
    else {
        trackSelectionValid_ = false;
    }
}

bool TocEditView::trackSelection(int *tnum) const
{
    if (trackSelectionValid_)
        *tnum = trackSelection_;

    return trackSelectionValid_;
}

void TocEditView::indexSelection(int inum)
{
    if (inum >= 0) {
        indexSelection_ = inum;
        indexSelectionValid_ = true;
    }
    else {
        indexSelectionValid_ = false;
    }
}

bool TocEditView::indexSelection(int *inum) const
{
    if (indexSelectionValid_)
        *inum = indexSelection_;

    return indexSelectionValid_;
}

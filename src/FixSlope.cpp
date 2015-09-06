/*
 * Copyright (c) 2014 Jon Beverley (jon@csdl.biz)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "DataProcessor.h"
#include "Settings.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

// Config widget used by the Preferences/Options config panes
class FixSlope;
class FixSlopeConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixSlopeConfig)
    friend class ::FixSlope;
    protected:
    public:
        // there is no config
        FixSlopeConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixSlopeErrors));
        }

        QString explain() {
            return(QString(tr("Fix or add slope data. If slope data is "
                           "present it will be removed and overwritten.")));
        }

        void readConfig() {}
        void saveConfig() {}

};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixSlope : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixSlope)

    public:
        FixSlope() {}
        ~FixSlope() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent) {
            return new FixSlopeConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Fix Slope errors"));
        }
};

static bool FixSlopeAdded = DataProcessorFactory::instance().registerProcessor(QString("fix slope errors"), new FixSlope());

double avgAlt(RideFile *ride, int i, int vals) {
    double s = 0;
    for(int j = i - vals/2; j < i+vals/s; ++j)
        s += ride->dataPoints()[j]->alt;
    return s/(double)vals;
}

double filterAltData(double alt, double altp, double a) {
    return altp + a * (alt - altp);
}

bool
FixSlope::postProcess(RideFile *ride, DataProcessorConfig *)
{
//    std::vector<double> lowPassAlt = lpFilterAltData(ride, 0.02);
    double slope = 0;
    double alt = 0;
    double altp = ride->dataPoints()[0]->alt;
    double alpha = 0.02f; // whe should dynamically calculate alpha based on time between points
    for (int i = 1; i < ride->dataPoints().count()-1; ++i) {
        alt = filterAltData(ride->dataPoints()[i]->alt, altp, alpha);
        double deltaAlt = alt - altp;
        double deltaDist = (ride->dataPoints()[i+1]->km - ride->dataPoints()[i]->km) * 1000;
        if(deltaDist != 0) {
            slope = deltaAlt/deltaDist * 100.0f;
            if(fabs(slope) > 30.0f)
                slope = ride->dataPoints()[i-1]->slope;
        }
        else
            slope = ride->dataPoints()[i-1]->slope;
        ride->dataPoints()[i]->slope = slope;
        altp = alt;
    }
    ride->command->setDataPresent(RideFile::slope, true);
    return true;
}

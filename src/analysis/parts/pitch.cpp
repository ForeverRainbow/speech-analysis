//
// Created by rika on 16/11/2019.
//

#include "../Analyser.h"
#include "../../lib/Pitch/Pitch.h"

using namespace Eigen;

void Analyser::analysePitch()
{
    Pitch::Estimation est{};

    Pitch::estimate_AMDF(x, fs, est, 60, 700, 1.0, 0.1);

    if (est.isVoiced) {
        double pitch = est.pitch;

        Pitch::estimate_MPM(x, fs, est);
    }

    pitchTrack.pop_front();
    pitchTrack.push_back(est.isVoiced ? est.pitch : 0.0);
}

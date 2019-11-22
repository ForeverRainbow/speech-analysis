//
// Created by rika on 11/11/2019.
//

#include <iostream>
#include "YAAPT.h"

using namespace Eigen;

void YAAPT::refine(
        ConstRefXXd tPitch1, ConstRefXXd tMerit1,
        ConstRefXXd tPitch2, ConstRefXXd tMerit2,
        ConstRefXd sPitch, ConstRefXd energy,
        ConstRefXb vUvEnergy, const Params & prm,
        RefXXd pitch, RefXXd merit)
{
    const double nlfer_thresh2 = prm.nlferThresh2;
    const double meritPivot = prm.meritPivot;

    // Merge pitch candidates and their merits from two types of the signal
    pitch << tPitch1, tPitch2;
    merit << tMerit1, tMerit2;

    // Sort the pitch and merit arrays by descending merit.
    const int maxCands = pitch.rows();
    const int numFrames = pitch.cols();

    std::vector<Index> idx(maxCands);
    std::iota(idx.begin(), idx.end(), 0);

    for (int n = 0; n < numFrames; ++n) {
        std::sort(idx.begin(), idx.end(),
                  [n, &merit](Index i, Index j) { return merit(i, n) > merit(j, n); });

        for (int i = 0; i < maxCands; ++i) {
            auto current = i;
            while (i != idx[current]) {
                auto next = idx[current];
                std::swap(pitch(current, n), pitch(next, n));
                std::swap(merit(current, n), merit(next, n));
                idx[current] = current;
                current = next;
            }
            idx[current] = current;
        }
    }

    // A best pitch track is generated from the best candidates
    ArrayXd bestPitch(pitch.cols());
    medfilt1(pitch.row(0), prm.medianValue, bestPitch);

    bestPitch = vUvEnergy.select(bestPitch, 0);

    // Refine pitch candidates
    for (int i = 0; i < numFrames; ++i) {
        if (energy(i) <= nlfer_thresh2) {
            // Definitely unvoiced, all candidates will be unvoiced with high merit.
            pitch.col(i) = 0;
            merit.col(i) = meritPivot;
        }
        else {
            if (pitch(0, i) > 0) {
                // Already have the voiced candidate, want to have
                // at least one unvoiced candidate
                pitch(maxCands - 1, i) = 0.0;
                merit(maxCands - 1, i) = (1 - merit(0, i));
                for (int j = 1; j < maxCands - 1; ++j) {
                    if (pitch(j, i) == 0) {
                        merit(j, i) = 0.0;
                    }
                }
            }
            else {
                // There was no voiced candidate from NCCF.
                // Fill in option for F0 from spectrogram.
                pitch(0, i) = sPitch(i);
                merit(0, i) = std::min(1.0, energy(i) / 2.0);

                // All other candidates will be unvoiced with low merit.
                pitch(seq(1, last), i) = 0.0;
                merit(seq(1, last), i) = 1 - merit(0, i);
            }
        }
    }

    // Insert some good values from the track that appears
    // the best, without DP, including both F0 and VUV info.
    for (int i = 0; i < numFrames; ++i) {
        pitch(maxCands - 2, i) = bestPitch(i);

        // If this candidate was voiced, already have it, along with merit
        // If unvoiced, need to assign appropriate merit.
        if (bestPitch(i) > 0) {
            merit(maxCands - 2, i) = merit(0, i);
        }
        else {
            merit(maxCands - 2, i) = 1 - std::min(1.0, energy(i) / 2.0);
        }
    }

    // Copy over the sPitch array
    pitch.row(maxCands - 3) = sPitch;
    merit.row(maxCands - 3) = energy / 5.0;
}
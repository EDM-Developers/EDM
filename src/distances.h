#pragma once

#include "common.h"

std::vector<double> lp_distances(int Mp_i, const Options& opts, const Manifold& M, const Manifold& Mp);
std::vector<double> wasserstein_distances(int Mp_i, const Options& opts, const Manifold& M, const Manifold& Mp);
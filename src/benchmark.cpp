#include <benchmark/benchmark.h>

#ifdef _WIN32
#pragma comment(lib, "Shlwapi.lib")
#ifdef _DEBUG
#pragma comment(lib, "benchmarkd.lib")
#else
#pragma comment(lib, "benchmark.lib")
#endif
#endif

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include "driver.h"
#include "edm.h"

#define EIGEN_NO_DEBUG
#define EIGEN_DONT_PARALLELIZE
#include <Eigen/SVD>
typedef Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> MatrixView;

std::vector<size_t> minindex(const std::vector<double>& v, int k);

#include <array>

// Compiler flags tried on Windows: "/GL" and "/GL /LTCG", both slightly worse. "/O2" is the default.

// Inputs generated by 'perf-test.do' script
std::array<std::string, 4> tests = {
  "logmapsmall.h5", // "edm explore x, e(10)" on 200 obs of logistic map
  "logmaplarge.h5", // "edm xmap x y, theta(0.2) algorithm(smap)" on ~50k obs of logistic map
  "affectsmall.h5", // "edm xmap PA NA, dt e(10) k(-1) force alg(smap)" on ~5k obs of affect data
  "affectbige.h5"   // "edm xmap PA NA, dt e(150) k(20) force alg(smap)" on ~5k obs of affect data
};

static void bm_sqrt(benchmark::State& state)
{
  state.SetLabel("'sqrt' function");

  double i = 0.0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(std::sqrt(i));
    i += 1.0;
  }
}

BENCHMARK(bm_sqrt);

static void bm_pow_half(benchmark::State& state)
{
  state.SetLabel("'pow(., 0.5)' function");

  double i = 0.0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(std::pow(i, 0.5));
    i += 1.0;
  }
}

BENCHMARK(bm_pow_half);

void get_distances(int Mp_i, smap_opts_t opts, const std::vector<double>& y, const MatrixView& M, const MatrixView& Mp)
{
  int validDistances = 0;
  std::vector<double> d(M.rows());
  auto b = Mp.row(Mp_i);

  for (int i = 0; i < M.rows(); i++) {
    double dist = 0.;
    bool missing = false;
    int numMissingDims = 0;
    for (int j = 0; j < M.cols(); j++) {
      if ((M(i, j) == MISSING) || (b(j) == MISSING)) {
        if (opts.missingdistance == 0) {
          missing = true;
          break;
        }
        numMissingDims += 1;
      } else {
        dist += (M(i, j) - b(j)) * (M(i, j) - b(j));
      }
    }
    // If the distance between M_i and b is 0 before handling missing values,
    // then keep it at 0. Otherwise, add in the correct number of missingdistance's.
    if (dist != 0) {
      dist += numMissingDims * opts.missingdistance * opts.missingdistance;
    }

    if (missing || dist == 0.) {
      d[i] = MISSING;
    } else {
      d[i] = dist;
      validDistances += 1;
    }
  }
}

static void bm_get_distances(benchmark::State& state)
{
  std::string input = tests[state.range(0)];
  state.SetLabel(input);

  edm_inputs_t vars = read_dumpfile(input);

  MatrixView M((double*)vars.M.flat.data(), vars.M.rows, vars.M.cols);
  MatrixView Mp((double*)vars.Mp.flat.data(), vars.Mp.rows, vars.Mp.cols);

  int Mp_i = 0;
  for (auto _ : state) {
    get_distances(Mp_i, vars.opts, vars.y, M, Mp);
    Mp_i = (Mp_i + 1) % vars.Mp.rows;
  }
}

BENCHMARK(bm_get_distances)->DenseRange(0, tests.size() - 1)->Unit(benchmark::kMicrosecond);

void get_distances_colwise(int Mp_i, smap_opts_t opts, const std::vector<double>& y,
                           const Eigen::Map<Eigen::MatrixXd>& M, const Eigen::Map<Eigen::MatrixXd>& Mp)
{
  int validDistances = 0;
  std::vector<double> d(M.rows());
  auto b = Mp.row(Mp_i);

  for (int i = 0; i < M.rows(); i++) {
    double dist = 0.;
    bool missing = false;
    int numMissingDims = 0;
    for (int j = 0; j < M.cols(); j++) {
      if ((M(i, j) == MISSING) || (b(j) == MISSING)) {
        if (opts.missingdistance == 0) {
          missing = true;
          break;
        }
        numMissingDims += 1;
      } else {
        dist += (M(i, j) - b(j)) * (M(i, j) - b(j));
      }
    }
    // If the distance between M_i and b is 0 before handling missing values,
    // then keep it at 0. Otherwise, add in the correct number of missingdistance's.
    if (dist != 0) {
      dist += numMissingDims * opts.missingdistance * opts.missingdistance;
    }

    if (missing || dist == 0.) {
      d[i] = MISSING;
    } else {
      d[i] = dist;
      validDistances += 1;
    }
  }
}

static void bm_get_distances_colwise(benchmark::State& state)
{
  std::string input = tests[state.range(0)];
  state.SetLabel(input);

  edm_inputs_t vars = read_dumpfile(input);

  MatrixView M((double*)vars.M.flat.data(), vars.M.rows, vars.M.cols);
  MatrixView Mp((double*)vars.Mp.flat.data(), vars.Mp.rows, vars.Mp.cols);

  std::vector<double> M_flat_colwise(vars.M.flat.size());
  std::vector<double> Mp_flat_colwise(vars.Mp.flat.size());

  int ind = 0;
  for (int j = 0; j < vars.M.cols; j++) {
    for (int i = 0; i < vars.M.rows; i++) {
      M_flat_colwise[ind] = M(i, j);
      ind += 1;
    }
  }

  ind = 0;
  for (int j = 0; j < vars.Mp.cols; j++) {
    for (int i = 0; i < vars.Mp.rows; i++) {
      Mp_flat_colwise[ind] = Mp(i, j);
      ind += 1;
    }
  }

  Eigen::Map<Eigen::MatrixXd> M_col(M_flat_colwise.data(), vars.M.rows, vars.M.cols);
  Eigen::Map<Eigen::MatrixXd> Mp_col(Mp_flat_colwise.data(), vars.M.rows, vars.Mp.cols);

  int Mp_i = 0;
  for (auto _ : state) {
    get_distances_colwise(Mp_i, vars.opts, vars.y, M_col, Mp_col);
    Mp_i = (Mp_i + 1) % vars.Mp.rows;
  }
}

BENCHMARK(bm_get_distances_colwise)->DenseRange(0, tests.size() - 1)->Unit(benchmark::kMicrosecond);

void get_distances_reuse(int Mp_i, smap_opts_t opts, const std::vector<double>& y, const MatrixView& M,
                         const MatrixView& Mp)
{
  int validDistances = 0;
  std::vector<double> d(M.rows());
  auto b = Mp.row(Mp_i);

  for (int i = 0; i < M.rows(); i++) {
    double dist = 0.;
    bool missing = false;
    int numMissingDims = 0;
    for (int j = 0; j < M.cols(); j++) {
      double M_ij = M(i, j);
      double b_j = b(j);
      if ((M_ij == MISSING) || (b_j == MISSING)) {
        if (opts.missingdistance == 0) {
          missing = true;
          break;
        }
        numMissingDims += 1;
      } else {
        dist += (M_ij - b_j) * (M_ij - b_j);
      }
    }
    // If the distance between M_i and b is 0 before handling missing values,
    // then keep it at 0. Otherwise, add in the correct number of missingdistance's.
    if (dist != 0) {
      dist += numMissingDims * opts.missingdistance * opts.missingdistance;
    }

    if (missing || dist == 0.) {
      d[i] = MISSING;
    } else {
      d[i] = dist;
      validDistances += 1;
    }
  }
}

static void bm_get_distances_reuse(benchmark::State& state)
{
  std::string input = tests[state.range(0)];
  state.SetLabel(input);

  edm_inputs_t vars = read_dumpfile(input);

  MatrixView M((double*)vars.M.flat.data(), vars.M.rows, vars.M.cols);
  MatrixView Mp((double*)vars.Mp.flat.data(), vars.Mp.rows, vars.Mp.cols);

  int Mp_i = 0;
  for (auto _ : state) {
    get_distances_reuse(Mp_i, vars.opts, vars.y, M, Mp);
    Mp_i = (Mp_i + 1) % vars.Mp.rows;
  }
}

BENCHMARK(bm_get_distances_reuse)->DenseRange(0, tests.size() - 1)->Unit(benchmark::kMicrosecond);

template<typename T>
struct Observation
{
  const T* data;
  size_t E;

  T operator()(size_t j) const { return data[j]; }

  bool any_missing() const
  {
    bool missing = false;
    for (size_t i = 0; i < E; i++) {
      if (data[i] == MISSING) {
        missing = true;
        break;
      }
    }
    return missing;
  }
};

template<typename T>
struct Manifold
{
  std::vector<T> flat;
  size_t _rows, _cols;

  T operator()(size_t i, size_t j) const { return flat[i * _cols + j]; }

  size_t rows() const { return _rows; }
  size_t cols() const { return _cols; }

  Observation<T> get_observation(size_t i) const { return { &(flat[i * _cols]), _cols }; }
};

template<typename T>
std::pair<std::vector<T>, int> get_distances_new(const Manifold<T>& M, const Observation<T>& b, T missingdistance)
{
  int validDistances = 0;
  std::vector<T> d(M.rows());

  for (int i = 0; i < M.rows(); i++) {
    T dist = 0.;
    bool missing = false;
    int numMissingDims = 0;
    for (int j = 0; j < M.cols(); j++) {
      T M_ij = M(i, j);
      T b_j = b(j);
      if ((M_ij == MISSING) || (b_j == MISSING)) {
        if (missingdistance == 0) {
          missing = true;
          break;
        }
        numMissingDims += 1;
      } else {
        dist += (M_ij - b_j) * (M_ij - b_j);
      }
    }
    // If the distance between M_i and b is 0 before handling missing values,
    // then keep it at 0. Otherwise, add in the correct number of missingdistance's.
    if (dist != 0) {
      dist += numMissingDims * missingdistance * missingdistance;
    }

    if (missing || dist == 0.) {
      d[i] = std::numeric_limits<T>::max(); // MISSING doesn't fit inside float
    } else {
      d[i] = dist;
      validDistances += 1;
    }
  }
  return { d, validDistances };
}

static void bm_get_distances_new(benchmark::State& state)
{
  std::string input = tests[state.range(0)];
  state.SetLabel(input);

  edm_inputs_t vars = read_dumpfile(input);

  Manifold<double> M = { vars.M.flat, (size_t)vars.M.rows, (size_t)vars.M.cols };
  Manifold<double> Mp = { vars.Mp.flat, (size_t)vars.Mp.rows, (size_t)vars.Mp.cols };

  int Mp_i = 0;
  for (auto _ : state) {
    Observation<double> b = Mp.get_observation(Mp_i);
    auto [d, validDistances] = get_distances_new(M, b, vars.opts.missingdistance);
    Mp_i = (Mp_i + 1) % vars.Mp.rows;
  }
}

BENCHMARK(bm_get_distances_new)->DenseRange(0, tests.size() - 1)->Unit(benchmark::kMicrosecond);

static void bm_get_distances_new_float(benchmark::State& state)
{
  std::string input = tests[state.range(0)];
  state.SetLabel(input);

  edm_inputs_t vars = read_dumpfile(input);

  std::vector<float> M_flat(vars.M.flat.begin(), vars.M.flat.end());
  std::vector<float> Mp_flat(vars.Mp.flat.begin(), vars.Mp.flat.end());

  Manifold<float> M = { M_flat, (size_t)vars.M.rows, (size_t)vars.M.cols };
  Manifold<float> Mp = { Mp_flat, (size_t)vars.Mp.rows, (size_t)vars.Mp.cols };

  int Mp_i = 0;
  for (auto _ : state) {
    Observation<float> b = Mp.get_observation(Mp_i);
    auto [d, validDistances] = get_distances_new(M, b, (float)vars.opts.missingdistance);
    Mp_i = (Mp_i + 1) % vars.Mp.rows;
  }
}

BENCHMARK(bm_get_distances_new_float)->DenseRange(0, tests.size() - 1)->Unit(benchmark::kMicrosecond);

static void bm_nearest_neighbours(benchmark::State& state)
{
  std::string input = tests[state.range(0)];
  state.SetLabel(input);

  edm_inputs_t vars = read_dumpfile(input);

  MatrixView M((double*)vars.M.flat.data(), vars.M.rows, vars.M.cols);
  MatrixView Mp((double*)vars.Mp.flat.data(), vars.Mp.rows, vars.Mp.cols);

  int Mp_i = 0;
  smap_opts_t opts = vars.opts;

  int validDistances = 0;
  std::vector<double> d(M.rows());
  auto b = Mp.row(Mp_i);

  for (int i = 0; i < M.rows(); i++) {
    double dist = 0.;
    bool missing = false;
    int numMissingDims = 0;
    for (int j = 0; j < M.cols(); j++) {
      if ((M(i, j) == MISSING) || (b(j) == MISSING)) {
        if (opts.missingdistance == 0) {
          missing = true;
          break;
        }
        numMissingDims += 1;
      } else {
        dist += (M(i, j) - b(j)) * (M(i, j) - b(j));
      }
    }
    // If the distance between M_i and b is 0 before handling missing values,
    // then keep it at 0. Otherwise, add in the correct number of missingdistance's.
    if (dist != 0) {
      dist += numMissingDims * opts.missingdistance * opts.missingdistance;
    }

    if (missing || dist == 0.) {
      d[i] = MISSING;
    } else {
      d[i] = dist;
      validDistances += 1;
    }
  }

  int l = opts.l;

  for (auto _ : state) {
    std::vector<size_t> ind = minindex(d, l);
  }
}

BENCHMARK(bm_nearest_neighbours)->DenseRange(0, tests.size() - 1)->Unit(benchmark::kMicrosecond);

static void bm_simplex(benchmark::State& state)
{
  std::string input = tests[state.range(0)];
  state.SetLabel(input);

  edm_inputs_t vars = read_dumpfile(input);

  MatrixView M((double*)vars.M.flat.data(), vars.M.rows, vars.M.cols);
  MatrixView Mp((double*)vars.Mp.flat.data(), vars.Mp.rows, vars.Mp.cols);

  int Mp_i = 0;
  smap_opts_t opts = vars.opts;
  std::vector<double> y = vars.y;

  int validDistances = 0;
  std::vector<double> d(M.rows());
  auto b = Mp.row(Mp_i);

  for (int i = 0; i < M.rows(); i++) {
    double dist = 0.;
    bool missing = false;
    int numMissingDims = 0;
    for (int j = 0; j < M.cols(); j++) {
      if ((M(i, j) == MISSING) || (b(j) == MISSING)) {
        if (opts.missingdistance == 0) {
          missing = true;
          break;
        }
        numMissingDims += 1;
      } else {
        dist += (M(i, j) - b(j)) * (M(i, j) - b(j));
      }
    }
    // If the distance between M_i and b is 0 before handling missing values,
    // then keep it at 0. Otherwise, add in the correct number of missingdistance's.
    if (dist != 0) {
      dist += numMissingDims * opts.missingdistance * opts.missingdistance;
    }

    if (missing || dist == 0.) {
      d[i] = MISSING;
    } else {
      d[i] = dist;
      validDistances += 1;
    }
  }

  int l = opts.l;

  std::vector<size_t> ind = minindex(d, l);

  double d_base = d[ind[0]];
  std::vector<double> w(l);

  double sumw = 0., r = 0.;

  for (auto _ : state) {
    for (int j = 0; j < l; j++) {
      /* TO BE ADDED: benchmark pow(expression,0.5) vs sqrt(expression) */
      /* w[j] = exp(-theta*pow((d[ind[j]] / d_base),(0.5))); */
      w[j] = exp(-opts.theta * sqrt(d[ind[j]] / d_base));
      sumw = sumw + w[j];
    }
    for (int j = 0; j < l; j++) {
      r = r + y[ind[j]] * (w[j] / sumw);
    }
  }
}

BENCHMARK(bm_simplex)->DenseRange(0, tests.size() - 1);

static void bm_smap(benchmark::State& state)
{
  std::string input = tests[state.range(0)];
  state.SetLabel(input);

  edm_inputs_t vars = read_dumpfile(input);

  MatrixView M((double*)vars.M.flat.data(), vars.M.rows, vars.M.cols);
  MatrixView Mp((double*)vars.Mp.flat.data(), vars.Mp.rows, vars.Mp.cols);

  int Mp_i = 0;
  smap_opts_t opts = vars.opts;
  std::vector<double> y = vars.y;

  int validDistances = 0;
  std::vector<double> d(M.rows());
  auto b = Mp.row(Mp_i);

  for (int i = 0; i < M.rows(); i++) {
    double dist = 0.;
    bool missing = false;
    int numMissingDims = 0;
    for (int j = 0; j < M.cols(); j++) {
      if ((M(i, j) == MISSING) || (b(j) == MISSING)) {
        if (opts.missingdistance == 0) {
          missing = true;
          break;
        }
        numMissingDims += 1;
      } else {
        dist += (M(i, j) - b(j)) * (M(i, j) - b(j));
      }
    }
    // If the distance between M_i and b is 0 before handling missing values,
    // then keep it at 0. Otherwise, add in the correct number of missingdistance's.
    if (dist != 0) {
      dist += numMissingDims * opts.missingdistance * opts.missingdistance;
    }

    if (missing || dist == 0.) {
      d[i] = MISSING;
    } else {
      d[i] = dist;
      validDistances += 1;
    }
  }

  int l = opts.l;

  std::vector<size_t> ind = minindex(d, l);

  double d_base = d[ind[0]];
  std::vector<double> w(l);
  double sumw = 0., r = 0.;

  for (auto _ : state) {
    Eigen::MatrixXd X_ls(l, M.cols());
    std::vector<double> y_ls(l), w_ls(l);

    double mean_w = 0.;
    for (int j = 0; j < l; j++) {
      /* TO BE ADDED: benchmark pow(expression,0.5) vs sqrt(expression) */
      /* w[j] = pow(d[ind[j]],0.5); */
      w[j] = sqrt(d[ind[j]]);
      mean_w = mean_w + w[j];
    }
    mean_w = mean_w / (double)l;
    for (int j = 0; j < l; j++) {
      w[j] = exp(-opts.theta * (w[j] / mean_w));
    }

    int rowc = -1;
    for (int j = 0; j < l; j++) {
      if (y[ind[j]] == MISSING) {
        continue;
      }
      bool anyMissing = false;
      for (int i = 0; i < M.cols(); i++) {
        if (M(ind[j], i) == MISSING) {
          anyMissing = true;
          break;
        }
      }
      if (anyMissing) {
        continue;
      }
      rowc++;

      y_ls[rowc] = y[ind[j]] * w[j];
      w_ls[rowc] = w[j];
      for (int i = 0; i < M.cols(); i++) {
        X_ls(rowc, i) = M(ind[j], i) * w[j];
      }
    }
    if (rowc == -1) {
      continue;
    }

    // Pull out the first 'rowc+1' elements of the y_ls vector and
    // concatenate the column vector 'w' with 'X_ls', keeping only
    // the first 'rowc+1' rows.
    Eigen::VectorXd y_ls_cj(rowc + 1);
    Eigen::MatrixXd X_ls_cj(rowc + 1, M.cols() + 1);

    for (int i = 0; i < rowc + 1; i++) {
      y_ls_cj(i) = y_ls[i];
      X_ls_cj(i, 0) = w_ls[i];
      for (int j = 1; j < X_ls.cols() + 1; j++) {
        X_ls_cj(i, j) = X_ls(i, j - 1);
      }
    }

    Eigen::BDCSVD<Eigen::MatrixXd> svd(X_ls_cj, Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::VectorXd ics = svd.solve(y_ls_cj);

    r = ics(0);
    for (int j = 1; j < M.cols() + 1; j++) {
      if (b(j - 1) != MISSING) {
        r += b(j - 1) * ics(j);
      }
    }
  }
}

BENCHMARK(bm_smap)->DenseRange(0, tests.size() - 1);

// Compare the threading libraries against each other on mf_smap_loop
std::array<int, 4> nthreads = { 1, 2, 4, 8 };

void display(const char* s) {}

static void bm_mf_smap_loop(benchmark::State& state)
{
  int testNum = ((int)state.range(0)) / ((int)nthreads.size());
  int threads = nthreads[state.range(0) % nthreads.size()];

  std::string input = tests[testNum];
  state.SetLabel(fmt::format("{} ({} threads)", input, threads));

  edm_inputs_t vars = read_dumpfile(input);

  vars.nthreads = threads;

  for (auto _ : state)
    smap_res_t res = mf_smap_loop(vars.opts, vars.y, vars.M, vars.Mp, vars.nthreads, display);
}

BENCHMARK(bm_mf_smap_loop)
  ->DenseRange(0, tests.size() * nthreads.size() - 1)
  ->MeasureProcessCPUTime()
  ->Unit(benchmark::kMillisecond);

retcode mf_smap_single(int Mp_i, smap_opts_t opts, const std::vector<double>& y, const MatrixView& M,
                       const MatrixView& Mp, std::vector<double>& ystar, std::optional<MatrixView>& Bi_map,
                       bool keep_going() = nullptr);

#ifdef _MSC_VER
#include <omp.h>

smap_res_t mf_smap_loop_openmp(smap_opts_t opts, const std::vector<double>& y, const manifold_t& M,
                               const manifold_t& Mp, int nthreads)
{
  // Create Eigen matrixes which are views of the supplied flattened matrices
  MatrixView M_mat((double*)M.flat.data(), M.rows, M.cols);     //  count_train_set, mani
  MatrixView Mp_mat((double*)Mp.flat.data(), Mp.rows, Mp.cols); // count_predict_set, mani

  std::optional<std::vector<double>> flat_Bi_map{};
  std::optional<MatrixView> Bi_map{};
  if (opts.save_mode) {
    flat_Bi_map = std::vector<double>(Mp.rows * opts.varssv);
    Bi_map = MatrixView(flat_Bi_map->data(), Mp.rows, opts.varssv);
  }

  // OpenMP loop with call to mf_smap_single function
  std::vector<retcode> rc(Mp.rows);
  std::vector<double> ystar(Mp.rows);

  omp_set_num_threads(nthreads);

  int i;
#pragma omp parallel for
  for (i = 0; i < Mp.rows; i++) {
    rc[i] = mf_smap_single(i, opts, y, M_mat, Mp_mat, ystar, Bi_map);
  }

  // Check if any mf_smap_single call failed, and if so find the most serious error
  retcode maxError = *std::max_element(rc.begin(), rc.end());

  return smap_res_t{ maxError, ystar, flat_Bi_map };
}

static void bm_mf_smap_loop_openmp(benchmark::State& state)
{
  int testNum = ((int)state.range(0)) / ((int)nthreads.size());
  int threads = nthreads[state.range(0) % nthreads.size()];

  std::string input = tests[testNum];
  state.SetLabel(fmt::format("{} ({} threads)", input, threads));

  edm_inputs_t vars = read_dumpfile(input);

  for (auto _ : state) {
    smap_res_t res = mf_smap_loop_openmp(vars.opts, vars.y, vars.M, vars.Mp, threads);
  }
}

BENCHMARK(bm_mf_smap_loop_openmp)
  ->DenseRange(0, tests.size() * nthreads.size() - 1)
  ->MeasureProcessCPUTime()
  ->Unit(benchmark::kMillisecond);

#endif

#ifdef _MSC_VER

#include <execution>

smap_res_t mf_smap_loop_cpp17_seq(smap_opts_t opts, const std::vector<double>& y, const manifold_t& M,
                                  const manifold_t& Mp, int nthreads)
{
  // Create Eigen matrixes which are views of the supplied flattened matrices
  MatrixView M_mat((double*)M.flat.data(), M.rows, M.cols);     //  count_train_set, mani
  MatrixView Mp_mat((double*)Mp.flat.data(), Mp.rows, Mp.cols); // count_predict_set, mani

  std::optional<std::vector<double>> flat_Bi_map{};
  std::optional<MatrixView> Bi_map{};
  if (opts.save_mode) {
    flat_Bi_map = std::vector<double>(Mp.rows * opts.varssv);
    Bi_map = MatrixView(flat_Bi_map->data(), Mp.rows, opts.varssv);
  }

  // OpenMP loop with call to mf_smap_single function
  std::vector<retcode> rc(Mp.rows);
  std::vector<double> ystar(Mp.rows);

  std::vector<int> inds(Mp.rows);
  std::iota(inds.begin(), inds.end(), 0);

  std::transform(std::execution::seq, inds.begin(), inds.end(), rc.begin(),
                 [&](int i) { return mf_smap_single(i, opts, y, M_mat, Mp_mat, ystar, Bi_map); });

  // Check if any mf_smap_single call failed, and if so find the most serious error
  retcode maxError = *std::max_element(rc.begin(), rc.end());

  return smap_res_t{ maxError, ystar, flat_Bi_map };
}

static void bm_mf_smap_loop_cpp17_seq(benchmark::State& state)
{
  std::string input = tests[state.range(0)];
  state.SetLabel(input);

  edm_inputs_t vars = read_dumpfile(input);

  for (auto _ : state) {
    smap_res_t res = mf_smap_loop_cpp17_seq(vars.opts, vars.y, vars.M, vars.Mp, vars.nthreads);
  }
}

BENCHMARK(bm_mf_smap_loop_cpp17_seq)
  ->DenseRange(0, tests.size() - 1)
  ->MeasureProcessCPUTime()
  ->Unit(benchmark::kMillisecond);

smap_res_t mf_smap_loop_cpp17_par(smap_opts_t opts, const std::vector<double>& y, const manifold_t& M,
                                  const manifold_t& Mp, int nthreads)
{
  // Create Eigen matrixes which are views of the supplied flattened matrices
  MatrixView M_mat((double*)M.flat.data(), M.rows, M.cols);     //  count_train_set, mani
  MatrixView Mp_mat((double*)Mp.flat.data(), Mp.rows, Mp.cols); // count_predict_set, mani

  std::optional<std::vector<double>> flat_Bi_map{};
  std::optional<MatrixView> Bi_map{};
  if (opts.save_mode) {
    flat_Bi_map = std::vector<double>(Mp.rows * opts.varssv);
    Bi_map = MatrixView(flat_Bi_map->data(), Mp.rows, opts.varssv);
  }

  // OpenMP loop with call to mf_smap_single function
  std::vector<retcode> rc(Mp.rows);
  std::vector<double> ystar(Mp.rows);

  std::vector<int> inds(Mp.rows);
  std::iota(inds.begin(), inds.end(), 0);

  std::transform(std::execution::par, inds.begin(), inds.end(), rc.begin(),
                 [&](int i) { return mf_smap_single(i, opts, y, M_mat, Mp_mat, ystar, Bi_map); });

  // Check if any mf_smap_single call failed, and if so find the most serious error
  retcode maxError = *std::max_element(rc.begin(), rc.end());

  return smap_res_t{ maxError, ystar, flat_Bi_map };
}

static void bm_mf_smap_loop_cpp17_par(benchmark::State& state)
{
  std::string input = tests[state.range(0)];
  state.SetLabel(input);

  edm_inputs_t vars = read_dumpfile(input);

  for (auto _ : state) {
    smap_res_t res = mf_smap_loop_cpp17_par(vars.opts, vars.y, vars.M, vars.Mp, vars.nthreads);
  }
}

BENCHMARK(bm_mf_smap_loop_cpp17_par)
  ->DenseRange(0, tests.size() - 1)
  ->MeasureProcessCPUTime()
  ->Unit(benchmark::kMillisecond);

smap_res_t mf_smap_loop_cpp17_par_unseq(smap_opts_t opts, const std::vector<double>& y, const manifold_t& M,
                                        const manifold_t& Mp, int nthreads)
{
  // Create Eigen matrixes which are views of the supplied flattened matrices
  MatrixView M_mat((double*)M.flat.data(), M.rows, M.cols);     //  count_train_set, mani
  MatrixView Mp_mat((double*)Mp.flat.data(), Mp.rows, Mp.cols); // count_predict_set, mani

  std::optional<std::vector<double>> flat_Bi_map{};
  std::optional<MatrixView> Bi_map{};
  if (opts.save_mode) {
    flat_Bi_map = std::vector<double>(Mp.rows * opts.varssv);
    Bi_map = MatrixView(flat_Bi_map->data(), Mp.rows, opts.varssv);
  }

  // OpenMP loop with call to mf_smap_single function
  std::vector<retcode> rc(Mp.rows);
  std::vector<double> ystar(Mp.rows);

  std::vector<int> inds(Mp.rows);
  std::iota(inds.begin(), inds.end(), 0);

  std::transform(std::execution::par_unseq, inds.begin(), inds.end(), rc.begin(),
                 [&](int i) { return mf_smap_single(i, opts, y, M_mat, Mp_mat, ystar, Bi_map); });

  // Check if any mf_smap_single call failed, and if so find the most serious error
  retcode maxError = *std::max_element(rc.begin(), rc.end());

  return smap_res_t{ maxError, ystar, flat_Bi_map };
}

static void bm_mf_smap_loop_cpp17_par_unseq(benchmark::State& state)
{
  std::string input = tests[state.range(0)];
  state.SetLabel(input);

  edm_inputs_t vars = read_dumpfile(input);

  for (auto _ : state) {
    smap_res_t res = mf_smap_loop_cpp17_par_unseq(vars.opts, vars.y, vars.M, vars.Mp, vars.nthreads);
  }
}

BENCHMARK(bm_mf_smap_loop_cpp17_par_unseq)
  ->DenseRange(0, tests.size() - 1)
  ->MeasureProcessCPUTime()
  ->Unit(benchmark::kMillisecond);

#endif

static void bm_mf_smap_loop_again(benchmark::State& state)
{
  bm_mf_smap_loop(state);
}

BENCHMARK(bm_mf_smap_loop_again)
  ->DenseRange(0, tests.size() * nthreads.size() - 1)
  ->MeasureProcessCPUTime()
  ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
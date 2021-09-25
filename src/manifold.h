#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <Eigen/Core>
#include <nlohmann/json.hpp>
#if defined(WITH_ARRAYFIRE)
#include <arrayfire.h>
#endif

using json = nlohmann::json;

#if defined(WITH_ARRAYFIRE)
struct ManifoldOnGPU {
  af::array mdata;      // shape [_E_actual _nobs 1 1] - manifold
  af::array yvec;       // Shape [_nobs 1 1 1]
  af::array panel;      // Shape [_nobs 1 1 1] - panel ids
  int nobs, E_x, E_dt, E_extras, E_lagged_extras, E_actual;
  double missing;
};
#endif

class Manifold
{
  std::shared_ptr<double[]> _flat = nullptr;
  std::vector<double> _y;
  std::vector<int> _panel_ids;
  int _nobs, _E_x, _E_dt, _E_extras, _E_lagged_extras, _E_actual;
  double _missing;

public:
  Manifold(std::unique_ptr<double[]>& flat, std::vector<double> y, std::vector<int> panelIDs, int nobs, int E_x,
           int E_dt, int E_extras, int E_lagged_extras, int E_actual, double missing)
    : _flat(std::move(flat))
    , _y(y)
    , _panel_ids(panelIDs)
    , _nobs(nobs)
    , _E_x(E_x)
    , _E_dt(E_dt)
    , _E_extras(E_extras)
    , _E_lagged_extras(E_lagged_extras)
    , _E_actual(E_actual)
    , _missing(missing)
  {}

  double operator()(int i, int j) const { return _flat[i * _E_actual + j]; }

  Eigen::Map<const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> map() const
  {
    return { _flat.get(), _nobs, _E_actual };
  }

  Eigen::Map<const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> laggedObsMap(
    int obsNum) const
  {
    int numLaggedExtras = _E_lagged_extras / _E_x;
    return { &(_flat[obsNum * _E_actual]), 1 + (_E_dt > 0) + numLaggedExtras, _E_x };
  }

  Eigen::Map<const Eigen::VectorXd> yMap() const { return { &(_y[0]), _nobs }; }

  double x(int i, int j) const { return _flat[i * _E_actual + j]; }
  double dt(int i, int j) const { return _flat[i * _E_actual + _E_x + j]; }
  double extras(int i, int j) const { return _flat[i * _E_actual + _E_x + _E_dt + j]; }
  int panel(int i) const { return _panel_ids[i]; }

  double unlagged_extras(int obsNum, int varNum) const
  {
    int ind = obsNum * _E_actual + _E_x + _E_dt + _E_lagged_extras + varNum;
    return _flat[ind];
  }

  double range() const
  {
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::min();

    for (int i = 0; i < _nobs * _E_actual; i++) {
      if (_flat[i] != _missing) {
        if (_flat[i] < min) {
          min = _flat[i];
        }
        if (_flat[i] > max) {
          max = _flat[i];
        }
      }
    }
    return max - min;
  }

  double missing() const { return _missing; }

  bool any_missing(int obsNum) const
  {
    for (int j = 0; j < _E_actual; j++) {
      if (operator()(obsNum, j) == _missing) {
        return true;
      }
    }
    return false;
  }

  bool any_not_missing(int obsNum) const
  {
    for (int j = 0; j < _E_actual; j++) {
      if (operator()(obsNum, j) != _missing) {
        return true;
      }
    }
    return false;
  }

  int num_not_missing(int obsNum) const
  {
    int count = 0;
    for (int j = 0; j < _E_actual; j++) {
      if (operator()(obsNum, j) != _missing) {
        count += 1;
      }
    }
    return count;
  }

  double y(int i) const { return _y[i]; }
  int ySize() const { return (int)_y.size(); }

  int nobs() const { return _nobs; }
  int E() const { return _E_x; }
  int E_dt() const { return _E_dt; }
  int E_lagged_extras() const { return _E_lagged_extras; }
  int E_extras() const { return _E_extras; }
  int E_actual() const { return _E_actual; }
  const std::vector<int>& panelIds() const { return _panel_ids; }
  std::shared_ptr<double[]> flatf64() const { return _flat; }
  const std::vector<double>& yvec() const { return _y; }
  std::shared_ptr<double[]> laggedObsMapf64(int obsNum) const {
      return std::move(std::shared_ptr<double[]>(_flat, _flat.get() + obsNum * _E_actual));
  }

#if defined(WITH_ARRAYFIRE)
  ManifoldOnGPU toGPU(const bool useFloat=false) const;
#endif
};

class ManifoldGenerator
{
private:
  bool _use_dt = false;
  bool _add_dt0 = false;
  bool _cumulative_dt = false;
  int _tau;
  double _missing;
  int _num_extras, _num_extras_lagged;
  double _dtWeight = 0.0;
  std::vector<double> _x, _y, _co_x, _t;
  std::vector<std::vector<double>> _extras;
  std::vector<int> _panel_ids;

  double lagged(const std::vector<double>& vec, const std::vector<int>& inds, int i, int j) const;
  double find_dt(const std::vector<int>& inds, int i, int j) const;

public:
  friend void to_json(json& j, const ManifoldGenerator& g);
  friend void from_json(const json& j, ManifoldGenerator& g);

  ManifoldGenerator() = default;

  ManifoldGenerator(const std::vector<double>& t, const std::vector<double>& x, const std::vector<double>& y,
                    const std::vector<std::vector<double>>& extras, int numExtrasLagged, double missing, int tau)
    : _t(t)
    , _x(x)
    , _y(y)
    , _extras(extras)
    , _num_extras((int)extras.size())
    , _num_extras_lagged(numExtrasLagged)
    , _missing(missing)
    , _tau(tau)
  {}

  void add_coprediction_data(const std::vector<double>& co_x) { _co_x = co_x; }

  void add_dt_data(double dtWeight, bool dt0, bool cumulativeDT)
  {
    _dtWeight = dtWeight;
    _use_dt = true;
    _add_dt0 = dt0;
    _cumulative_dt = cumulativeDT;
  }

  void add_panel_ids(const std::vector<int>& panelIDs) { _panel_ids = panelIDs; }

  Manifold create_manifold(int E, const std::vector<bool>& filter, bool copredict, bool prediction) const;

  int E_dt(int E) const { return (_use_dt) * (E - 1 + _add_dt0); }
  int E_extras(int E) const { return _num_extras + _num_extras_lagged * (E - 1); }
  int E_actual(int E) const { return E + E_dt(E) + E_extras(E); }

  int numExtrasLagged() const { return _num_extras_lagged; }
  int numExtras() const { return _num_extras; }
};

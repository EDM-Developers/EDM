#include "driver.h"

int main(int argc, char* argv[])
{
  if (argc < 2) {
    std::cerr << "Usage: ./driver <fname>" << std::endl;
    return -1;
  }

  std::string fname_in(argv[1]);

  Inputs vars = read_dumpfile(fname_in);

  if (argc > 2) {
    vars.opts.nthreads = atoi(argv[2]);
  }
  //vars.opts.nthreads = 1;

  ConsoleIO io;
  Prediction smap_res = mf_smap_loop(vars.opts, vars.y, vars.M, vars.Mp, io);

  std::size_t ext = fname_in.find_last_of(".");
  fname_in = fname_in.substr(0, ext);
  std::string fname_out = fname_in + "-out.h5";

  write_results(fname_out, smap_res, vars.opts.varssv);

  return smap_res.rc;
}
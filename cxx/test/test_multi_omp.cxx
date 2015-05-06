
#include <cstdio>
#include <random>
#include <map>
#include <chrono>

//#include <tomographer/qit/matrq.h>
#include <tomographer/tools/loggers.h>
//#include <tomographer/tomoproblem.h>
//#include <tomographer/integrator.h>
#include <tomographer/dmllhintegrator.h>
#include <tomographer/multiprocomp.h>



// test a customized logger with origin filtering

template<typename BaseLogger>
class OriginFilteredLogger : public Tomographer::LoggerBase<OriginFilteredLogger<BaseLogger> >
{
  BaseLogger & baselogger;

  std::map<std::string,int> levels_set;
public:
  OriginFilteredLogger(BaseLogger & baselogger_)
    :
    // defualt level is the level of the base logger, set LONGDEBUG here.
    Tomographer::LoggerBase<OriginFilteredLogger<BaseLogger> >(Tomographer::Logger::LONGDEBUG),
    baselogger(baselogger_),
    levels_set()
  {
  }

  void setDomainLevel(const std::string& s, int level)
  {
    levels_set[s] = level;
  }
  void removeDomainSetting(const std::string& s)
  {
    auto it = levels_set.find(s);
    if (it == levels_set.end()) {
      this->warning("OriginFilteredLogger<>::removeDomainSetting", "domain not set: `%s'", s.c_str());
      return;
    }
    levels_set.erase(s);
  }

  inline int level() const
  {
    return Tomographer::Logger::LONGDEBUG;
  }

  inline void emit_log(int level, const char * origin, const std::string& msg)
  {
    baselogger.emit_log(level, origin, msg);
  }

  inline bool filter_by_origin(int level, const char * origin) const
  {
    //    std::cout << "filter_by_origin("<<level<<", "<< origin<<");\n";
    auto it = levels_set.find(std::string(origin));
    if (it == levels_set.end()) {
      // rely on default setting
      return Tomographer::Logger::is_at_least_of_severity(level, baselogger.level());
    }
    return Tomographer::Logger::is_at_least_of_severity(level, it->second);
  }

};

namespace Tomographer {
template<typename BaseLogger>
struct LoggerTraits<OriginFilteredLogger<BaseLogger> > : public LoggerTraits<BaseLogger>
{
  enum {
    HasOwnGetLevel = 1, // delegates level() to base logger
    HasFilterByOrigin = 1 // we have customized origin filtering
  };
};
}



int main()
{
  using namespace Tomographer;

  // loggers

  // ---------------
  
  // first, independently, test OMPTaskLogger:      --- WORKS!

  BufferLogger buflog(Logger::DEBUG);
  
  MultiProc::OMPTaskLogger<BufferLogger> testtasklogger(buflog);
  
  testtasklogger.debug("main()", "test task logger: log something to our bufferlogger");
  testtasklogger.longdebug("main()", "test task logger: log something to our bufferlogger on longdebug level");
  
  std::cout << "[WRITTEN DIRECTLY TO STDOUT: TEST BUFFERLOGGER:]\n" << buflog.get_contents() << "\n";

  // ---------------

  SimpleFoutLogger flog1(stdout);//, Logger::DEBUG);

  typedef OriginFilteredLogger<SimpleFoutLogger> OurLogger;

  OurLogger flog(flog1);

  flog1.setLevel(Logger::INFO);
  flog.setDomainLevel("main()", Logger::LONGDEBUG);
  flog.setDomainLevel("MHRandomWalk", Logger::DEBUG);

  // some initializations

  flog.info("main()", "testing our integrator with Pauli meas. on a qubit ... ");

  QubitPaulisMatrQ qmq(2);
  
  typedef IndepMeasTomoProblem<QubitPaulisMatrQ> OurTomoProblem;

  OurTomoProblem dat(qmq);

  dat.Exn = qmq.initVectorParamListType(6);
  flog.debug("main()", [&](std::ostream& str) {
      str << "Exn.size = " << dat.Exn.rows() << " x " << dat.Exn.cols() << "\n";
    });
  dat.Exn <<
    0.5, 0.5,  0.707107,  0,
    0.5, 0.5, -0.707107,  0,
    0.5, 0.5,  0,         0.707107,
    0.5, 0.5,  0,        -0.707107,
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  dat.Nx = qmq.initFreqListType(6);
  //  dat.Nx << 1500, 800, 300, 300, 10, 30;

  // try to reproduce the nice "1qubit-test9-pureup-extreme-onlyupmeas" curve
  dat.Nx << 0, 0, 0, 0, 250, 0;

  dat.x_MLE << 1.0, 0, 0, 0; // pure up state
  dat.T_MLE <<
    1.0, 0,
    0,   0;

  // NOW, RUN THE MH TASKS:

  flog.debug("main()", "Starting to log stuff.");

  // MHRandomWalkTask<TomoProblem, typename Rng = std::mt19937, typename FidelityValueType = double>

  typedef DMLLHIntegratorTasks::CData<OurTomoProblem,FidelityToRefCalculator<OurTomoProblem> > MyCData;
  typedef MultiProc::OMPTaskLogger<OurLogger> MyTaskLogger;
  typedef DMLLHIntegratorTasks::MHRandomWalkTask<OurTomoProblem,FidelityToRefCalculator<OurTomoProblem>,
						 MyTaskLogger> MyMHRandomWalkTask;
  typedef DMLLHIntegratorTasks::MHRandomWalkResultsCollector<
    MyMHRandomWalkTask::ValueHistogramMHRWStatsCollectorType::HistogramType
    >  MyResultsCollector;

  // ---------------

  MyCData taskcdat(dat);
  // seed for random number generator
  taskcdat.base_seed = std::chrono::system_clock::now().time_since_epoch().count();
  // parameters for the fidelity histogram
  taskcdat.histogram_params = MyCData::HistogramParams(0.98, 1.0, 50);
  // parameters of the random walk
  taskcdat.n_sweep = 20;
  taskcdat.n_therm = 100;
  taskcdat.n_run = 1000;
  taskcdat.step_size = 0.05;

  MyResultsCollector results(taskcdat.histogram_params);

//#define clock std::chrono::high_resolution_clock
  typedef std::chrono::system_clock clock;

  auto time_start = clock::now();

  MultiProc::makeOMPTaskDispatcher<MyMHRandomWalkTask>(
      &taskcdat, &results, flog, 128 /* num_runs */, 1 /* n_chunk */
      ).run();

  auto time_end = clock::now();

  // delta-time, in seconds and fraction of seconds
  double dt = (double)(time_end - time_start).count() * clock::period::num / clock::period::den ;
  // integral part and fractional part
  double dt_i_d;
  double dt_f = std::modf(dt, &dt_i_d);
  int dt_i = (int)(dt_i_d+0.5);

  flog.longdebug("main()", [&](std::ostream& str) {
      str << "Integration finished.";
    });

  std::cout << "FINAL HISTOGRAM\n" << results.pretty_print() << "\n";

  std::cout << Tools::fmts("Total elapsed time: %d:%02d:%02d.%03d\n\n",
			   dt_i/3600, (dt_i/60)%60, dt_i%60, (int)(dt_f*1000+0.5)).c_str();
  
  return 0;
}


#include <cstdio>
#include <random>
#include <iostream>

#include <tomographer/qit/matrq.h>
#include <tomographer/loggers.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/integrator.h>
#include <tomographer/dmintegrator.h>
#include <tomographer/multiprocomp.h>



int main()
{
  std::cout << "testing our integrator with Pauli meas. on a qubit ... \n";

  QubitPaulisMatrQ qmq(2);
  
  typedef IndepMeasTomoProblem<QubitPaulisMatrQ> OurTomoProblem;

  OurTomoProblem dat(qmq);

  dat.Exn = qmq.initVectorParamListType(6);
  std::cout << "Exn.size = " << dat.Exn.rows() << " x " << dat.Exn.cols() << "\n";
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

  typedef FidelityHistogramStatsCollector<QubitPaulisMatrQ,VacuumLogger>
    OurFidStatsCollector;

  struct OurCData {
    OurCData(const OurTomoProblem &prob_) : prob(prob_) { }
    OurTomoProblem prob;
    OurFidStatsCollector::HistogramType::Params histogram_params;
  };

  struct OurMHTask {
    int _seed;
    VacuumLogger _vlog;
    OurFidStatsCollector fidstatscollector;

    static inline int get_input(int k, const OurCData * /*pcdata*/)
    {
      return 438943 + k;
    }

    OurMHTask(int inputseed, const OurCData * pcdata)
      : _seed(inputseed),
        _vlog(),
        fidstatscollector(pcdata->histogram_params, pcdata->prob.T_MLE, pcdata->prob.matq, _vlog)
    {
    }

    inline void run(const OurCData * pcdata)
    {
      
      typedef DMStateSpaceRandomWalk<std::mt19937,OurTomoProblem,OurFidStatsCollector,VacuumLogger>
        MyRandomWalk;

      std::mt19937 rng(_seed); // seeded random number generator

      VacuumLogger vlog;

      MyRandomWalk rwalk(20, 500, 40000, 0.05, pcdata->prob.matq.initMatrixType(),
                         pcdata->prob, rng, fidstatscollector, vlog);
      
      rwalk.run();
    }
  };

  struct ResultsCollector {
    OurFidStatsCollector::HistogramType final_histogram;

    ResultsCollector(OurFidStatsCollector::HistogramType::Params p)
      : final_histogram(p)
    {
    }

    inline void init() const { }

    inline void run_finished() const { }

    inline void collect_results(const OurMHTask& t)
    {
      final_histogram.bins += t.fidstatscollector.histogram().bins;
      final_histogram.off_chart += t.fidstatscollector.histogram().off_chart;
    }
  };


  // NOW, RUN THE MH TASKS:

  OurCData taskcdat(dat);
  taskcdat.histogram_params = OurFidStatsCollector::HistogramType::Params(0.98, 1.0, 50);

  ResultsCollector results(taskcdat.histogram_params);

  time_t time_start;
  time(&time_start);

  MultiProc::run_omp_tasks<OurMHTask>(&taskcdat, &results, (size_t)256 /* num_runs */, (size_t)1 /* n_chunk */);

  time_t time_end;
  time(&time_end);

  int dt = time_end-time_start;


  std::cout << "FINAL HISTOGRAM\n" << results.final_histogram.pretty_print() << "\n";

  std::cout << fmts("Total elapsed time: %d:%02d:%02d\n\n", dt/3600, (dt/60)%60, dt%60).c_str();
  
  return 0;
}

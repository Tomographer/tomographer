
# There is nothing to do here because this is a header-only library.


# in the future: add install target.


set(tomographer_HEADERS
  qit/dist.h
  qit/matrq.h
  qit/param_herm_x.h
  qit/util.h
  dmintegrator.h
  histogram.h
  integrator.h
  loggers.h
  multiproc.h
  multiprocomp.h
  tomoproblem.h
  )


install(DIRECTORY .
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tomographer
  FILES_MATCHING
  PATTERN "*.h"
  )

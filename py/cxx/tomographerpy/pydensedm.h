
#ifndef PYDENSEDM_H
#define PYDENSEDM_H

#include <string>

#include <tomographerpy/common.h>

#include <tomographer/densedm/dmtypes.h>
#include <tomographer/densedm/indepmeasllh.h>
#include <tomographer/densedm/param_herm_x.h>


namespace Py {

typedef Tomographer::DenseDM::DMTypes<Eigen::Dynamic, RealType> DMTypes;

typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes, RealType, CountIntType> IndepMeasLLH;

typedef Tomographer::DenseDM::ParamX<DMTypes> ParamX;

}


#endif

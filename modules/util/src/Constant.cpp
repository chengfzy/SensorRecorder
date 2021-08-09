#include "libra/util/Constant.h"

using namespace libra::util;
using namespace Eigen;

#if CxxStd < 17
// redundant decleration, fix c++ flaw if early than c++17
constexpr double Constant::kG;
constexpr double Constant::kEps;
#endif

// definition
const Vector3d Constant::kGVec = Vector3d(0, 0, -kG);
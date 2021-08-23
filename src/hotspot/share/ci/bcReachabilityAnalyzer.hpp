#ifndef SHARE_CI_BCREACHABILITYANALYZER_HPP
#define SHARE_CI_BCREACHABILITYANALYZER_HPP

#include "ci/ciMethod.hpp"

class BCReachabilityAnalyzer : public ResourceObj {

 public:
  BCReachabilityAnalyzer(ciMethod* method, BCReachabilityAnalyzer* parent = NULL);

};

#endif //SHARE_CI_BCREACHABILITYANALYZER_HPP
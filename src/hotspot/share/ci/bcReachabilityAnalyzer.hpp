#ifndef SHARE_CI_BCREACHABILITYANALYZER_HPP
#define SHARE_CI_BCREACHABILITYANALYZER_HPP

#include "ci/ciMethodBlocks.hpp"
#include "ci/ciMethod.hpp"
#include "utilities/growableArray.hpp"

class BCReachabilityAnalyzer : public ResourceObj {
 private:
  GrowableArray<ciType *> _discovered_klasses;
  GrowableArray<ciMethod *> _callees;

  void recordReferencedType(ciType *type);
  void recordCallee(ciMethod *method);
  void iterate_blocks(Arena *arena, ciMethod *method, ciMethodBlocks *methodBlocks);
  void iterate_one_block(ciBlock *blk, ciMethod *method, ciMethodBlocks *methodBlocks, GrowableArray<ciBlock *> &successors);

 public:
  BCReachabilityAnalyzer(ciMethod* method, BCReachabilityAnalyzer* parent = NULL);
  void do_analysis(ciMethod *target);

  GrowableArray<ciMethod *> get_callees() {
    return _callees;
  }

};

#endif //SHARE_CI_BCREACHABILITYANALYZER_HPP
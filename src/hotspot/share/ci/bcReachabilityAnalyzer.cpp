#include "ci/bcReachabilityAnalyzer.hpp"
#include "ci/ciField.hpp"
#include "ci/ciMethodBlocks.hpp"
#include "ci/ciMethod.hpp"
#include "ci/ciSignature.hpp"
#include "ci/ciStreams.hpp"
#include "ci/ciType.hpp"
#include "ci/ciUtilities.hpp"
#include "interpreter/bytecode.hpp"
#include "memory/arena.hpp"
#include "utilities/growableArray.hpp"

BCReachabilityAnalyzer::BCReachabilityAnalyzer(ciMethod* method, BCReachabilityAnalyzer* parent)
{
  do_analysis(method);
}

void BCReachabilityAnalyzer::recordReferencedType(ciType *type) {
    printf("ReferencedType: >");
    fflush(stdout);
    type->print_name();
    printf("||Loaded=%s", type->is_loaded() ? "Y" : "N");
    printf("<\n");
    _discovered_klasses.append(type);
}

void BCReachabilityAnalyzer::recordCallee(ciMethod *method) {
  _callees.append(method);
}

// based on bcEscapeAnalyzer
void BCReachabilityAnalyzer::do_analysis(ciMethod *target) {
  Arena* arena = CURRENT_ENV->arena();
  // identify basic blocks
  ciMethodBlocks *methodBlocks = target->get_method_blocks();

  iterate_blocks(arena, target, methodBlocks);
}

// based on bcEscapeAnalyzer
void BCReachabilityAnalyzer::iterate_blocks(Arena *arena, ciMethod *method, ciMethodBlocks *methodBlocks) {
  int numblocks = methodBlocks->num_blocks();
  int stkSize   = method->max_stack();
  int numLocals = method->max_locals();

  GrowableArray<ciBlock *> worklist(arena, numblocks / 4, 0, NULL);
  GrowableArray<ciBlock *> successors(arena, 4, 0, NULL);

  methodBlocks->clear_processed();

  // initialize block 0 state from method signature
  ciSignature* sig = method->signature();
  ciBlock* first_blk = methodBlocks->block_containing(0);
  int fb_i = first_blk->index();
  for (int i = 0; i < sig->count(); i++) {
    ciType* t = sig->type_at(i);
    if (!t->is_primitive_type()) {
      recordReferencedType(t);
    }
  }

  worklist.push(first_blk);
  while(worklist.length() > 0) {
    ciBlock *blk = worklist.pop();
    if (blk->is_handler() || blk->is_ret_target()) {
      // for an exception handler or a target of a ret instruction, we assume the worst case,
      // that any variable could contain any argument

    } else {

    }
    iterate_one_block(blk, method, methodBlocks, /* state, */ successors);
    // if this block has any exception handlers, push them
    // onto successor list
    if (blk->has_handler()) {
      DEBUG_ONLY(int handler_count = 0;)
      int blk_start = blk->start_bci();
      int blk_end = blk->limit_bci();
      for (int i = 0; i < numblocks; i++) {
        ciBlock *b = methodBlocks->block(i);
        if (b->is_handler()) {
          int ex_start = b->ex_start_bci();
          int ex_end = b->ex_limit_bci();
          if ((ex_start >= blk_start && ex_start < blk_end) ||
              (ex_end > blk_start && ex_end <= blk_end)) {
            successors.push(b);
          }
          DEBUG_ONLY(handler_count++;)
        }
      }
      assert(handler_count > 0, "must find at least one handler");
    }
    // merge computed variable state with successors
    while(successors.length() > 0) {
      ciBlock *succ = successors.pop();
      // merge_block_states(blockstates, succ, &state);
      if (!succ->processed())
        worklist.push(succ);
    }
  }
}

// based on BCEscapeAnalyzer
void BCReachabilityAnalyzer::iterate_one_block(ciBlock *blk, ciMethod *method, ciMethodBlocks *methodBlocks, GrowableArray<ciBlock *> &successors) {
  blk->set_processed();
  ciBytecodeStream s(method);
  int limit_bci = blk->limit_bci();
  bool fall_through = false;

  s.reset_to_bci(blk->start_bci());
  while (s.next() != ciBytecodeStream::EOBC() && s.cur_bci() < limit_bci) {
    fall_through = true;
    printf(".");
    switch (s.cur_bc()) {
      case Bytecodes::_nop:
      case Bytecodes::_aconst_null:
      case Bytecodes::_iconst_m1:
      case Bytecodes::_iconst_0:
      case Bytecodes::_iconst_1:
      case Bytecodes::_iconst_2:
      case Bytecodes::_iconst_3:
      case Bytecodes::_iconst_4:
      case Bytecodes::_iconst_5:
      case Bytecodes::_fconst_0:
      case Bytecodes::_fconst_1:
      case Bytecodes::_fconst_2:
      case Bytecodes::_bipush:
      case Bytecodes::_sipush:
      case Bytecodes::_lconst_0:
      case Bytecodes::_lconst_1:
      case Bytecodes::_dconst_0:
      case Bytecodes::_dconst_1:
        /* No ops */
        break;
      case Bytecodes::_ldc:
      case Bytecodes::_ldc_w:
      case Bytecodes::_ldc2_w:
      {
        // Avoid calling get_constant() which will try to allocate
        // unloaded constant. We need only constant's type.
        int index = s.get_constant_pool_index();
        constantTag tag = s.get_constant_pool_tag(index);
        if (tag.is_long() || tag.is_double()) {
          // do nothing
        } else {
          ciConstant constant = s.get_constant();
          recordReferencedType(constant.as_object()->klass());
        }
        break;
      }
      case Bytecodes::_aload:
      case Bytecodes::_iload:
      case Bytecodes::_fload:
      case Bytecodes::_iload_0:
      case Bytecodes::_iload_1:
      case Bytecodes::_iload_2:
      case Bytecodes::_iload_3:
      case Bytecodes::_fload_0:
      case Bytecodes::_fload_1:
      case Bytecodes::_fload_2:
      case Bytecodes::_fload_3:
      case Bytecodes::_lload:
      case Bytecodes::_dload:
      case Bytecodes::_lload_0:
      case Bytecodes::_lload_1:
      case Bytecodes::_lload_2:
      case Bytecodes::_lload_3:
      case Bytecodes::_dload_0:
      case Bytecodes::_dload_1:
      case Bytecodes::_dload_2:
      case Bytecodes::_dload_3:
      case Bytecodes::_aload_0:
      case Bytecodes::_aload_1:
      case Bytecodes::_aload_2:
      case Bytecodes::_aload_3:
      case Bytecodes::_iaload:
      case Bytecodes::_faload:
      case Bytecodes::_baload:
      case Bytecodes::_caload:
      case Bytecodes::_saload:
      case Bytecodes::_laload:
      case Bytecodes::_daload:
        /* No ops */
        break;
      case Bytecodes::_aaload:
        { // TODO
        }
        break;
      case Bytecodes::_istore:
      case Bytecodes::_fstore:
      case Bytecodes::_istore_0:
      case Bytecodes::_istore_1:
      case Bytecodes::_istore_2:
      case Bytecodes::_istore_3:
      case Bytecodes::_fstore_0:
      case Bytecodes::_fstore_1:
      case Bytecodes::_fstore_2:
      case Bytecodes::_fstore_3:
      case Bytecodes::_lstore:
      case Bytecodes::_dstore:
      case Bytecodes::_lstore_0:
      case Bytecodes::_lstore_1:
      case Bytecodes::_lstore_2:
      case Bytecodes::_lstore_3:
      case Bytecodes::_dstore_0:
      case Bytecodes::_dstore_1:
      case Bytecodes::_dstore_2:
      case Bytecodes::_dstore_3:
      case Bytecodes::_astore:
      case Bytecodes::_astore_0:
      case Bytecodes::_astore_1:
      case Bytecodes::_astore_2:
      case Bytecodes::_astore_3:
      case Bytecodes::_iastore:
      case Bytecodes::_fastore:
      case Bytecodes::_bastore:
      case Bytecodes::_castore:
      case Bytecodes::_sastore:
      case Bytecodes::_lastore:
      case Bytecodes::_dastore:
        /* No op */
        break;
      case Bytecodes::_aastore:
      {
       // TODO?
        break;
      }
      case Bytecodes::_pop:
      case Bytecodes::_pop2:
      case Bytecodes::_dup:
      case Bytecodes::_dup_x1:
      case Bytecodes::_dup_x2:
      case Bytecodes::_dup2:
      case Bytecodes::_dup2_x1:
      case Bytecodes::_dup2_x2:
      case Bytecodes::_swap:
      case Bytecodes::_iadd:
      case Bytecodes::_fadd:
      case Bytecodes::_isub:
      case Bytecodes::_fsub:
      case Bytecodes::_imul:
      case Bytecodes::_fmul:
      case Bytecodes::_idiv:
      case Bytecodes::_fdiv:
      case Bytecodes::_irem:
      case Bytecodes::_frem:
      case Bytecodes::_iand:
      case Bytecodes::_ior:
      case Bytecodes::_ixor:
      case Bytecodes::_ladd:
      case Bytecodes::_dadd:
      case Bytecodes::_lsub:
      case Bytecodes::_dsub:
      case Bytecodes::_lmul:
      case Bytecodes::_dmul:
      case Bytecodes::_ldiv:
      case Bytecodes::_ddiv:
      case Bytecodes::_lrem:
      case Bytecodes::_drem:
      case Bytecodes::_land:
      case Bytecodes::_lor:
      case Bytecodes::_lxor:
      case Bytecodes::_ishl:
      case Bytecodes::_ishr:
      case Bytecodes::_iushr:
      case Bytecodes::_lshl:
      case Bytecodes::_lshr:
      case Bytecodes::_lushr:
      case Bytecodes::_ineg:
      case Bytecodes::_fneg:
      case Bytecodes::_lneg:
      case Bytecodes::_dneg:
      case Bytecodes::_iinc:
      case Bytecodes::_i2l:
      case Bytecodes::_i2d:
      case Bytecodes::_f2l:
      case Bytecodes::_f2d:
      case Bytecodes::_i2f:
      case Bytecodes::_f2i:
      case Bytecodes::_l2i:
      case Bytecodes::_l2f:
      case Bytecodes::_d2i:
      case Bytecodes::_d2f:
      case Bytecodes::_l2d:
      case Bytecodes::_d2l:
      case Bytecodes::_i2b:
      case Bytecodes::_i2c:
      case Bytecodes::_i2s:
      case Bytecodes::_lcmp:
      case Bytecodes::_dcmpl:
      case Bytecodes::_dcmpg:
      case Bytecodes::_fcmpl:
      case Bytecodes::_fcmpg:
        /* no op */
        break;
      case Bytecodes::_ifeq:
      case Bytecodes::_ifne:
      case Bytecodes::_iflt:
      case Bytecodes::_ifge:
      case Bytecodes::_ifgt:
      case Bytecodes::_ifle:
      {
        int dest_bci = s.get_dest();
        assert(methodBlocks->is_block_start(dest_bci), "branch destination must start a block");
        assert(s.next_bci() == limit_bci, "branch must end block");
        successors.push(methodBlocks->block_containing(dest_bci));
        break;
      }
      case Bytecodes::_if_icmpeq:
      case Bytecodes::_if_icmpne:
      case Bytecodes::_if_icmplt:
      case Bytecodes::_if_icmpge:
      case Bytecodes::_if_icmpgt:
      case Bytecodes::_if_icmple:
      {
        int dest_bci = s.get_dest();
        assert(methodBlocks->is_block_start(dest_bci), "branch destination must start a block");
        assert(s.next_bci() == limit_bci, "branch must end block");
        successors.push(methodBlocks->block_containing(dest_bci));
        break;
      }
      case Bytecodes::_if_acmpeq:
      case Bytecodes::_if_acmpne:
      {
        int dest_bci = s.get_dest();
        assert(methodBlocks->is_block_start(dest_bci), "branch destination must start a block");
        assert(s.next_bci() == limit_bci, "branch must end block");
        successors.push(methodBlocks->block_containing(dest_bci));
        break;
      }
      case Bytecodes::_goto:
      {
        int dest_bci = s.get_dest();
        assert(methodBlocks->is_block_start(dest_bci), "branch destination must start a block");
        assert(s.next_bci() == limit_bci, "branch must end block");
        successors.push(methodBlocks->block_containing(dest_bci));
        fall_through = false;
        break;
      }
      case Bytecodes::_jsr:
      {
        int dest_bci = s.get_dest();
        assert(methodBlocks->is_block_start(dest_bci), "branch destination must start a block");
        assert(s.next_bci() == limit_bci, "branch must end block");
        successors.push(methodBlocks->block_containing(dest_bci));
        fall_through = false;
        break;
      }
      case Bytecodes::_ret:
        // we don't track  the destination of a "ret" instruction
        assert(s.next_bci() == limit_bci, "branch must end block");
        fall_through = false;
        break;
      case Bytecodes::_return:
        assert(s.next_bci() == limit_bci, "return must end block");
        fall_through = false;
        break;
      case Bytecodes::_tableswitch:
        {
          Bytecode_tableswitch sw(&s);
          int len = sw.length();
          int dest_bci;
          for (int i = 0; i < len; i++) {
            dest_bci = s.cur_bci() + sw.dest_offset_at(i);
            assert(methodBlocks->is_block_start(dest_bci), "branch destination must start a block");
            successors.push(methodBlocks->block_containing(dest_bci));
          }
          dest_bci = s.cur_bci() + sw.default_offset();
          assert(methodBlocks->is_block_start(dest_bci), "branch destination must start a block");
          successors.push(methodBlocks->block_containing(dest_bci));
          assert(s.next_bci() == limit_bci, "branch must end block");
          fall_through = false;
          break;
        }
      case Bytecodes::_lookupswitch:
        {
          Bytecode_lookupswitch sw(&s);
          int len = sw.number_of_pairs();
          int dest_bci;
          for (int i = 0; i < len; i++) {
            dest_bci = s.cur_bci() + sw.pair_at(i).offset();
            assert(methodBlocks->is_block_start(dest_bci), "branch destination must start a block");
            successors.push(methodBlocks->block_containing(dest_bci));
          }
          dest_bci = s.cur_bci() + sw.default_offset();
          assert(methodBlocks->is_block_start(dest_bci), "branch destination must start a block");
          successors.push(methodBlocks->block_containing(dest_bci));
          fall_through = false;
          break;
        }
      case Bytecodes::_ireturn:
      case Bytecodes::_freturn:
      case Bytecodes::_lreturn:
      case Bytecodes::_dreturn:
        /* no op */
        break;
      case Bytecodes::_areturn:
        // TODO: need to do something?
        fall_through = false;
        break;
      case Bytecodes::_getfield:
      case Bytecodes::_putfield:
        // Fall through to the static case for now and for
        // the loading of the canonical holder of the field.
        // Eventually, we may be able to further optimize this
        // not load instance field-related classes as we won't
        // have an instance to operate on if the class is never
        // allocated.
        // TODO: re-examine fall through here in the future and
        //       potentially optimize the instance case.
      case Bytecodes::_getstatic:
      case Bytecodes::_putstatic:
        {
          // For a static field, we need to ensure that the
          // declared class is loaded.  This code counts on
          // actual loding of the class to ensure that all
          // supers{class, interfaces} will be loaded as per
          // the spec.
          //
          // We don't force the loading of the declared type
          // of the field as we won't necessarily have an
          // instance of the class at runtime.  No need to
          // force it loaded if we don't know it will ever
          // be allocated.
          bool ignored_will_link;
          ciField* field = s.get_field(ignored_will_link);
          recordReferencedType(field->holder());
        }
        break;
      case Bytecodes::_invokevirtual:
      case Bytecodes::_invokespecial:
      case Bytecodes::_invokestatic:
      case Bytecodes::_invokedynamic:
      case Bytecodes::_invokeinterface:

        { bool ignored_will_link;
          ciSignature* declared_signature = NULL;
          ciMethod* target = s.get_method(ignored_will_link, &declared_signature);
          ciKlass*  holder = s.get_declared_method_holder();
          if (true || target->is_static()) {
            // Enqueue the class that declares the method as we need to have
            // it loaded.  For non-static invokes, we need an instance for
            // first for the class to be worth loading.
            // BUT we won't revisit every method every time a class is loaded.
            // Be less conservative and allow additional class loads on the assumption
            // we can remove them later.
            recordReferencedType(holder);
          }
          assert(declared_signature != NULL, "cannot be null");
          // TODO: likely have to pass the bytecode in as well to track the
          // kind of invoke
          recordCallee(target);
        }
  #if 0
          // If the current bytecode has an attached appendix argument,
          // push an unknown object to represent that argument. (Analysis
          // of dynamic call sites, especially invokehandle calls, needs
          // the appendix argument on the stack, in addition to "regular" arguments
          // pushed onto the stack by bytecode instructions preceding the call.)
          //
          // The escape analyzer does _not_ use the ciBytecodeStream::has_appendix(s)
          // method to determine whether the current bytecode has an appendix argument.
          // The has_appendix() method obtains the appendix from the
          // ConstantPoolCacheEntry::_f1 field, which can happen concurrently with
          // resolution of dynamic call sites. Callees in the
          // ciBytecodeStream::get_method() call above also access the _f1 field;
          // interleaving the get_method() and has_appendix() calls in the current
          // method with call site resolution can lead to an inconsistent view of
          // the current method's argument count. In particular, some interleaving(s)
          // can cause the method's argument count to not include the appendix, which
          // then leads to stack over-/underflow in the escape analyzer.
          //
          // Instead of pushing the argument if has_appendix() is true, the escape analyzer
          // pushes an appendix for all call sites targeted by invokedynamic and invokehandle
          // instructions, except if the call site is the _invokeBasic intrinsic
          // (that intrinsic is always targeted by an invokehandle instruction but does
          // not have an appendix argument).
          if (target->is_loaded() &&
              Bytecodes::has_optional_appendix(s.cur_bc_raw()) &&
              target->intrinsic_id() != vmIntrinsics::_invokeBasic) {
            state.apush(unknown_obj);
          }
          // Pass in raw bytecode because we need to see invokehandle instructions.
          invoke(state, s.cur_bc_raw(), target, holder);
          // We are using the return type of the declared signature here because
          // it might be a more concrete type than the one from the target (for
          // e.g. invokedynamic and invokehandle).
          ciType* return_type = declared_signature->return_type();
          if (!return_type->is_primitive_type()) {
            state.apush(unknown_obj);
          } else if (return_type->is_one_word()) {
            state.spush();
          } else if (return_type->is_two_word()) {
            state.lpush();
          }
        }
#endif /* 0 */
        // qbicc does
        //  processReachableInstanceMethodInvoke for { virtual, interface, and special }
        //  processStaticElementInitialization for { static }

        break;
      case Bytecodes::_new:
        // TODO
        // Track the class initializers for current class, supers, and interfaces with defaults
        // Update set of instantiated classes
        //  Update set of reachable classes - walking up the heirarchy and interfaces
        // Enqueue target method for processing
        {
            // this is a new, not a method invoke so following code not valid
            // bool ignored_will_link;
            // ciSignature* declared_signature = NULL;
            // ciMethod* target = s.get_method(ignored_will_link, &declared_signature);
            // ciKlass* holder = s.get_declared_method_holder();
            bool will_link;
            ciKlass* klass = s.get_klass(will_link);
            recordReferencedType(klass);
        }
        break;
      case Bytecodes::_newarray:
        // primitive array - do nothing?  Unless we need to track the primitive arrays for inclusion as well?
        break;
      case Bytecodes::_anewarray:
        {
            bool will_link;
            ciKlass* arrayKlass = s.get_klass(will_link);
            recordReferencedType(arrayKlass);
        }
        // TODO
        break;
      case Bytecodes::_multianewarray:
        {
            int i = s.cur_bcp()[3];
            bool will_link;
            ciKlass* arrayKlass = s.get_klass(will_link);
            // The processor of the class will need to walk the dimensions and try to load
            // each dimension's class after ensuring component type is loaded.
            recordReferencedType(arrayKlass);
            // TODO
        }
        break;
      case Bytecodes::_arraylength:
        break;
      case Bytecodes::_athrow:
        fall_through = false;
        break;
      case Bytecodes::_checkcast:
      case Bytecodes::_instanceof:
        { // TODO: this is questionable - not entirely clear we should be loading classes
          // eagerly here.
          bool ignored_will_link;
          recordReferencedType(s.get_klass(ignored_will_link));
        }
        break;
      case Bytecodes::_monitorenter:
      case Bytecodes::_monitorexit:
        break;
      case Bytecodes::_wide:
        ShouldNotReachHere();
        break;
      case Bytecodes::_ifnull:
      case Bytecodes::_ifnonnull:
      {
        int dest_bci = s.get_dest();
        assert(methodBlocks->is_block_start(dest_bci), "branch destination must start a block");
        assert(s.next_bci() == limit_bci, "branch must end block");
        successors.push(methodBlocks->block_containing(dest_bci));
        break;
      }
      case Bytecodes::_goto_w:
      {
        int dest_bci = s.get_far_dest();
        assert(methodBlocks->is_block_start(dest_bci), "branch destination must start a block");
        assert(s.next_bci() == limit_bci, "branch must end block");
        successors.push(methodBlocks->block_containing(dest_bci));
        fall_through = false;
        break;
      }
      case Bytecodes::_jsr_w:
      {
        int dest_bci = s.get_far_dest();
        assert(methodBlocks->is_block_start(dest_bci), "branch destination must start a block");
        assert(s.next_bci() == limit_bci, "branch must end block");
        successors.push(methodBlocks->block_containing(dest_bci));
        fall_through = false;
        break;
      }
      case Bytecodes::_breakpoint:
        break;
      default:
        ShouldNotReachHere();
        break;
    }

  }
  if (fall_through) {
    int fall_through_bci = s.cur_bci();
    if (fall_through_bci < method->code_size()) {
      assert(methodBlocks->is_block_start(fall_through_bci), "must fall through to block start.");
      successors.push(methodBlocks->block_containing(fall_through_bci));
    }
  }
}
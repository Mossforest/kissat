#include "relaxed.h"
#include "internal.h"
#include "clause.h"
#include "decide.h"
#include "propsearch.h"
#include "backtrack.h"
#include "allocate.h"
#include "stack.h"
#include "resources.h"
#include "relaxed.h"
#include "walk.h"
#include "trail.h"
#include "value.h"
#include "logging.h"
#include "analyze.h"
#include <stdio.h>



void initialize_relaxed_parameters(){
    freeze_restarts = 0;//solver->options.restarts_gap;
    top_trail_sz = ls_call_num = ls_time = 0;
    confl_ratio = 0.4;  // condition_1 percent_ratio
    percent_ratio = 0.9;  // condition_2 confl_ratio
    printf("c parameters for relaxed:\n"); 
    printf("c confl_ratio=%.2f  \tpercent_ratio=%.2f \trestarts_gap=%d\n",
           confl_ratio,percent_ratio,freeze_restarts);
}

bool kissat_meet_relaxed_condition(struct kissat *solver){
    if(freeze_restarts>0) return false;
    int assigned_sz = SIZE_STACK (solver->trail);
    printf("Condition 1: %.2f\n", ((double)(assigned_sz+0.0)/(solver->vars)));
    if (top_trail_sz != 0)
        printf("Condition 2: %.2f\n\n", ((double)(assigned_sz+0.0)/(top_trail_sz)));
    if( ((double)(assigned_sz+0.0)/(solver->vars)) >= confl_ratio)
        return true;
    if( top_trail_sz != 0 && ((double)(assigned_sz+0.0)/(top_trail_sz)) >= percent_ratio)
        return true;
    return false;
}

void kissat_restore_relaxed_phases (kissat *solver) {
  value *const relaxed = solver->phases.relaxed;
  const flags *const flags = solver->flags;
  value *values = solver->values;
  for (all_variables (idx))
    {
      if (!flags[idx].active)
	continue;
      value value = 0;
	    value = relaxed[idx];
      const unsigned lit = LIT (idx);
      const unsigned not_lit = NOT (lit);
      values[lit] = value;
      values[not_lit] = -value;
      LOG ("copied %s relaxed phase %d", LOGVAR (idx), (int) value);
    }
}

bool kissat_call_relaxed_solver(kissat *solver){
    bool res = kissat_walk_relaxed(solver);
    ls_call_num ++;
    return res;
}


int kissat_relaxed_propagate (struct kissat *solver){
    double time_begin = kissat_process_time();
    top_trail_sz = SIZE_STACK (solver->trail);
    
    level_before_relaxed_call_ls = solver->level;
    while(solver->unassigned>0){
        kissat_search_propagate_relaxed (solver);
        if(solver->unassigned>0){
            kissat_decide(solver); //trail++
        }
    }

    // save
    kissat_save_relaxed_phases (solver);
    unsigned save_level = solver->level;
    // unsigned* save_propagated = solver->propagate.end;
    // unsigned* save_trail = solver->trail.end;

    // clear
    solver->level = 0;
    // CLEAR_STACK (solver->trail);
    // CLEAR_STACK (solver->propagate);

    int res = 0;
    bool call_flag = kissat_call_relaxed_solver(solver);
    if (!call_flag) {
        printf("relaxed failed, not complete yet.\n");
        // restore
        solver->level = save_level;
        // solver->propagate.end = save_propagated;
        // solver->trail.end = save_trail;
        kissat_restore_relaxed_phases(solver);
        kissat_backtrack_without_updating_phases(solver, level_before_relaxed_call_ls);
        clause *conflict = kissat_search_propagate (solver);
        if (conflict) 
          res = kissat_analyze (solver, conflict);
    } else
      res = 10;
    freeze_restarts=500;//solver->options.restarts_gap;
    double this_call_time = kissat_process_time() - time_begin;
    ls_time += this_call_time;
    printf("c LS_res=%d, call=%d, restarts=%lu, conflicts=%lu, time=%.2f, all_ls_time=%.2f\n",res,ls_call_num,solver->statistics.restarts,solver->statistics.conflicts,this_call_time,ls_time);
    return res;
}

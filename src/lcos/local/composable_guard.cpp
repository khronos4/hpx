//  (C) Copyright 2013-2015 Steven R. Brandt
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include "hpx/lcos/local/composable_guard.hpp"

namespace hpx { namespace lcos { namespace local {

void run_composable(guard_task *task);
void run_async(guard_task *task);

// A link in the list of tasks attached
// to a guard
struct guard_task {
    std::atomic<guard_task *> next;
    boost::function<void()> run;
    std::atomic<signed char> refcnt;
    bool single_guard;

    guard_task() : next((guard_task *)0), run(0), refcnt(2), single_guard(true) {}

    template<class Archive>
    void serialize(Archive& ar,const unsigned int version) {}
};

void free(guard_task *task) {
    if(task == NULL)
        return;
    BOOST_ASSERT(task->refcnt>0);
    if(--task->refcnt == 0) {
        delete task;
    }
}

int sort_guard(boost::shared_ptr<guard> l1,boost::shared_ptr<guard> l2) {
    return boost::get_pointer(l1) - boost::get_pointer(l2);
}

void guard_set::sort() {
    if(!sorted) {
        std::sort(guards.begin(),guards.end(),sort_guard);
        sorted = true;
    }
}

typedef std::vector<boost::shared_ptr<guard> >::iterator guard_iter;

struct stage_data_element {
    guard_task *stage;
};

struct stage_data {
    boost::function<void()> task;
    const unsigned int n;
    stage_data_element *elems;
    std::atomic<unsigned int> counter;
    stage_data(boost::function<void()> task_,std::vector<boost::shared_ptr<guard> >& guards);
    ~stage_data() {
        delete[] elems;
    }
};

void stage_task(boost::shared_ptr<stage_data> sd,unsigned int i,unsigned int n) {
    unsigned int val = sd->counter.fetch_add(1);
    guard_task *zero = NULL;
    // if this is the last task in the set...
    if(val+1 == n) {
        sd->task();
        // The tasks on the other guards had single_task marked,
        // so they haven't had their next field set yet. Setting
        // the next field is necessary if they are going to
        // continue processing.
        for(unsigned int k=0;k<sd->n;k++) {
            guard_task *lt = sd->elems[k].stage;
            if(!lt->next.compare_exchange_strong(zero,lt)) {
                run_async(lt->next);
            }
            free(lt);
        }
    }
}


stage_data::stage_data(boost::function<void()> task_,std::vector<boost::shared_ptr<guard> >& guards) : task(task_), n(guards.size()), elems(new stage_data_element[n]), counter(0) {
    for(unsigned int i=0;i<n;i++) {
        elems[i].stage = new guard_task();
        elems[i].stage->single_guard = false;
    }
}

void run_guarded(guard& g,guard_task *task) {
    guard_task *zero = NULL;
    guard_task *prev = g.task.exchange(task);
    if(prev != NULL) {
        guard_task *zero = NULL;
        if(!prev->next.compare_exchange_strong(zero,task)) {
            run_async(task);
        }
        free(prev);
    } else {
        run_async(task);
    }
}

void run_guarded(guard_set& guards,boost::function<void()> task) {
    guard_task *zero = NULL;
    int n = guards.guards.size();
    if(n == 0) {
        task();
        return;
    } else if(n == 1) {
        run_guarded(*guards.guards[0],task);
        return;
    }
    boost::shared_ptr<stage_data> sd(new stage_data(task,guards.guards));
    for(unsigned int k=0;k<sd->n;k++) {
        sd->elems[k].stage->run = boost::bind(stage_task,sd,k,n);
        guard_task *stage = sd->elems[k].stage;
        run_guarded(*guards.guards[k],stage);
    }
}

void run_guarded(guard& guard,boost::function<void()> task) {
    guard_task *tptr = new guard_task();
    tptr->run = task;
    run_guarded(guard,tptr);
}
}}};

typedef hpx::actions::plain_action1<
    hpx::lcos::local::guard_task *,
    hpx::lcos::local::run_composable
> composable_run_action;

HPX_REGISTER_PLAIN_ACTION(composable_run_action);

namespace hpx { namespace lcos { namespace local {
void run_async(guard_task *task) {
    hpx::apply<composable_run_action>(hpx::find_here(),task);
}

void run_composable(guard_task *task) {
    BOOST_ASSERT(task != NULL);
    task->run();
    guard_task *zero = 0;
    // If single_guard is false, then this is one of the
    // setup tasks for a multi-guarded task. By not setting
    // the next field, we halt processing on items queued
    // to this guard.
    if(task->single_guard) {
        if(!task->next.compare_exchange_strong(zero,task)) {
            run_async(task->next.load());
        }
        free(task);
    }
}
}}};

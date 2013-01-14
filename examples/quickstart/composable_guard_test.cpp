#include <iostream>
#include <hpx/lcos/local/composable_guard.hpp>

int i1(0), i2(0);
hpx::lcos::local::guard_set guards;
boost::shared_ptr<hpx::lcos::local::guard> l1(new hpx::lcos::local::guard());
boost::shared_ptr<hpx::lcos::local::guard> l2(new hpx::lcos::local::guard());

void incr1() {
    // implicitly lock l1
    i1++;
    // implicitly unlock l1
}
void incr2() {
    // implicitly lock l2
    i2++;
    // implicitly unlock l2
}
void both() {
    // implicitly lock l1 and l2
    i1++;
    i2++;
    // implicitly unlock l1 and l2
}

const int ntimes = 1000;

void check() {
    if(2*ntimes == i1 && 2*ntimes == i2) {
        std::cout << "Test passed" << std::endl;
    } else {
        std::cout << "Test failed: i1=" << i1 << " i2=" << i2 << std::endl;
    }
}

int hpx_main(boost::program_options::variables_map&) {
    // create the guard set
    guards.add(l1);
    guards.add(l2);

    for(int i=0;i<ntimes;i++) {
        // spawn 3 asynchronous tasks
        run_guarded(guards,both);
        run_guarded(*l1,incr1);
        run_guarded(*l2,incr2);
    }

    run_guarded(guards,check);
    return hpx::finalize();
}

int main(int argc, char* argv[]) {
    boost::program_options::options_description
       desc_commandline("Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()
        ;

    // Initialize and run HPX
    return hpx::init(desc_commandline, argc, argv);
}

//  Copyright (c) 2008-2009 Chirag Dekate, Hartmut Kaiser, Anshul Tandon
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_PX_THREAD_MAY_20_2008_0910AM)
#define HPX_PX_THREAD_MAY_20_2008_0910AM

#include <hpx/config.hpp>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/lockfree/atomic_int.hpp>
#include <boost/coroutine/coroutine.hpp>
#include <boost/coroutine/shared_coroutine.hpp>
#include <boost/lockfree/cas.hpp>
#include <boost/lockfree/branch_hints.hpp>

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/components/component_type.hpp>
#include <hpx/components/server/wrapper.hpp>
#include <hpx/lcos/base_lco.hpp>

namespace hpx { namespace threads { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    // This is the representation of a ParalleX thread
    class thread : public lcos::base_lco, private boost::noncopyable
    {
    private:
        typedef 
            boost::coroutines::shared_coroutine<thread_state()> 
        coroutine_type;

    public:
        thread(boost::function<thread_function_type> func, 
                thread_id_type id, threadmanager& tm, thread_state newstate)
          : coroutine_(func, id), tm_(tm), current_state_(newstate)
        {}

        ~thread() 
        {}

        thread_state execute()
        {
            return coroutine_();
        }

        thread_state get_state() const 
        {
            boost::lockfree::memory_barrier();
            return static_cast<thread_state>(current_state_);
        }

        thread_state set_state(thread_state newstate)
        {
            using namespace boost::lockfree;
            for (;;) {
                long prev_state = current_state_;
                if (likely(CAS(&current_state_, prev_state, (long)newstate)))
                    return static_cast<thread_state>(prev_state);
            }
        }

        thread_id_type get_thread_id() const
        {
            return coroutine_.get_thread_id();
        }

        threadmanager& get_thread_manager() 
        {
            return tm_;
        }

    public:
        // action support

        // This is the component id. Every component needs to have an embedded
        // enumerator 'value' which is used by the generic action implementation
        // to associate this component with a given action.
        enum { value = components::component_thread };

        /// 
        thread_state set_event (thread_self&, applier::applier&)
        {
            // we need to reactivate the thread itself
            if (suspended == static_cast<thread_state>(current_state_))
                tm_.set_state(get_thread_id(), pending);

            // FIXME: implement functionality required for depleted state
            return terminated;
        }

    private:
        coroutine_type coroutine_;
        threadmanager& tm_;
        // the state is stored as a long to allow to use CAS
        long current_state_;
    };

///////////////////////////////////////////////////////////////////////////////
}}}

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace threads 
{
    ///////////////////////////////////////////////////////////////////////////
    /// \class thread thread.hpp hpx/runtime/threads/thread.hpp
    ///
    /// A \a thread is the representation of a ParalleX thread. It's a first
    /// class object in ParalleX. In our implementation this is a user level 
    /// thread running on top of one of the OS threads spawned by the \a 
    /// threadmanager.
    ///
    /// A \a thread encapsulates:
    ///  - A thread status word (see the functions \a thread#get_state and 
    ///    \a thread#set_state)
    ///  - A function to execute (the thread function)
    ///  - A frame (in this implementation this is a block of memory used as 
    ///    the threads stack)
    ///  - A block of registers (not implemented yet)
    ///
    /// Generally, \a threads are not created or executed directly. All 
    /// functionality related to the management of \a thread's is 
    /// implemented by the \a threadmanager.
    class thread 
      : public components::wrapper<detail::thread, thread, boost::mpl::true_>
    {
    private:
        typedef detail::thread wrapped_type;
        typedef components::wrapper<
            wrapped_type, thread, boost::mpl::true_> 
        base_type;

        // avoid warning about using 'this' in initializer list
        thread* This() { return this; }

    public:
        /// \brief Construct a new \a thread
        ///
        /// \param func     [in] The thread function to execute by this 
        ///                 \a thread.
        /// \param tm       [in] A reference to the thread manager this 
        ///                 \a thread will be associated with.
        /// \param newstate [in] The initial thread state this instance will
        ///                 be initialized with.
        thread(boost::function<thread_function_type> threadfunc, 
                threadmanager& tm, thread_state new_state = init)
          : base_type(new detail::thread(threadfunc, This(), tm, new_state))
        {}

        ~thread() 
        {}

        thread_id_type get_thread_id() const
        {
            return const_cast<thread*>(this);
        }

        /// \brief Allow to access the thread manager instance this thread has 
        ///        been associated with.
        threadmanager& get_thread_manager() 
        {
            return base()->get_thread_manager();
        }

        /// The get_state function allows to query the state of this thread
        /// instance.
        ///
        /// \returns        This function returns the current state of this
        ///                 thread. It will return one of the values as defined 
        ///                 by the \a thread_state enumeration.
        ///
        /// \note           This function will be seldom used directly. Most of 
        ///                 the time the state of a thread will be retrieved
        ///                 by using the function \a threadmanager#get_state.
        thread_state get_state() const 
        {
            return base()->get_state();
        }

        /// The set_state function allows to change the state this thread 
        /// instance.
        ///
        /// \param newstate [in] The new state to be set for the thread 
        ///                 referenced by the \a id parameter.
        ///
        /// \note           This function will be seldom used directly. Most of 
        ///                 the time the state of a thread will have to be 
        ///                 changed using the threadmanager. Moreover,
        ///                 changing the thread state using this function does
        ///                 not change its scheduling status. It only sets the
        ///                 thread's status word. To change the thread's 
        ///                 scheduling status \a threadmanager#set_state should
        ///                 be used.
        thread_state set_state(thread_state new_state)
        {
            return base()->set_state(new_state);
        }

        /// \brief Execute the thread function
        ///
        /// \returns        This function returns the thread state the thread
        ///                 should be scheduled from this point on. The thread
        ///                 manager will use the returned value to set the 
        ///                 thread's scheduling status.
        thread_state operator()()
        {
            return base()->execute();
        }

    protected:
        base_type& base() { return *this; }
        base_type const& base() const { return *this; }
    };

    ///////////////////////////////////////////////////////////////////////////
    thread_id_type const invalid_thread_id = 0;

    // support for boost::intrusive_ptr<thread>
    inline void intrusive_ptr_add_ref(thread* p)
    {
        ++p->use_count_;
    }

    inline void intrusive_ptr_release(thread* p)
    {
        if (--p->use_count_ == 0)
            delete p;
    }

}}

#endif

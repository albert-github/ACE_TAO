// -*- C++ -*-
// $Id$

#include "DelReplyH_Sender_exec.h"
#include "ace/OS_NS_unistd.h"

namespace CIAO_DelReplyH_Sender_Impl
{
  Atomic_UShort nr_callbacks = 0;
  Atomic_UShort nr_exceptions = 0;

  //============================================================
  // Facet Executor Implementation Class: MyFoo_callback_exec_i
  //============================================================
  MyFoo_callback_exec_i::MyFoo_callback_exec_i (void)
  {
    ACE_DEBUG ((LM_DEBUG,"CONSTRUCTOR replyhandle\n"));
    ++nr_callbacks;
  }

  MyFoo_callback_exec_i::~MyFoo_callback_exec_i (void)
  {
    ACE_DEBUG ((LM_DEBUG,"DESTRUCTOR replyhandle\n"));
    --nr_callbacks;
  }

  //============================================================
  // Operations from ::CCM_AMI::MyFoo_callback
  //============================================================

  // FOO methods
  void
  MyFoo_callback_exec_i::foo (
    ::CORBA::Long /*ami_return_val*/,
    const char * answer)
  {
    ACE_DEBUG ((LM_DEBUG, "OK: ASYNCH foo() returns answer: %C.\n",
                           answer));
  }

  void
  MyFoo_callback_exec_i::foo_excep (
      ::CCM_AMI::ExceptionHolder * /*excep_holder*/)
  {
    ++nr_exceptions;
  }

  //============================================================
  // Worker thread for asynchronous invocations for MyFoo
  //============================================================
  asynch_foo_generator::asynch_foo_generator (
   ::DelReplyH::CCM_Sender_Context_ptr context)
  : context_(::DelReplyH::CCM_Sender_Context::_duplicate (context))
  {
  }

  int asynch_foo_generator::svc ()
  {
    ACE_OS::sleep (3);
    ::DelReplyH::AMI4CCM_MyFoo_var my_foo_ami_  =
       context_->get_connection_sendc_run_my_foo();

    if (CORBA::is_nil (my_foo_ami_))
      {
        ACE_ERROR ((LM_ERROR, "ERROR Sender (ASYNCH) :\tfoo_ami is NIL !\n"));  
        return 1;
      }
    else
      {
        //Invoke Asynchronous calls
        //test with  replyhandler.
        ::DelReplyH::CCM_AMI4CCM_MyFooReplyHandler_var cb = 
          new MyFoo_callback_exec_i ();
        my_foo_ami_->sendc_foo(cb.in(),
                              "Do something asynchronous");
   
      }
      //Invoke Asynchronous calls to test exception handling 
      ::DelReplyH::CCM_AMI4CCM_MyFooReplyHandler_var cb2 =
          new MyFoo_callback_exec_i ();
      my_foo_ami_->sendc_foo (cb2.in (), "");
      
      return 0;
  }

  //============================================================
  // Worker thread for synchronous invocations for MyFoo
  //============================================================
  synch_foo_generator::synch_foo_generator (
    ::DelReplyH::CCM_Sender_Context_ptr context)
  : context_(::DelReplyH::CCM_Sender_Context::_duplicate (context))
  {
  }

  int synch_foo_generator::svc ()
  {
    ACE_OS::sleep (3);
    ::DelReplyH::MyFoo_var my_foo_ami_ =
      context_->get_connection_run_my_foo ();

    //run some synch calls
    try 
      {
        CORBA::String_var answer;
        my_foo_ami_->foo("synchronous call", answer.out ());
        ACE_DEBUG ((LM_DEBUG, "OK: SYNCH foo returns <%C>.\n",
                              answer.in ()));
      }
    catch (const DelReplyH::InternalError& )
      {
        ACE_ERROR ((LM_ERROR, "ERROR: SYNCH foo(): "
                              "Unexpected return.\n"));
      }
    return 0;
  }

  //============================================================
  // Component Executor Implementation Class: Sender_exec_i
  //============================================================
  Sender_exec_i::Sender_exec_i (void)
  {
  }

  Sender_exec_i::~Sender_exec_i (void)
  {
  }

  void
  Sender_exec_i::set_session_context (
    ::Components::SessionContext_ptr ctx)
  {
    this->context_ = ::DelReplyH::CCM_Sender_Context::_narrow (ctx);
    if ( ::CORBA::is_nil (this->context_.in ()))
      {
        throw ::CORBA::INTERNAL ();
      }
  }

  void
  Sender_exec_i::configuration_complete (void)
  {
  }

  void
  Sender_exec_i::ccm_activate (void)
  {
    asynch_foo_generator* asynch_foo_gen =
      new asynch_foo_generator (this->context_.in ());
    asynch_foo_gen->activate (THR_NEW_LWP | THR_JOINABLE, 1);

    synch_foo_generator* synch_foo_gen =
       new synch_foo_generator (this->context_.in());
    synch_foo_gen->activate (THR_NEW_LWP | THR_JOINABLE, 1);
  }

  void
  Sender_exec_i::ccm_passivate (void)
  {
  }

  void
  Sender_exec_i::ccm_remove (void)
  {
    if (nr_callbacks.value() != 0)
      {
        ACE_ERROR ((LM_ERROR, "ERROR: Not all ReplyHandlers were "
                              "deleted, %d left.\n", nr_callbacks.value ()));
      }
    if (nr_exceptions.value() != 1)
      {
        ACE_ERROR ((LM_ERROR, "ERROR: Wrong number of exceptions received. "
          "Expected : 1, Received = %u.\n",nr_exceptions.value()));
      }
    if ((nr_callbacks.value() == 0) && (nr_exceptions.value() == 1))
      {
        ACE_DEBUG ((LM_DEBUG, 
                "OK: All constructed ReplyHandlers were deleted.\n"));
      }
  }

  extern "C"  ::Components::EnterpriseComponent_ptr
  create_DelReplyH_AMI_Sender_Impl (void)
  {
    ::Components::EnterpriseComponent_ptr retval =
      ::Components::EnterpriseComponent::_nil ();

    ACE_NEW_RETURN (
      retval,
      Sender_exec_i,
      ::Components::EnterpriseComponent::_nil ());

    return retval;
  }
}
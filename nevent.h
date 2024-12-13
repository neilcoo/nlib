#ifndef NEVENT_H
#define NEVENT_H

// Nevent v2.3 by Neil Cooper 13th Nov 2014
// Implements an easy-to-use pthread mutex based event object.

#include <pthread.h>  // for pthread_mutex_t and pthread_cond_t
#include "ntime.h"

class Nevent
{
public:
    Nevent( const bool           theCountingEventFlag = false,
            const unsigned long  theInitialState = 0,
            const bool           theManualResetFlag = false   );

    virtual ~Nevent();

    void signal();
    //  Set the event to the signalled state.

    void unsignal();
    // Intended for manual reset events only.
    // Sets a boolean-type event to unsignalled. Decrements a counting-type event by 1.

    void reset();
    // Sets a boolean-type event to unsignalled. Sets a counting-type event to 0.

    unsigned long currentState();
    //  Returns current state of event.
    //  In a multi-threaded environment, no guarantees exist about whether the return value
    //  is still true by the time the function returns.
    //  Return:
    //    n>0: signalled, n==0: not signalled.

    bool wait( const Ntime theTimeout = 0 );
    //  Wait (optionally for the specified amout of time) for the event to be signalled.
    //  Parameters:
    //      theTimeout: Timeout in milliseconds to wait before returning.
    //                  0 = infinite ( i.e. don't return until signalled ).
    //  Return: false if timeout occurred. Always returns true if theTimeout == 0.


private:

   void changeBoolEventState( const bool theState );

   void changeCountingEventState( const long theChange );

   void resetCountingEventState();

   pthread_mutex_t   m_accessMutex;
   pthread_cond_t    m_condition;
   bool              m_isCountingEvent;
   bool              m_isManualReset;

   union
      {
      bool           m_boolEvent;
      unsigned long  m_countingEvent;
      } m_eventUnion;
};


#endif




// ---
// Simple semaphore spinlock and unlock functions: 
//    lcLock(long int& s)      -- locks the semaphore w/spinlock wait
//    lcUnlock(long int& s)    -- frees the semaphore
//
//    where 's' is the semaphore that must be initialized to zero.
//
// Visual Studio version
// ---

inline void lcLock(long int& semaphore)
{
   // Try to lock the semaphore (i.e., set it to one), but make
   // sure that it was free (i.e., previously set to zero).
   // Otherwise wait (spin) until it is free.
#if 1
   while (_InterlockedCompareExchange(static_cast<long int*>(&semaphore), 1, 0) == 1) { _mm_pause(); }
#else
   _asm {
      push eax
      push ecx
      mov  eax, semaphore
   $DPG001:
      mov  ecx, 1
      lock xchg [eax], ecx
      test ecx, ecx
      je SHORT $DPG003
   $DPG002:
      pause
      cmp [eax], 0
      jne SHORT $DPG002
      jmp SHORT $DPG001
   $DPG003:
      pop  ecx
      pop  eax
   }
#endif
}

inline void lcUnlock(long int& semaphore)
{
   // free the semaphore (i.e., just set it to zero)
#if 1
   _InterlockedExchange(static_cast<long int*>(&semaphore), 0);
#else
   _asm {
      push eax
      push ecx
      mov  eax, semaphore
      mov  ecx, 0
      lock xchg [eax], ecx
      pop  ecx
      pop  eax
   }
#endif
}

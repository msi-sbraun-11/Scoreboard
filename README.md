# Scoreboard
Implementing the Scoreboarding algorithm in C++.  The scoreboarding algorithm in computer architecture manages out-of-order instruction execution while handling data hazards and resource conflicts. It tracks the status of instructions, ensuring proper synchronization of operands and functional units.

There are 3 parts to the Scoreboard:
  1. _Instruction status_ — Indicates which of the four steps the instruction is in.
  2. _Functional unit status_ — Indicates the state of the functional unit (FU). There are nine fields for each functional unit:
       * Busy   —   Indicates whether the unit is busy or not.
       * Op     —   Operation to perform in the unit (e.g., add or subtract).
       * Fi     —   Destination register.
       * Fj, Fk —   Source-register numbers.
       * Qj, Qk —   Functional units producing source registers Fj, Fk.
       * Rj, Rk —   Flags indicating when Fj, Fk are ready and not yet read. Set to No after operands are read.
  3. _Register result status_ — Indicates which functional unit will write each register, if an active instruction has the register as its destination. This field is set to blank whenever there are no pending instructions that will write that register.    

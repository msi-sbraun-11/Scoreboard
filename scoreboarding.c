// simple scoreboard for 6 instructions
#include <stdbool.h>
#define ADD1 0
#define ADD2 1
#define MUL1 2
#define MUL2 3
#define DIV 4
#define INTUNIT 5

typedef struct record
{
    bool busy;
    char op[7];
    short int Fi, Fj, Fk;
    char Qi[7], Qj[7], Qk[7];
    bool Rj, Rk;
}record;

int R[32];      // integer register file
float F[16];    // FP register file

short int FPAdder[2][2], // 2 add units; s, t; takes 2 cycles for EX
    FPMul[2][2],   // 1 multiply units; s, t; takes 6 cycles for EX
    FPDiv[1][2],   // 1 division unit; s, t; takes 11 cycles for EX
    IntALU[1][2];  // 1 integer unit; s, t; takes 1 cycle for EX

/** components of the Scoreboard **/
short int InstStatus[6][4]; // for 6 instructions only
record Scoreboard[6]; // functional units
int RegStatus[16];

void issue();
void readOperands();
void execute();
void writeResult();

int main()
{
    short int cycle = 0, inst_completed = 0;
    while(inst_completed != 6)
    {

    }

    return 0;
}

void issue()
{

}
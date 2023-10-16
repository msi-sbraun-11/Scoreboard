// simple scoreboard
#include <stdbool.h>
#define ADD1 1
#define ADD2 2
#define MUL1 3
#define MUL2 4
#define DIV 5
#define INTUNIT 6

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

short int FPAdder[2][2], // 2 add units; takes 2 cycles for EX
    FPMul[2][2],   // 1 multiply units; takes 6 cycles for EX
    FPDiv[1][2],   // 1 division unit; takes 11 cycles for EX
    IntALU[1][2];  // 1 integer unit; takes 1 cycle for EX

/** components of the Scoreboard **/
short int InstStatus[4][4];
record Scoreboard[6];
int RegStatus[16];

void issue();
void readOperands();
void execute();
void writeResult();

int main()
{

    return 0;
}

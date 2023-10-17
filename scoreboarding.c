// simple scoreboard for 6 instructions: primarily for the Floating-Point unit
#include <stdbool.h>
#include <stdio.h>

#define ADD1 0
#define ADD2 1
#define MUL1 2
#define MUL2 3
#define DIV1 4
#define ADDER 5

typedef struct record
{
    bool busy;            // is the functional unit being used?
    char op[7];           // opcode
    short int Fi, Fj, Fk; // destination, source1, source2
    char Qj[7], Qk[7];    // functional units producing Fj and Fk
    bool Rj, Rk;          // is Fj and Fk available?
}record;

int R[32];      // Integer register file
float F[16];    // FP register file

short int FPAdder[2][2], // 2 add units; s, t;          takes 2 cycles for EX
    FPMul[2][2],         // 1 multiply units; s, t;     takes 6 cycles for EX
    FPDiv[1][2],         // 1 division unit; s, t;      takes 11 cycles for EX
    Adder[1][2];        // 1 adder for calculating 
                        // effective address; s, t;     takes 1 cycle for EX

/** components of the Scoreboard **/

short int InstStatus[6][4];     // for 6 instructions only
record Scoreboard[6];           // functional units
int RegStatus[16];

void issue();
void readOperands();
void execute();
void writeResult();
void initializeScoreBoard();

int main()
{
    short int cycle = 0;
    FILE *file;
    file = fopen("instructions.txt", "r");
    char opcode[7], d[4], s[4], t[4];
    initializeScoreBoard();
    while(1)
    {
        int result = fscanf(file, "%s %s %s %s", opcode, d, s, t);
        if(result == EOF) break;
        else
        {

        }
    }

    return 0;
}

void initializeScoreBoard()
{
    for(int i=0; i<6; i++)
    {
        Scoreboard[i].busy = false;
        Scoreboard[i].Rj = Scoreboard[i].Rk = true;
    }
}

void issue(char op[7], char d[4], char s[4], char t[4])
{
    if(strcmp(op, "L.D") == 0)
    {
        if(Scoreboard[ADDER].busy == false)
        {
            
        }
    }
    else if(strcmp(op, "S.D") == 0)
    {
        if(Scoreboard[ADDER].busy == false)

    }
    else if(strcmp(op, "MUL.D") == 0)
    {
        if(Scoreboard[MUL1].busy == false)

        else if(Scoreboard[MUL2].busy == false)

    }
    else if(strcmp(op, "ADD.D") == 0)
    {
        if(Scoreboard[ADD1].busy == false)

        else if(Scoreboard[ADD2].busy == false)
    }
    else if(strcmp(op, "DIV.D") == 0)
    {
        if(Scoreboard[DIV1].busy == false)

    }
}
// simple scoreboard for 6 instructions: primarily for the Floating-Point unit
#include <iostream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <utility>

using namespace std;

#define ADD1 0
#define ADD2 1
#define MUL1 2
#define MUL2 3
#define DIV1 4
#define ADDER 5

#define OPLEN 7
#define NUM_FU 6
#define INTREGFILESIZE 32
#define FLOATREGFILESIZE 16
#define NUM_FPADD 2
#define NUM_FPMUL 2
#define NUM_FPDIV 1
#define NUM_ADDER 1

class record
{
    bool busy;            // is the functional unit being used?
    short int op;         // opcode
    short int Fi, Fj, Fk; // destination, source1, source2
    short int Qj, Qk;     // functional units producing Fj and Fk
    bool Rj, Rk;          // is Fj and Fk available?
    short int StallsLeft; // need to count how many stalls left; helps in indicating that the instruction has executed
    short int InstStatus; // useful to know at which clock cycle instruction finished its stage

    public:
        record()
        {
            busy = false;
            Fi = Fj = Fk = -1;
            Qj = Qk = -1;
            Rj = Rk = false;
            StallsLeft = 0;
            InstStatus = 0;
        }
};

class scoreboard
{
    int cycle;
    vector<record> fu_status(NUM_FU);     // functional unit status
    vector<int> R(INTREGFILESIZE);      // Integer register file
    vector<float> F(FLOATREGFILESIZE);  // FP register file
    vector<int> FloatRegStatus(-1, FLOATREGFILESIZE), IntRegStatus(-1, INTREGFILESIZE); // register status

    // we are giving register number, not the value as such; so let's make it <int,int>.
    // In the readOperands() and execute() function, we can get the value and pass.

    /* vector<pair<float, float>> FPAdder(NUM_FPADD), // 2 add units; s, t;          takes 2 cycles for EX
            FPMul(NUM_FPMUL),                      // 2 multiply units; s, t;     takes 6 cycles for EX
            FPDiv(NUM_FPDIV);                      // 1 division unit; s, t;      takes 11 cycles for E
    vector<pair<int, int>> Adder(NUM_ADDER);       // 1 adder for calculating 
                                                   // effective address; s, t;    takes 1 cycle for EX */

    void setrecord(int, int, int, int, int);
    public:
        bool issue(string, int, int, int);
        bool readOperands(int);
        bool execute();
        bool writeResult();
};

void setrecord(int fu, int opcode, int d, int s, int t)
{
    fu_status[fu].busy = true;
    fu_status[fu].op = opcode;
    fu_status[fu].Fi = d;
    fu_status[fu].Fj = s;
    fu_status[fu].Fk = t;
    FloatRegStatus[fu_status[fu].Fi] = fu;
    fu_status[fu].Qj = FloatRegStatus[Fj];
    fu_status[fu].Qk = FloatRegStatus[Fk];
    if(fu_status[fu].Qj == -1)
        fu_status[fu].Rj = true;
    else fu_status[fu].Rj = false;
    if(fu_status[fu].Qk == -1)
        fu_status[fu].Rk = true;
    else fu_status[fu].Rk = false;
}

bool scoreboard::issue(string op, int d, int s, int t) // need to take care of offset shift to diff bw IntRegFile and FloatRegFile
{
    unordered_map<string, int> optab = {
        {"L.D", 0},
        {"S.D", 1},
        {"MUL.D", 2},
        {"ADD.D", 3},
        {"DIV.D", 4},
         };
    bool issued = false; // We need to indicate that the instruction has been issued so that in the execution loop, 
                         // we can move to the next stage
    switch(optab[op])
    {
        case 0: 
                if(!fu_status[ADDER].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(ADDER, optab[op], d, s, t);
                    issued = true;
                }    
                else
                    issued = false;
                break;

        case 1:
                if(!fu_status[ADDER].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(ADDER, optab[op], d, s, t);
                    issued = true;
                }    
                else
                    issued = false;
                break;

        case 2:
                if(!fu_status[MUL1].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(MUL1, optab[op], d, s, t);
                    issued = true;
                }    
                else if(!fu_status[MUL2].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(MUL2, optab[op], d, s, t);
                    issued = true;
                }
                else
                    issued = false;
                break;

        case 3:
                if(!fu_status[ADD1].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(ADD1, optab[op], d, s, t);
                    issued = true;
                }
                else if(!fu_status[ADD2].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(ADD2, optab[op], d, s, t);
                    issued = true;
                }
                else
                    issued = false;
                break;

        case 4:
                if(!fu_status[DIV1].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(DIV1, optab[op], d, s, t);
                    issued = true;
                }
                else
                    issued = false;
                break;
    }

    return issued;
/*     if(strcmp(op, "L.D") == 0)
    {
        if(Scoreboard[ADDER].busy == false)
        {
            
        }
        else 
            return false;
    }
    else if(strcmp(op, "S.D") == 0)
    {
        if(Scoreboard[ADDER].busy == false)
        {
            
        }
        else 
            return false;
    }
    else if(strcmp(op, "MUL.D") == 0)
    {
        if(Scoreboard[MUL1].busy == false)
        {
            
        }
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

    } */
}

bool readOperands(int fu) // doesn't need any arguments because all the data that it needs is present in the status table
{
    if(fu_status[fu].Rj && fu_status[fu].Rk)
    {
        fu_status[fu].Fj = FloatRegStatus[fu_status[fu].Fj];
        fu_status[fu].Fk = FloatRegStatus[fu_status[fu].Fk];
        return true;
    }    
    else
        return false;
}

bool execute()
{

}

bool writeResult()
{

}

int main()
{
    // short int cycle = 0;
    FILE *file;
    file = fopen("instructions.txt", "r");
    char opcode[7], d[4], s[4], t[4];
    // initializeScoreBoard();
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
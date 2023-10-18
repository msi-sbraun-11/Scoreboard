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
    string op;            // opcode
    short int Fi, Fj, Fk; // destination, source1, source2
    short int Qj, Qk;     // functional units producing Fj and Fk
    bool Rj, Rk;          // is Fj and Fk available?
    short int StallsLeft; // need to count how many stalls left; helps in indicating that the instruction has executed
    short int InstStatus; // useful to knoiw at which clock cycle instruction finished its stage

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
    list<record> fu_status(NUM_FU);     // functional unit status
    vector<int> R(INTREGFILESIZE);      // Integer register file
    vector<float> F(FLOATREGFILESIZE);  // FP register file
    vector<int> FloatRegStatus(-1, FLOATREGFILESIZE), IntRegStatus(-1, INTREGFILESIZE); // register status

    // we are giving register number, not the value as such; so let's make it <int,int>.
    // In the readOperands() and execute() function, we can get the value and pass.

    vector<pair<int, int>> FPAdder(NUM_FPADD),     // 2 add units; s, t;          takes 2 cycles for EX
            FPMul(NUM_FPMUL),                      // 2 multiply units; s, t;     takes 6 cycles for EX
            FPDiv(NUM_FPDIV);                      // 1 division unit; s, t;      takes 11 cycles for E
    vector<pair<int, int>> Adder(NUM_ADDER);       // 1 adder for calculating 
                                                   // effective address; s, t;    takes 1 cycle for EX
    public:
        bool issue(string, int, int, int);
        void readOperands(scoreboard*);
    void execute(scoreboard*);
    void writeResult(scoreboard*);
};

scoreboard::scoreboard()
{
    FloatRegStatus = {-1};
    IntRegStatus = {-1};
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
                if(not /* ! */ fu_status[ADDER].busy /* && */ and
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    Adder.push_back(make_pair(s, t));
                    issued = true;
                }    
                else
                    issued = false;
                break;

        case 1:
                if(not /* ! */ fu_status[ADDER].busy /* && */ and
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    Adder.push_back(make_pair(s, t));
                    issued = true;
                }    
                else
                    issued = false;
                break;

        case 2:
                if(not /* ! */ fu_status[MUL1].busy /* && */ and
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    FPMul[0].push_back(make_pair(s, t));
                    issued = true;
                }    
                else if(not /* ! */ fu_status[MUL2].busy /* && */ and
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    FPMul[1].push_back(make_pair(s, t));
                    issued = true;
                }
                else
                    issued = false;
                break;

        case 3:
                if(not /* ! */ fu_status[ADD1].busy /* && */ and
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    FPAdder[0].push_back(make_pair(s, t));
                    issued = true;
                }
                else if(not /* ! */ fu_status[ADD2].busy /* && */ and
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    FPAdder[1].push_back(make_pair(s, t));
                    issued = true;
                }
                break;

        case 4:
                if(not /* ! */ fu_status[DIV1].busy /* && */ and
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    FPDiv[0].push_back(make_pair(s, t));
                    issued = true;
                }
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
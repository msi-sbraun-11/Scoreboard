// simple scoreboard for 6 instructions: primarily for the Floating-Point unit
#include <iostream>
#include <fstream>
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
#define MEMSIZE 512
#define NUM_FPADD 2
#define NUM_FPMUL 2
#define NUM_FPDIV 1
#define NUM_ADDER 1

enum {ADD, MUL, DIV, LOAD, STORE};
union numeric
{
    float ValF;
    int ValI;
};

class record
{
    // enum{ADD, MUL, DIV, ADDER} FuncUnitType;
    bool busy;            // is the functional unit being used?
    short int op;         // opcode
    short int Fi, Fj, Fk; // destination, source1, source2
    short int Qj, Qk;     // functional units producing Fj and Fk
    bool Rj, Rk;          // is Fj and Fk available?
    bool immediate;       // is the instruction an immediate instruction? (needed for LOAD and STORE operations)
    short int StallsLeft; // need to count how many stalls left; helps in indicating that the instruction has executed
    vector<short int> InstStatus; // useful to know at which clock cycle instruction finished its stage
    numeric Vj, Vk, temp;

    public:
        record()
        {
            busy = false;
            Fi = Fj = Fk = -1;
            Qj = Qk = -1;
            Rj = Rk = false;
            StallsLeft = 0;
            immediate = false;
        }
        friend class scoreboard;    
};

class scoreboard
{
    int cycle;
    vector<record> fu_status(NUM_FU);       // functional unit status
    vector<int> R(INTREGFILESIZE);          // Integer register file FLOATREGFILESIZE...FLOATREGFILESIZE+INTREGFILESIZE-1
    vector<float> F(FLOATREGFILESIZE);      // FP register file 0...FLOATREGFILESIZE-1
    vector<int> FloatRegStatus(-1, FLOATREGFILESIZE), IntRegStatus(-1, INTREGFILESIZE); // register status
    vector<numeric> Memory(MEMSIZE);
    // we are giving register number, not the value as such; so let's make it <int,int>.
    // In the readOperands() and execute() function, we can get the value and pass.

    // FPADD 3 cycles for EX
    // FPMUL 5 cycles for EX
    // FPDIV 11 cycles for EX
    // ADDER 1 cycle for calculating effective address; 1 cycle for reading from/writing to memory
    // therefore 2 cycles for EX
    
    void setrecord(int, int, int, int, int);
    void resetRecord(int);
    public:
        bool issue(string, int, int, int, bool);
        bool readOperands(int);
        bool execute(int);
        bool writeResult(int);
        void ExecutionLoop();
};

void scoreboard::setrecord(int fu, int opcode, int d, int s, int t, bool imm)
{
    auto rec = fu_status[fu];
    rec.busy = true;
    rec.op = opcode;
    /* switch(rec.op)
    {
        case add: rec.FuncUnitType = ADD;
                  break;
        case mul: rec.FuncUnitType = MUL;
                  break;
        case div: rec.FuncUnitType = DIV;
                  break;
        case load:
        case store: rec.FuncUnitType = ADDER;
                    break;
    } */
    rec.immediate = imm;
    rec.Fi = d;
    rec.Fj = s;
    rec.Fk = t;
    FloatRegStatus[rec.Fi] = fu;
    rec.Qj = FloatRegStatus[Fj];
    rec.Qk = FloatRegStatus[Fk];
    if(rec.Qj == -1)
        rec.Rj = true;
    else rec.Rj = false;
    if(rec.Qk == -1)
        rec.Rk = true;
    else rec.Rk = false;
    unordered_map<string, int> mapStallsLeft = {
        {ADD, 3},
        {MUL, 5},
        {DIV, 11},
        {LOAD, 2},
        {STORE, 2}
    };
    rec.StallsLeft = mapStallsLeft[rec.op];
}

void scoreboard::resetRecord(int fu)
{
    auto rec = fu_status[fu];
    rec.busy = false;
    rec.Fi = Fj = Fk = -1;
    rec.Qj = Qk = -1;
    rec.Rj = Rk = false;
    rec.StallsLeft = 0;
    rec.immediate = false;
}

bool scoreboard::issue(string op, int d, int s, int t, bool imm) // need to take care of offset shift to diff bw IntRegFile and FloatRegFile
{
    // add 0 mul 1 div 2 load 3 store 4
    unordered_map<string, int> optab = {
        /* {"L.D", 0}, {"S.D", 0}, {"MUL.D", 1}, {"ADD.D", 2}, {"DIV.D", 3}, */
        {"ADD.D", ADD},
        {"MUL.D", MUL},
        {"DIV.D", DIV},
        {"L.D", LOAD},
        {"S.D", STORE}
    };
    bool issued = false; // We need to indicate that the instruction has been issued so that in the execution loop, 
                         // we can move to the next stage

    switch(optab[op])
    {
        case LOAD: 
                if(!fu_status[ADDER].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(ADDER, optab[op], d, s, t, imm);
                    issued = true;
                }    
                break;

        case STORE:
                if(!fu_status[ADDER].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(ADDER, optab[op], d, s, t, imm);
                    issued = true;
                }    
                break;

        case MUL:
                if(!fu_status[MUL1].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(MUL1, optab[op], d, s, t, imm);
                    issued = true;
                }    
                else if(!fu_status[MUL2].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(MUL2, optab[op], d, s, t, imm);
                    issued = true;
                }
                break;

        case ADD:
                if(!fu_status[ADD1].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(ADD1, optab[op], d, s, t, imm);
                    issued = true;
                }
                else if(!fu_status[ADD2].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(ADD2, optab[op], d, s, t, imm);
                    issued = true;
                }
                break;

        case DIV:
                if(!fu_status[DIV1].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), d) == FloatRegStatus.end())
                {
                    setrecord(DIV1, optab[op], d, s, t, imm);
                    issued = true;
                }
                break;
    }
    if(issued)
        rec.InstStatus.push_back(cycle);
    return issued;
}

bool scoreboard::readOperands(int fu) // doesn't need any more arguments because all the data that it needs is present in the status table
{
    auto rec = fu_status[fu];
    if(rec.Rj && rec.Rk)
    {
        // If immediate, load the value itself
        if(rec.immediate)
            rec.Vj.ValI = rec.Fj;
        else
            rec.Vj.ValF = F[rec.Fj];
        // I am addressing from where to load here itself; 
        // so the convention that we are maintaining is 0...15-FPRegister file and 16...47-IntRegisterFile
        if(rec.Fk < 16)
            rec.Vk.ValF = F[rec.Fk];
        else
            rec.Vk.ValI = R[rec.Fk-16];
        rec.InstStatus.push_back(cycle);
        return true;
    }    
    else
        return false;
}

bool scoreboard::execute(int fu)
{
    auto rec;
    rec = fu_status[fu];
    if(StallsLeft != 0)
    {
        StallsLeft -= 1;
        return false;
    }
    else
    {
        if(rec.op == ADD)
            temp = rec.Vj.ValF + rec.Vk.ValF;
        else if(rec.op == MUL)
            temp = rec.Vj.ValF * rec.Vk.ValF;
        else if(rec.op == DIV)
            temp = rec.Vj.ValF / rec.Vk.ValF;
        else if(rec.op == LOAD || rec.op == STORE)
        {
            int effectiveaddress = rec.Vj.ValI + rec.Vk.ValI;
            temp = Memory[effectiveaddress];
        } 
        rec.InstStatus.push_back(cycle);   
        return true;
    }
}

bool scoreboard::writeResult(int fu)
{
    auto rec = fu_status[fu];
    for(int i = 0; i < NUM_FU; i++)
    {
        if(i != fu && (fu_status[i].Fj == rec.Fi || fu_status[i].Fk == rec.Fi))
            return false;
    }
    F[rec.Fi] = temp;
    rec.InstStatus.push_back(cycle);
    return true;
}

void scoreboard::ExecutionLoop()
{
    
}

int main()
{
    
    return 0;
}
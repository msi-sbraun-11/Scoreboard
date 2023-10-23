// simple scoreboard for 6 instructions: primarily for the Floating-Point unit
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <utility>

#include <sstream>

using namespace std;

#define ADD1 0
#define ADD2 1
#define MUL1 2
#define MUL2 3
#define DIV1 4
#define ADDER 5
#define NUMFU 6
#define INTREGFILESIZE 32
#define FLOATREGFILESIZE 16
#define MEMSIZE 512
#define NUM_FPADD 2
#define NUM_FPMUL 2
#define NUM_FPDIV 1
#define NUM_ADDER 1

typedef struct InstFormat
{
    string op_string;
    int d, s, t;
    bool imm;
}InstFormat;

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
    vector<record> fu_status = vector<record>(NUMFU);       // functional unit status
    vector<int> R = vector<int>(INTREGFILESIZE);          // Integer register file FLOATREGFILESIZE...FLOATREGFILESIZE+INTREGFILESIZE-1
    vector<float> F = vector<float>(FLOATREGFILESIZE);      // FP register file 0...FLOATREGFILESIZE-1
    vector<int> FloatRegStatus = vector<int>(FLOATREGFILESIZE, -1);
    vector<int> IntRegStatus = vector<int>(INTREGFILESIZE, -1); // register status
    vector<numeric> Memory = vector<numeric>(MEMSIZE);
    // we are giving register number, not the value as such; so let's make it <int,int>.
    // In the readOperands() and execute() function, we can get the value and pass.
    unordered_map<string, int> optab = {
        /* {"L.D", 0}, {"S.D", 0}, {"MUL.D", 1}, {"ADD.D", 2}, {"DIV.D", 3}, */
        {"ADD.D", ADD},
        {"MUL.D", MUL},
        {"DIV.D", DIV},
        {"L.D", LOAD},
        {"S.D", STORE}
    };
    // FPADD 3 cycles for EX
    // FPMUL 5 cycles for EX
    // FPDIV 11 cycles for EX
    // ADDER 1 cycle for calculating effective address; 1 cycle for reading from/writing to memory
    // therefore 2 cycles for EX
    unordered_map<int, int> mapStallsLeft = {
        {ADD, 3},
        {MUL, 5},
        {DIV, 11},
        {LOAD, 2},
        {STORE, 2}
    };
    void setrecord(int, int, int, int, int, bool);
    void resetRecord(int);
    string ReadInstruction(ifstream&);
    InstFormat Parse(string);
    public:
        scoreboard()
        {
            cycle = 0;
            //Initialising memory
            Memory[110].ValF = 4.0;
            Memory[106].ValF = 7.0;
            //Initialise integer register file
            R.at(2) = 100;
            R.at(3) = 101;
        }
        bool issue(InstFormat);
        bool readOperands(int);
        bool execute(int);
        bool writeResult(int);
        void ExecutionLoop(ifstream&);
};

void scoreboard::setrecord(int fu, int opcode, int d, int s, int t, bool imm)
{
    auto &rec = fu_status[fu];
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
    rec.Qj = FloatRegStatus[rec.Fj];
    if(rec.Fk < FLOATREGFILESIZE)
        rec.Qk = FloatRegStatus[rec.Fk];
    else
        rec.Qk = IntRegStatus[rec.Fk-FLOATREGFILESIZE];
    cout<<"Inside setrecord() "<<rec.Fj<<" "<<rec.Fk<<" "<<rec.Qj<<" "<<rec.Qk<<endl;
    if(rec.Qj == -1)
        rec.Rj = true;
    else rec.Rj = false;
    if(rec.Qk == -1)
        rec.Rk = true;
    else rec.Rk = false;
    rec.StallsLeft = mapStallsLeft[rec.op];
}

void scoreboard::resetRecord(int fu)
{
    auto &rec = fu_status[fu];
    rec.busy = false;
    rec.Fi = rec.Fj = rec.Fk = -1;
    rec.Qj = rec.Qk = -1;
    rec.Rj = rec.Rk = false;
    rec.StallsLeft = 0;
    rec.immediate = false;
    FloatRegStatus[rec.Fi] = -1;
}

InstFormat scoreboard::Parse(string s)
{
    InstFormat T; string word; vector<string> v4;
    stringstream ss(s);
    while(ss >> word)
        v4.push_back(word);

    T.op_string = v4.at(0);
    
    int x = stoi(v4.at(1).substr(1));
    T.d = x;
    
    if(v4.at(2)[0] == '+')
        T.imm = true;
    else
        T.imm = false;
    T.s = stoi(v4.at(2).substr(1));
    
    T.t = stoi(v4.at(3).substr(1));
    if(v4.at(3)[0] == 'R')    
        T.t += FLOATREGFILESIZE;

    return T;
}

string scoreboard::ReadInstruction(ifstream &ifile)
{
    string line;
    getline(ifile, line);
    return line;
}


bool scoreboard::issue(InstFormat IFormat)
        /* string op, int d, int s, int t, bool imm */ 
// need to take care of offset shift to diff bw IntRegFile and FloatRegFile
{
    // add 0 mul 1 div 2 load 3 store 4
    bool issued = false; // We need to indicate that the instruction has been issued so that in the execution loop, 
                         // we can move to the next stage
    switch(optab[IFormat.op_string])
    {
        case LOAD: 
                if(!fu_status[ADDER].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), IFormat.d) == FloatRegStatus.end())
                {
                    setrecord(ADDER, optab[IFormat.op_string], IFormat.d, IFormat.s, IFormat.t, IFormat.imm);
                    issued = true;
                    fu_status[ADDER].InstStatus.push_back(cycle);
                }    
                break;

        case STORE:
                if(!fu_status[ADDER].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), IFormat.d) == FloatRegStatus.end())
                {
                    setrecord(ADDER, optab[IFormat.op_string], IFormat.d, IFormat.s, IFormat.t, IFormat.imm);
                    issued = true;
                    fu_status[ADDER].InstStatus.push_back(cycle);
                }    
                break;

        case MUL:
                if(!fu_status[MUL1].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), IFormat.d) == FloatRegStatus.end())
                {
                    setrecord(MUL1, optab[IFormat.op_string], IFormat.d, IFormat.s, IFormat.t, IFormat.imm);
                    issued = true;
                    fu_status[ADD1].InstStatus.push_back(cycle);
                }    
                else if(!fu_status[MUL2].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), IFormat.d) == FloatRegStatus.end())
                {
                    setrecord(MUL2, optab[IFormat.op_string], IFormat.d, IFormat.s, IFormat.t, IFormat.imm);
                    issued = true;
                    fu_status[MUL2].InstStatus.push_back(cycle);
                }
                break;

        case ADD:
                if(!fu_status[ADD1].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), IFormat.d) == FloatRegStatus.end())
                {
                    setrecord(ADD1, optab[IFormat.op_string], IFormat.d, IFormat.s, IFormat.t, IFormat.imm);
                    issued = true;
                    fu_status[ADD1].InstStatus.push_back(cycle);
                }
                else if(!fu_status[ADD2].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), IFormat.d) == FloatRegStatus.end())
                {
                    setrecord(ADD2, optab[IFormat.op_string], IFormat.d, IFormat.s, IFormat.t, IFormat.imm);
                    issued = true;
                    fu_status[ADD2].InstStatus.push_back(cycle);
                }
                break;

        case DIV:
                if(!fu_status[DIV1].busy &&
                    find(FloatRegStatus.begin(), FloatRegStatus.end(), IFormat.d) == FloatRegStatus.end())
                {
                    setrecord(DIV1, optab[IFormat.op_string], IFormat.d, IFormat.s, IFormat.t, IFormat.imm);
                    issued = true;
                    fu_status[DIV1].InstStatus.push_back(cycle);
                }
                break;
    }    
    if(issued)    
    cout<<optab[IFormat.op_string]<<" "<<cycle<<" "<<"Issue stage done"<<endl;
    return issued;
}

bool scoreboard::readOperands(int fu) // doesn't need any more arguments because all the data that it needs is present in the status table
{
    cout<<fu<<" "<<cycle<<" "<<"readOperands"<<endl;
    auto &rec = fu_status[fu];
    if(rec.Rj && rec.Rk)
    {
        // If immediate, load the value itself
        if(rec.immediate)
            rec.Vj.ValI = rec.Fj;
        else
            rec.Vj.ValF = F[rec.Fj];
        // I am addressing from where to load here itself; 
        // so the convention that we are maintaining is 0...15-FPRegister file and 16...47-IntRegisterFile
        if(rec.Fk < FLOATREGFILESIZE)
            rec.Vk.ValF = F[rec.Fk];
        else
            rec.Vk.ValI = R[rec.Fk-FLOATREGFILESIZE];
        rec.InstStatus.push_back(cycle);
        cout<<fu<<" "<<cycle<<" "<<"readOperands done"<<endl;
        return true;
    }    
    else
        return false;
}

bool scoreboard::execute(int fu)
{
    cout<<fu<<" "<<cycle<<" "<<"execute stage"<<endl;
    auto &rec = fu_status[fu];
    if(rec.StallsLeft)
    {
        rec.StallsLeft -= 1;
        return false;
    }
    else
    {
        if(rec.op == ADD)
            rec.temp.ValF = rec.Vj.ValF + rec.Vk.ValF;
        else if(rec.op == MUL)
            rec.temp.ValF = rec.Vj.ValF * rec.Vk.ValF;
        else if(rec.op == DIV)
            rec.temp.ValF = rec.Vj.ValF / rec.Vk.ValF;
        else if(rec.op == LOAD)
        {
            int effectiveaddress = rec.Vj.ValI + rec.Vk.ValI;
            rec.temp = Memory[effectiveaddress];
        } 
        else if(rec.op == STORE)
        {
            int effectiveaddress = rec.Vj.ValI + rec.Vk.ValI;
            Memory[effectiveaddress] = rec.temp;
        }
        rec.InstStatus.push_back(cycle);   
        cout<<fu<<" "<<cycle<<" "<<"execute stage done"<<endl;
        return true;
    }
}

bool scoreboard::writeResult(int fu)
{
    cout<<fu<<" "<<cycle<<" "<<"writeResult"<<endl;
    auto &rec = fu_status[fu];
    // record *temprec;
    F[rec.Fi] = rec.temp.ValF;
    FloatRegStatus[rec.Fi] = -1;
    // Eliminate dependencies of other instructions
    for(int i = 0; i < NUMFU; i++)
    {
        if(i != fu)
        {
            auto &temprec = fu_status[i];
            if(temprec.Fj == rec.Fi)
            {
                temprec.Qj = -1;
                temprec.Rj = true;
            }
            if(temprec.Fk == rec.Fi)
            {
                temprec.Qk = -1;
                temprec.Rk = true;
            }             
        }
    }
    rec.InstStatus.push_back(cycle);
    // some writing should happen here
    //resetRecord(fu);
    cout<<fu<<" "<<cycle<<" "<<"writeResult done"<<endl;
    return true;
}

void scoreboard::ExecutionLoop(ifstream &ifs)
{
    int fu, l; bool ReadNext = true, ph; string inst;
    InstFormat tempstruct;
    for(cycle = 0; cycle < 10/* not ifs.eof() */; cycle++)
    {
        if(ReadNext)
        {
            inst = ReadInstruction(ifs);
            cout<<cycle<<" "<<inst<<endl;
            tempstruct = Parse(inst);
        }
        for(fu = 0; fu < NUMFU; fu++)
        {
            l = fu_status[fu].InstStatus.size();
            switch(l)
            {
                case 0: ReadNext = issue(tempstruct);
                        if(ReadNext) cout<<"Issued"<<endl;
                        break;
                case 1: ph = readOperands(fu); 
                        if(ph) cout<<"READ Operands"<<endl;
                        break;
                case 2: ph = execute(fu); 
                        if(ph) cout<<"executed"<<endl;
                        break;
                case 3: ph = writeResult(fu); 
                        if(ph) cout<<"written result"<<endl;
                        break;
            }
        }        
    }
}

int main()
{
    scoreboard SCOREBOARD;
    ifstream ifs;
    ifs.open("instructions.txt", ifstream::in);
    SCOREBOARD.ExecutionLoop(ifs);
    
    return 0;
}
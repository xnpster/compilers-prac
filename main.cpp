#include<iostream>

#ifdef __cplusplus
    #define export exports
    extern "C" {
        #include "qbe/all.h"
    }
    #undef export
#else
    #include <qbe/all.h>
#endif

#include <cstdio>

using namespace std;

Target T;

char debug['Z'+1];

extern "C" void dbgfile(char*);
extern "C" void dataHandler(Dat*);
extern "C" void funHandler(Fn*);

static void init() {
    debug['A'] = 0; /* abi lowering */
    debug['C'] = 0; /* copy elimination */
    debug['F'] = 0; /* constant folding */
    debug['I'] = 0; /* instruction selection */
    debug['M'] = 0; /* memory optimization */
    debug['N'] = 0; /* ssa construction */
    debug['L'] = 0; /* liveness */
    debug['P'] = 0; /* parsing */
    debug['R'] = 0; /* reg. allocation */
    debug['S'] = 0; /* spilling */
}

void dataHandler(Dat* data) {
    cout << "Data handler" << endl;
}

void funHandler(Fn* fun) {
    cout << "Handler for function " << fun->name << endl;

    fillrpo(fun);
    
    for (Blk* b=fun->start; b; b=b->link) {
        cout << "Block " << b->id << " " << b->name << endl;

        int instr_num = b->nins;

        for(int i = 0; i < instr_num; i++) {
            Ins* instr = b->ins + i;

            cout << "Op:" << instr->op << endl;
        }
    }
    
}

void dbgfile(char* str) {
    cout << str << endl;
}

int main(int argc, char** argv) {
    cout << "Running..." << endl;

    if(argc <= 1) {
        cout << "usage: run [filename]" << endl;
        return 0;
    }

    init();
    char* input_file_name = argv[1];

    FILE* input_file = fopen(input_file_name, "r");

    if(!input_file) {
        cout << "input file " << input_file_name << " not exist" << endl;
        return 0;
    }

    parse(input_file, input_file_name, dbgfile, dataHandler, funHandler);

    return 0;
}
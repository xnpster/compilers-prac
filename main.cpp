#include "qbelib.h"
#include "Utils/macro.h"
#include "LoopNode/loop_node.h"
#include "LICM/licm.h"

#include<iostream>

#include <cstdio>

using namespace std;

Target T;

char debug['Z'+1];

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

static void dataHandler(Dat* dat) {
    (void) dat;
}

static void funHandler(Fn* fn) {
    fillrpo(fn); // Traverses the CFG in reverse post-order, filling blk->id.
    fillpreds(fn);
    filluse(fn);
    ssa(fn);
    #if DEBUG
        printfn(fn, stdout);
    #endif
    auto loops = getLoops(fn);
    runLoopInvariantCodeMotion(loops, fn);

    #if DEBUG
        for(int i = 0; i < fn->nblk; i++) {
            Blk* b = fn->rpo[i];
            cout << b->id << " " << b->name << endl;
        }
    #endif

    printfn(fn, stdout);
}

static void dbgfile(char* str) {
    cout << str << endl;
}

int main(int argc, char** argv) {
#if DEBUG
    cout << "Running..." << endl;
#endif

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
    freeall();

    return 0;
}
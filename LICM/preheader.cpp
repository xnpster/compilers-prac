#include "preheader.h"
#include "../Utils/macro.h"
#include <set>
#include <map>

#if DEBUG
#   include <iostream>
#endif

#include <string>

using namespace std;

static const string prefix = "prehead@";

static bool insert_block_to_func(Fn* fn, Blk* before, Blk* curr) {
    for (auto b=fn->start; b; b=b->link) {
        if(b->link == before) {
            b->link = curr;
            curr->link = before;

            return true;
        }
    }

    return false;
}

static void create_preheader(shared_ptr<LoopNode> loop, Fn* fn, 
                             map<Blk*, Blk*>& created_headers, 
                             map<Blk*, int>& prehead_idx)
{
    #if DEBUG
        cout << "add preheader to " <<  loop->header->name << endl;
    #endif

    set<Blk*> outer_preds;
    Blk** preds = loop->header->pred; 

    for(int i = 0; i < (loop->header->npred); i++)
        if(loop->blocks.find(preds[i]) == loop->blocks.end())
            outer_preds.insert(preds[i]);

    if(!loop->invariants.empty() && !outer_preds.empty()) {
        if (outer_preds.size() == 1 && false) { // TODO: remove
            // Если у хедера только один предок - он становится прехедером
            loop->preheader = *(outer_preds.begin());
            prehead_idx[loop->preheader] = loop->preheader->nins;
        } else if(created_headers.find(loop->header) != created_headers.end()) {
            loop->preheader = created_headers[loop->header];
            prehead_idx[loop->preheader] = 0;
        } else {
            // Новый прехеддер
            Blk* prehead = newblk();
            insert_block_to_func(fn, loop->header, prehead);

            loop->preheader = prehead;
            created_headers[loop->header] = prehead;
            prehead_idx[loop->preheader] = 0;
            // Прехеддер - новый предок хедера
            string newname = string(prefix + string(loop->header->name));
            strncpy(prehead->name, newname.c_str(), newname.size() + 1);
            prehead->nins = 0;
            prehead->s1 = loop->header;
            prehead->s2 = NULL;

            prehead->jmp.type = Jjmp;

            // В цикле у всех предков хедера потомком делаем прехедер вместо хедера
            for(auto pred : outer_preds) {
                if(pred->s1 == loop->header)
                    pred->s1 = prehead;

                if(pred->s2 == loop->header)
                    pred->s2 = prehead;
            }
        }
        // Alloc ins

        int result_size = loop->preheader->nins + loop->invariants.size();
        Ins* ins_mem = (Ins*)malloc(result_size * sizeof(Ins));

        if(loop->preheader->nins != 0) {
            memcpy(ins_mem, loop->preheader->ins, loop->preheader->nins * sizeof(Ins));
            //free(loop->preheader->ins);
        }

        loop->preheader->ins = ins_mem;
        loop->preheader->nins = result_size;
    }

    for(auto descend: loop->nested) {
        // Создаём прехедер для вложенных циклов
        create_preheader(descend, fn, created_headers, prehead_idx);
        
        // Добавляем прехедеры вложенных циклов в множество блоков текущего цикла
        if(descend->preheader)
            loop->blocks.insert(descend->preheader);
    }
}

void add_preheaders(set<shared_ptr<LoopNode>> loops, Fn* func, map<Blk*, int>& prehead_idx) {
    map<Blk*, Blk*> created_headers;

    for(auto loop: loops) {
        create_preheader(loop, func, created_headers, prehead_idx);
    }

    fillpreds(func);
}

static void fill_preheader(shared_ptr<LoopNode> loop, Fn* func, map<Blk*, int>& prehead_idx) {
    // Added ins
    
    if(loop->preheader) {
        int& counter = prehead_idx[loop->preheader];
        
        #if DEBUG
            cout << "fill preheader " <<  loop->preheader->name << endl;
        #endif

        for (const auto& invariant: loop->invariants) {
            #if DEBUG
                cout << "inv ";
                printref(invariant->real, func, stdout);
                cout << endl;
            #endif

            for (auto& block: loop->blocks) {
                bool added = false;
                for (int i = 0; i < block->nins; i++) {
                    if (invariant != makeComparable(block->ins[i].to)) {
                        continue;
                    }
                    
                    // Move ins from block to preheader
                    loop->preheader->ins[counter++] = block->ins[i];

                    #if DEBUG
                        cout << loop->preheader->name << "[" << counter - 1 << "/" << loop->preheader->nins - 1 << "] = ";
                        printref(block->ins[i].to, func, stdout);
                        cout << endl;
                    #endif

                    for(i++; i < block->nins; i++) {
                        block->ins[i - 1] = block->ins[i];
                    }
                    block->nins--;
                    added = true;
                    break;
                }
                if (added) {
                    break;
                }
            }
        }

        loop->preheader->nins = counter;
    }

    for (auto& nst: loop->nested) {
        fill_preheader(nst, func, prehead_idx);
    }
    
    return;
}

void fill_preheaders(set<shared_ptr<LoopNode>> loops, Fn* func, map<Blk*, int>& prehead_idx) {
    for (auto& loop: loops) {
        fill_preheader(loop, func, prehead_idx);
    }
    return;
}

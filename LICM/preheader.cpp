#include "preheader.h"

#include <iostream>
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

static void create_preheader(shared_ptr<LoopNode> loop, Fn* fn) {
    for(auto descend: loop->nested) {
        // Создаём прехедер для вложенных циклов
        create_preheader(descend, fn);
        
        // Добавляем прехедеры вложенных циклов в множество блоков текущего цикла
        loop->blocks.insert(descend->preheader);
    }

    if(!loop->invariants.empty()) {
        if (loop->header->npred == 1) {
            // Если у хедера только один предок - он становится прехедером
            loop->preheader == *(loop->header->pred);
            return;
        }

        // Новый прехеддер
        Blk* prehead = newblk();
        insert_block_to_func(fn, loop->header, prehead);

        loop->preheader = prehead;
        // Прехеддер - новый предок хедера
        string newname = string(prefix + string(loop->header->name));
        strncpy(prehead->name, newname.c_str(), newname.size() + 1);
        prehead->s1 = prehead->s2 = loop->header;

        prehead->jmp.type = Jjmp;

        Blk** preds = loop->header->pred; 

        // В цикле у всех предков хедера потомком делаем прехедер вместо хедера
        for(int i = 0; i < (loop->header->npred); i++) {
            if((preds[i]->s1) == loop->header) {
                preds[i]->s1 = prehead;
            }

            if((preds[i]->s2) == loop->header) {
                preds[i]->s2 = prehead;
            }
       }
       // Alloc ins
       loop->preheader->nins = loop->invariants.size();
       loop->preheader->ins = (Ins*)malloc(loop->preheader->nins * sizeof(Ins));
    }
}

void add_preheaders(set<shared_ptr<LoopNode>> loops, Fn* func) {
    for(auto loop: loops) {
        create_preheader(loop, func);
    }

    fillpreds(func);
}

static void fill_preheader(shared_ptr<LoopNode> loop, Fn* func) {
    // Added ins
    int counter = 0;
    // cout << endl << loop->preheader->name << endl;
    for (const auto& invariant: loop->invariants) {
        for (auto& block: loop->blocks) {
            // cout << "Blk, ya! " << block->name << endl;
            bool added = false;
            for (int i = 0; i < block->nins; i++) {
                if (invariant != makeComparable(block->ins[i].to)) {
                    continue;
                }
                // cout << "FIND: " << block->name << " " << i << endl;
                // Move ins from block to preheader
                // cout << loop->preheader->nins << endl;
                loop->preheader->ins[counter++] = block->ins[i];
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
    cout << endl;
    return;
}

void fill_preheaders(set<shared_ptr<LoopNode>> loops, Fn* func) {
    for (auto& loop: loops) {
        fill_preheader(loop, func);
    }
    return;
}

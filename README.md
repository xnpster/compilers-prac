# LICM (Loop invariant code motion)

## Build

Just run ```make```

## Run

After building the project, ```run``` executable will appear. To process IR file ```input.ssa``` you should run ```./run input.ssa```.

# Task

## Перемещение кода, инвариантного относительно цикла
Реализуйте алгоритм перемещения участков кода, инвариантных относительно цикла, для функций на языке промежуточного представления QBE.

Инструкция инвариантна относительно цикла, если она удовлетворяет одному из следующих условий: ее операнды — константы; все определения операндов, достигающие инструкции находятся вне цикла; внутри цикла имеется в точности одно определение операнда, но оно само инвариантно относительно цикла. Такие инструкции должны быть исключены из цикла и перенесены в его предзаголовок. Вызовы функций и инструкции, обращающиеся к памяти, не считаются инвариантными.

## Input format
Решение должно считывать со стандартного потока ввода текстовый файл, содержащий ровно одну функцию на языке промежуточного представления QBE, а на стандартный поток вывода выдавать её же промежуточное представление после после преобразования к SSA форме (с помощью libqbe, см. заготовку решения) и преобразования участков с циклами. Если в цикле присутствуют инструкции, инвариантные относительно цикла, необходимо перед заголовком цикла добавить новый заголовок и перенести в него все такие инструкции.

## Заготовка решения
```
#ifdef __cplusplus
#define export exports
extern "C" {
#include <qbe/all.h>
}
#undef export
#else
#include <qbe/all.h>
#endif

#include <stdio.h>

static void readfn (Fn *fn) {
    fillrpo(fn); // Traverses the CFG in reverse post-order, filling blk->id.
    fillpreds(fn);
    filluse(fn);
    ssa(fn);

    ...

    printfn(fn, stdout);
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}
```

## Examples
### Input
```
export function w $foo() {
@start
    %x =w copy 1
@d0
    %y =w copy %x
@d1
    %z =w add %x, %y
    jnz %z, @d0, @end
@end
    ret
}
```
### Output
```
export function $foo() {
@start
    %x.1 =w copy 1
@prehead@d0
    %y.2 =w copy %x.1
    %z =w add %x.1, %y.2
@d0
@d1
    jnz %z, @d0, @end
@end
    ret0
}
```
## Notes
Запрещается использовать функции loopiter и fillloop, а также копировать их исходный код.
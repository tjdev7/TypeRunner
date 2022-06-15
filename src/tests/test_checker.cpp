#include <gtest/gtest.h>

#include "../parser2.h"
#include "../checker/compiler.h"
#include "../checker/vm.h"
#include "../checker/debug.h"

using namespace ts;

string compile(string code) {
    Parser parser;
    auto result = parser.parseSourceFile("app.ts", code, ScriptTarget::Latest, false, ScriptKind::TS, {});
    checker::Compiler compiler;
    auto program = compiler.compileSourceFile(result);
    auto bin = program.build();
    checker::printBin(bin);
    return bin;
}

vector<string> run(string code) {
    vm::VM vm;
    vm.run(compile(code));
    vm.printErrors();
    return vm.errors;
}

TEST(checker, program) {
    checker::Program program;

    program.pushOp(instructions::OP::Call);
    program.pushAddress(1);

    program.pushOp(instructions::OP::Var);
    program.pushAddress(2);

    program.pushOp(instructions::OP::Any);
    program.pushOp(instructions::OP::Assign);
    program.pushAddress(2);

    fmt::print("bytes {}", program.ops);
}

TEST(checker, type) {
    Parser parser;

    string code = R"(
    const v1: string = "abc";
    const v2: number = 123;
    )";

    auto bin = compile(code);

    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
    EXPECT_EQ(vm.errors.size(), 0);

    bench(10, [&] {
        vm::VM vm;
        vm.run(bin);
    });
}

TEST(checker, type2) {
    Parser parser;

    string code = R"(
    type a = number;
    type b = string | a;
    const v1: b = 'abc';
    const v2: b = 23;
    const v3: b = true;
    )";

    auto bin = compile(code);

    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
    EXPECT_EQ(vm.errors.size(), 1);
}

TEST(checker, type31) {
    string code = R"(
    type a<K, T> = T;
    const v1: a<true, number> = 34;
    const v2: a<true, number> = "as";
    )";

    auto bin = compile(code);

    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
    EXPECT_EQ(vm.errors.size(), 1);
    bench(10, [&] {
        vm::VM vm;
        vm.run(bin);
    });
}

TEST(checker, type3) {
    Parser parser;

    string code = R"(
    type a<K, T> = K | (T extends string ? 'yes' : 'no');
    const v1: a<true, number> = 'no';
    const v2: a<true, string> = 'yes';
    const v3: a<true, string> = true;
    const v4: a<true, string | number> = 'yes';
    const v5: a<true, string> = 'nope';
    )";

    auto bin = compile(code);

    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
    EXPECT_EQ(vm.errors.size(), 1);

    bench(10, [&] {
        vm::VM vm;
        vm.run(bin);
    });
}

TEST(checker, tuple) {
    Parser parser;

    string code = R"(
//    type a<T> = [1, ...T];
//    const v1: a<[string, number]> = [1, 'abc'];

//    type a = [string, number];
//    const var1: a['length'] = 3;
//
//    type a = [string, boolean];
//    type b = [...a, number];
//    const var1: b = ['asd', 123];

//    type a<T = string> = T;
//    const var1: a<number> = 'asd';

    type StringToNum<T extends string, A extends 0[] = []> = `${A['length']}` extends T ? A['length'] : StringToNum<T, [...A, 0]>;
    const var1: StringToNum<'999'> = 1002;
    )";

    auto bin = compile(code);

    vm::VM vm;
    vm.run(bin);
    vm.printErrors();
//    EXPECT_EQ(vm.errors.size(), 1);

    bench(10, [&] {
        vm::VM vm;
        vm.run(bin);
    });
}

TEST(checker, assign) {
    Parser parser;

    auto code = R"(
        const v: number | string = '';
        //Assign to '', change to string

        v = 123; //Assign to 123, change to number
    )";

    auto result = parser.parseSourceFile("app.ts", code, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});

    checker::Compiler compiler;

    auto program = compiler.compileSourceFile(result);
    auto bin = program.build();
    checker::printBin(bin);
    debug("done");
}

TEST(checker, stackFrame) {
    Parser parser;

    auto code = R"(
    type a = string;

    //each stack frame gets a number. The ref `a` references this stack number + symbol index.
    //when resolving in VM it searches frames upwards until correct frame was found.
    type Generic<T> = a | T;

    function another() {
        type a = ReturnType<typeof print>;
    }

    function print() {
        type b = a;
        type c = Generic<string>;

        function p() {
            type d = Generic<string>;
        }
        return c;
    }
    )";

    auto result = parser.parseSourceFile("app.ts", code, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
    debug("done");
}

TEST(checker, basic) {
    Parser parser;

    auto code = R"(
    const i: number = 123;

    function print(v: string) {
    }

    print(i);
    )";

    compile(code);
}

TEST(checker, basic2) {
    Parser parser;

    auto code = R"(
//    i = 1;
    const i: number | string = 123;

    if ('number' === typeof i) {
       function print(v: string) {
            i = 3;
            const i = v;
        }
    }
    //type print = (v: string) => void;
    //const print = function(v: string) {} //both equivalent, except of the symbol scope

    i = "no";
    print(i);
    )";

    auto result = parser.parseSourceFile("app.ts", code, ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
    debug("done");
}

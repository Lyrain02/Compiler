# Compiler
Lexical Analyzer, LL parser, SLR parser and Translation Schemma.

## Structure

There are 4 directories, LexicalAnalyzer, LLparser, LRparser and TranslationSchemma.

Take LLParser as an example:

```
|-- LLparser
  |-- LLparser.h    
  |-- LLparserMain.cpp
  |-- testcases    
    |-- prog1.txt   
    |-- prog2.txt
  |-- spec.md      //requirement doc
  |-- readMe.md    //design doc
```

## Run & Test

Take Lexical Analyzer for an example to help you run and test the codes.

Goto Compiler/LexicalAnalyzer: 

**Compile**

Use gcc to compile.
```
gcc C_LexAnalysis_mainProcess.cpp -lstdc++ -o main
```

**Run**

 Use testcases/prog1.txt as the input file.
```
./main < testcases/prog1.txt  
```

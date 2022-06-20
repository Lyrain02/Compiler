# LL Parser

## 亮点

1. 定义了Production, Token, Error结构体
2. 使用vector存储产生式、错误集合，利用set存储终结符、非终结符；
利用unordered_map<string, set<string> >存储每个symbol对应的first/follow集合；利用unordered_map< string, unordered_map <string, Production> >存储转换表内容。
3. 自动分析grammar，生成产生式、终结符、非终结符。
4. 错误处理部分考虑了非法输入、缺少输入、匹配错误、空栈、非空栈等情况；支持处理任何输入程序。
5. 利用#define TEST，可以处理测试版grammar，注释掉#define OJ，可以看标准错误输出。
6. 代码结构清晰，详见grammar定义、结构体定义、函数声明、全局变量、函数实现部分。
  
## 整体设计
  
### Flow
1. 构造parsing table
    1. 预处理
  
        处理grammar，生成productions, terminals和nonTerminals集合
  
       （默认grammar已经消除了左因子、左递归）
  
    2. 计算First、Follow集合
  
        计算所有symbol(terminals,nonTerminals)的First集合。
  
        计算所有nonTerminals的Follow集合。
    3. 构造parsing table
  
        table[nonTerminal][termnial]的值为产生式。
  
2. LL1分析待处理程序
    1. 初始化stack、input
  
        将START_SYMBOL压栈.
  
        处理带分析程序的tokens，记录每个token的值和line，最后放入$.
  
    2. LL(1)分析，得到分析过程使用的产生式序列
  
        包含错误处理过程
  
    3. 错误输出
  
        输出分析过程中发生的所有错误
  
    4. 标准化输出
  
        每个单词占据一行；处于语法树同一高度的单词，具有相同的缩进格数。
  
        输出格式为'\t'*height +  symbol
### 数据结构
- 结构体定义
  - Production 产生式
  
    用string存储产生式左侧内容，用vector<string>顺序存储产生式右侧内容
  - Token 单词
  
    待分析程序中的单词，包含内容和行号；标准化输出中的单词，包含内容和所处的语法树高度。
  - Error 错误
  
    包含错误id，错误信息msg，和错误在带分析程序中的位置line。
- 全局变量
  - productions 语法生成的全体产生式
  
      利用vector<Production>存储全体语法包含的产生式。
  - terminals, nonTerminals  终结符/非终结符集合
  
      利用set<string> 存储终结符/非终结符集合
  - first, follow first/follow集合
  
      利用unordered_map<string, set<string> >存储first和follow集合。
  
      利用symbol即可查询得到其对应的first/follow集合。
  
      unorderedmap的查询平均速度为O(1).
  - table 转换表
  
      利用unordered_mao<string, unordered_map<string, Production> >存储table，平均查询速度为O(1)。
  
      直接利用table[nonTerminal][terminal]访问其对应的产生式。
  - inputs, myStack 输入和分析栈
  
      利用vector<Token> 存储带分析的inputs集合。
  
      利用stack<string>存储当前的分析栈。
  - errors, outputs
  
      利用vector<Error>存储分析过程中发生的所有错误。
  
      利用vector<Production>记录分析过程中使用的产生式序列。
### 错误处理
- 匹配错误，parsing table缺乏对应值
  
    栈顶symbol和当前input值不同，并且parsing table中没有对应的production rule。
  
  1. 非法输入  invalid input 
  
      Parsing table中没有任何nonterminal可以产生对应的input。
  
      Grammar的terminal中不包含对应的input值。
  
      input属于terminal，但是没有non terminal可以直接转换生成input。
  
      实质：
  
      Parsing table中并没有input对应的列。
  
      错误处理：
  
      skip对应的input
  
  2. 缺少输入 no expected input
  
      栈顶symbol为terminal，并且与input不相同。
  
      实质：
  
      Parsing table中没有symbol对应的行；程序缺少相应的输入。
  
      错误处理：
  
      pop栈顶元素。
  
  3. 匹配错误  syntax error
  
      栈顶元素symbol为nonTermianl，input为terminal，parsing table中缺乏对应值。
  
      table[symbol][input]为空。
  
      错误处理：
  
      skip对应的symbol.
  
      把symbol->E添加到outputs中。
  
- stack/inputs其中一个为空
  
  4. 空栈错误    empty stack
  
      栈空了，但是inputs没处理完
  
      错误处理：
  
      在parsing table中寻找任意一项可以生成当前input的产生式，压栈。
  
      把E->symbol添加到outputs中。
  
  5. 非空栈错误   remaining stack
  
      inputs分析完了，但是栈内还有剩余的元素。
  
      错误处理：
  
      清空栈内元素。

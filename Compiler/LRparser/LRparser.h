// C语言SLR词法分析器
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <map>
#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <stack>
#include <algorithm>
#include <queue>
// #define TEST //测试版（可以用其他的grammar测试）
# define OJ//实验平台提交版

#ifdef TEST
	#define WIDE 20
	#define COLS 0
#else
	#define WIDE 45 //table每一个元素占据的长度
	#define COLS 3 //table每一行元素的个数
#endif

#ifdef OJ
	#define ERROR false
#else
	#define ERROR true
#endif

using namespace std;

//-----------------------grammar和start symbol定义---------------------------
#ifdef TEST
const string START_SYMBOL= "S";//增广用的S0
const string NEW_SYMBOL = START_SYMBOL+"0";
// const string START_SYMBOL= "E";//test_output
const string GRAMMAR = 
// "S -> S + T\
// \nS -> T\
// \nT -> T * F\
// \nT -> F\
// \nF -> ( S )\
// \nF -> id";

// "S -> A A\
// \n A -> a A\
// \n A -> b";//a a b a b

// "S -> S + S\
// \n S -> S * S\
// \n S -> id";//id + id + id

// "S -> 2 S 2\
// \n S -> 3 S 3\
// \n S -> 4";//3 2 4 2 3

"S -> ( L )\
\nS -> a\
\nL -> L , S\
\nL -> S"; // ( a , ( a , a ) )


#else
const string START_SYMBOL= "program";//增广用到的program0
const string NEW_SYMBOL = START_SYMBOL+"0";
const string GRAMMAR = 
"program -> compoundstmt\
\nstmt -> ifstmt\
\nstmt -> whilestmt\
\nstmt -> assgstmt\
\nstmt -> compoundstmt\
\ncompoundstmt -> { stmts }\
\nstmts -> stmt stmts\
\nstmts -> E\
\nifstmt -> if ( boolexpr ) then stmt else stmt\
\nwhilestmt -> while ( boolexpr ) stmt\
\nassgstmt -> ID = arithexpr ;\
\nboolexpr -> arithexpr boolop arithexpr\
\nboolop -> <\
\nboolop -> >\
\nboolop -> <=\
\nboolop -> >=\
\nboolop -> ==\
\narithexpr -> multexpr arithexprprime\
\narithexprprime -> + multexpr arithexprprime\
\narithexprprime -> - multexpr arithexprprime\
\narithexprprime -> E\
\nmultexpr -> simpleexpr multexprprime\
\nmultexprprime -> * simpleexpr multexprprime\
\nmultexprprime -> / simpleexpr multexprprime\
\nmultexprprime -> E\
\nsimpleexpr -> ID\
\nsimpleexpr -> NUM\
\nsimpleexpr -> ( arithexpr )";
#endif

//------------------------结构体定义------------------------

//产生式
typedef struct Production{
    string left;
    vector<string> right;
    Production(string l=""){
        left = l;
    }
    bool operator==(const Production &p)const{//vector比较
        return (p.left == left && p.right == right);
    }
    bool operator<(const Production &p)const{//set比较
        if(left < p.left)
            return true;
        else if(left > p.left){
            return false;
        }else{
            return (right<p.right);
        }
        // return (p.left < left && p.right < right);//重载失败，不能正确比较
    }
    bool operator>(const Production &p)const{//set比较
        if(left > p.left){
            return true;
        }else if(left < p.left){
            return false;
        }else{
            return (right > p.right);
        }
    }
}Production;

// 待分析程序中的Token
typedef struct Token{
	string token;
	int line;//待分析程序的行号；输出token所处的语法树的高度（和输出产生式序号不完全相同）
	Token(string t="", int num=-1){
		token = t;
		line = num;
	}
}Token;

//LR分析过程中的Item
typedef struct Item{
    Production production;
    int index;
    Item(Production p, int i = -1){
        production = p;
        index = i;
    }
    bool operator==(const Item &i)const{//vector比较
        return (i.production == production && i.index == index);
    }
    bool operator<(const Item &i)const{//set比较
        if(production < i.production)
            return true;
        else if(production > i.production){
            return false;
        }else{
            return (index<i.index);
        }
    }
}Item;

typedef struct Next{
    string symbol;
    int stateId;
    Next(string s="", int n=-1){
        symbol = s;
        stateId = n;
    }
}Next;

// LR中的一个状态
typedef struct State{
    int id;
    set<Item> items;
    vector<Next> nexts;
	State(int n = -1){
		id = n;
	}
}State;

// parsing table的元素
typedef struct TEntry{
	string type;//有 "shift","reduce","accept","goto"4种；error的时候，type为空
    int stateId;//代表状态的id, shift和goto会使用
    Production production;//reduce使用
	TEntry(string t, int num=-1){//shift, accept, goto
		type = t;
		stateId = num;
	}
    TEntry(string t, Production p){//reduce
		type = t;
		production = p;
	}
    TEntry(){}
}TEntry;

// 分析栈的元素
typedef struct SEntry{
    int stateId;//状态id
    string symbol;//对应的symbol
	SEntry(int num=-1, string str = ""){
        stateId = num;
        symbol = str;
	}
}SEntry;

//错误
typedef struct Error{
	int id; //错误id
	string msg; //错误关键信息
	int line; //发生错误的行号
	Error(int i, int l, string m){//形参不能和字段名称相同
		id = i;
		line = l;
		msg = m;
	}
}Error;

//----------------------函数声明----------------------------

// SLR控制流函数
void Analysis();//LL分析总流程
void init();//分析grammar，提取productions，terminals，nonTerminals
void findFirstAndFollow();//计算整体的Follow和First
void getStates();//利用closure和goto生成states
void getTable();//生成parsing table
void preparation(string prog);//准备inputs和stack
void processLR();//LR分析
void printErrors();//错误分析
void printStandardOutput();//标准化输出

// LR辅助函数
void read_prog(string& prog);//读取程序片段
void getTokens(string prog);//将目标程序拆分成inputs
void removeLeftRecursive();//消除左递归，用来求follow集合（实际上没有用到）
void getFirst(string target);//获取target的First集合
void getFollow(string target);//获取target的Follow集合
bool isE(Production p);//判断产生式是否为空
bool isSetE(set<string> set);//判断集合是否为空
void getClosure(State* state);//计算state的闭包
// bool hasProduction(string target);//判断parsing table中能否生成target
bool isFinished(Item item);//判断Item是否执行完

// 其余辅助函数
vector<string> split(string str,const char split);//字符串分割函数
void printTerminals(set<string> terms);//打印终结符
void printNonTerminals(set<string> nonterms);//打印非终结符
void printSymbols();//打印全体符号
void printProductions(vector<Production> productions);//打印grammar中的所有产生式
void printFirst();//打印First集合
void printFollow();//打印Follow集合
void printState(State* state);//打印单个State
void printStates();//打印所有的states
void printTable();//打印Table，输出所有states对应的symbols的entry
string toStringTEntry(TEntry tEntry);//转换成string
void printTable2();//打印Table，每一行最多COLS个terminals
void printInputs();//打印Inputs
string toStringProduction(Production p);//转换成string
void printProcessState(SEntry sEntry, string input, TEntry tEntry);//打印LR分析过程中的状态

//测试函数
void testStandardOutput();//测试标准输出

//-----------------------全局变量-------------------------
vector<Production> productions;//语法包含的所有产生式

set<string> terminals;//终结符集合
set<string> nonTerminals;//非终结符集合
set<string> symbols;//所有符号集合,包括$

//LR可以直接求具有左递归性质的follow集（follow集的求解和LR不太一样，需要利用集合是否改变，来进行终止判断）
set<string> LRset;//有左递归的NT集合（实际上没用到）
set<string> LRnonTerminals;//非终结符集合（实际上没用到）
vector<Production> productionsNLR;//没有左递归的产生式，用来计算first/follow（实际上没有用到）

bool change;//用来判断follow集合是否在改变
unordered_map<string,set<string> > first;//First map；key为symbol，value为对应的first集合
unordered_map<string,set<string> > follow;//Follow map；key为symbol，value为对应的follow集合
vector<State*> states;//所有状态的集合

unordered_map<int, unordered_map<string, TEntry> > table;//parsing table; table[nonTerminal][Terminal]对应值为产生式。

vector<Token> inputs;//待分析程序的token集合，以及"$"
stack<SEntry> myStack;//分析栈

vector<Error> errors;//程序所有的错误
vector<Production> outputs;//程序分析结果（产生式序列）

//-----------------------函数实现---------------------------

//标准输入，读取待分析程序
void read_prog(string& prog)
{
    char c;
    while(scanf("%c",&c)!=EOF){
        prog += c;
    }
}


//分析grammar，提取productions，terminals，nonTerminals
void init(){
	istringstream iss(GRAMMAR);	// 输入流
	string line;
	while(getline(iss,line)){
		vector<string> words = split(line, ' ');
		Production pro;
		pro.left = words[0];
		nonTerminals.insert(words[0]);
		for(int i=2;i<words.size();i++){
			terminals.insert(words[i]);
			pro.right.push_back(words[i]);
		}
		productions.push_back(pro);
	}

	//删除terminals里面的non terminals
	for(auto word:nonTerminals){
		terminals.erase(word);
	}
	terminals.insert("$");

    //计算所有symbols，包括$
    for(string t:terminals){
        symbols.insert(t);
    }
    symbols.insert("$");
    for(string t:nonTerminals){
        symbols.insert(t);
    }

	//打印
	// printTerminals(terminals);
	// printNonTerminals(nonTerminals);
	// printProductions(productions);
    // printSymbols();
}

//打印全体终结符
void printTerminals(set<string> terms){
	cout<<"terminals: ";
	for(auto s:terms){
		cout<<s<<" ";
	}
	cout<<endl;
}
//打印全体非终结符
void printNonTerminals(set<string> nonterms){
	cout<<"Non terminals: ";
	for(auto s:nonterms){
		cout<<s<<" ";
	}
	cout<<endl;
}
//打印全体符号
void printSymbols(){
	cout<<"Symbols: ";
	for(auto s:symbols){
		cout<<s<<" ";
	}
	cout<<endl;
}
//打印grammar包含的产生式
void printProductions(vector<Production> productions){
	cout<<"productions: "<<endl;
	for(auto p:productions){
		cout<<p.left<<" -> ";
		for(auto w:p.right){
			cout<<w<<" ";
		}
		cout<<endl;
	}
}
//字符串分割函数
vector<string> split(string str,const char split)
{
	istringstream iss(str);	// 输入流
	string token;			// 接收缓冲区
    vector<string> vec;
	while (getline(iss, token, split))	// 以split为分隔符
	{
        if(token.length()>0){
            // cout << token << endl; // 输出
            vec.push_back(token);
        }
	}
    return vec;
}
//消除左递归，用来求follow集合(没用)
void removeLeftRecursive(){
    //找到具有左递归的NT
    for(Production p:productions){
        if(p.left==p.right[0]){
            LRset.insert(p.left);
        }
    }
    //NT改变
    for(string nt:nonTerminals){
        LRnonTerminals.insert(nt);
    }
    for(string s:LRset){
        LRnonTerminals.insert(s+"'");
    }
    //产生式改变
    for(string s:LRset){
        Production p;
        p.left = s+"'";
        p.right.push_back("E");
        productionsNLR.push_back(p);
    }
    for(Production p:productions){
        if(LRset.find(p.left)!=LRset.end()){
            if(p.right[0]==p.left){
                Production pt;
                pt.left = p.left+"'";
                for(int i=1;i<p.right.size();i++){
                    pt.right.push_back(p.right[i]);
                }
                pt.right.push_back(pt.left);
                productionsNLR.push_back(pt);
            }else{
                p.right.push_back(p.left+"'");
                productionsNLR.push_back(p);
            }
        }else{
            productionsNLR.push_back(p);
        }
    }
}


//计算全体first和follow集合
void findFirstAndFollow(){
    //消除左递归，计算first和follow
    // removeLeftRecursive();//LL也没考虑消除左递归
    // printProductions(productions);
    // printProductions(productionsNLR);
	//计算first
	for(string symbol:terminals){
		first[symbol].insert(symbol);
        // cout<<"finished"<<endl;
	}
	for(string symbol:nonTerminals){
		getFirst(symbol);
        // cout<<"sssss"<<endl;
	}
    // //左递归复原
    // for(string s:LRset){
    //     for(string ss:first[s]){
    //         first[s].insert(ss);
    //     }
    // }
	// printFirst();

	//计算follow
	follow[START_SYMBOL].insert("$");
    change = true;
    while(change){
        change = false;
        for(auto p:productions){
		    getFollow(p.left);
	    }
    }
    // //左递归复原
    // for(string s:LRset){
    //     for(string ss:follow[s]){
    //         follow[s].insert(ss);
    //     }
    // }
	// printFollow();
}

//计算给定非终结符的first集合
void getFirst(string target){
    for(Production p:productions){
        if(p.left == target){
            if(isE(p)){
                first[target].insert("E");
            }else{
                if(p.right[0]==target){//没消除左递归
                    continue;
                }
                bool done = false;
                for(string s:p.right){
                    if(first[s].empty()){
                        getFirst(s);
                    }
                    if(isSetE(first[s])){
                        continue;
                    }else{
                        for(string t:first[s]){
                            first[target].insert(t);
                        }
                        done = true;
                        break;
                    }
                }
                if(!done){
                    first[target].insert("E");
                }
            }
        }
    }
}

//判断产生式是不是E
bool isE(Production p){
    if(p.right.size()==1 && p.right[0]=="E"){
        return true;
    }
    return false;
}
//判断集合是不是E
bool isSetE(set<string> set2){
    set<string> e2;
    e2.insert("E");
    return (e2==set2);
}

// 计算给定非终结符的follow集合（利用change变量判断follow集合是否改变，从而停止getFollow函数）
void getFollow(string target){//new
    // cout<<target<<endl;
    for(Production p:productions){
        for(int i=0;i<p.right.size();i++){
            bool isE = false;
            if(p.right[i]==target){
                if(i!=p.right.size()-1){
                    string next = p.right[i+1];
                    for(string s:first[next]){
                        if(s!="E"){
                            int size = follow[target].size();
                            follow[target].insert(s);
                            if(follow[target].size()!=size){
                                change = true;
                            }
                        }else{
                            isE = true;
                        }
                    }
                }
                if(i==p.right.size()-1 || isE){
                    if(p.left!=target){
                        for(string s:follow[p.left]){
                            int size = follow[target].size();
                            follow[target].insert(s);
                            if(follow[target].size()!=size){
                                change = true;
                            }
                        }
                    }
                }
            }
        }
    }
}

//计算给定非终结符的follow集合（LR模式下的follow集合求解，不适用于左递归情况，不适用于LR分析）
// void getFollow(string target){//old
// 	for(auto p:productions){
// 		vector<string> right = p.right;
// 		int len = right.size();
// 		bool hasE = false;
// 		bool isEnd = false;
// 		for(int i=0;i<len;i++){
// 			if(right[i]==target){
// 				if(i+1<len){	
// 					for(string s:first[right[i+1]]){
// 						if(s=="E"){
// 							hasE = true;
// 						}else{
// 							follow[target].insert(s);
// 						}
// 					}
// 				}else{
// 					isEnd = true;
// 				}
// 			}
// 		}
// 		if((hasE || isEnd) && (p.left!=target)){
// 			for(string s:follow[p.left]){
// 				if(follow[p.left].size()==0){//取消掉if语句，可以处理一般左递归，但是实验平台的grammar会死循环
// 					getFollow(p.left);
// 				}
// 				for(string s:follow[p.left]){
// 					follow[target].insert(s);
// 				}
// 			}
// 		}
// 	}
// }

//打印全体First集合
void printFirst(){
	cout<<"First: "<<endl;
	for(string symbol:terminals){
		cout<<symbol<<": ";
		set<string> set= first[symbol];
		for(string s:set){
			cout<<s<<" ";
		}
		cout<<endl;
	}
	for(string symbol:nonTerminals){
		cout<<symbol<<": ";
		set<string> set= first[symbol];
		for(string s:set){
			cout<<s<<" ";
		}
		cout<<endl;
	}
}

//打印全体Follow集合
void printFollow(){
	cout<<"Follow: "<<endl;
	for(string left:nonTerminals){
		cout<<left<<": ";
		for(string s:follow[left]){
			cout<<s<<" ";
		}
		cout<<endl;
	}
}


//利用closure和goto生成states
void getStates(){
    queue<State*> q;
    //构造产生式
    Production p0;
    p0.left = NEW_SYMBOL;
    p0.right.push_back(START_SYMBOL);
    //构造S0
    State* s0 = new State();
    s0->id = 0;
    s0->items.insert(Item(p0,0));
    getClosure(s0);
    states.push_back(s0);
    q.push(s0);
    while(!q.empty()){
        State* state = q.front();
        q.pop();
        for(string symbol:symbols){
            if(symbol=="E"){//E为空，不参与转移
                continue;
            }
            State* tmp = new State();
            for(Item item:state->items){
                if(!isFinished(item)){
                    if(item.production.right[item.index]==symbol){
                        // cout<<"ssss"<<symbol<<endl;
                        item.index+=1;
                        tmp->items.insert(item);
                    }
                }
            }
            if(tmp->items.size()>0){
                getClosure(tmp);
                // printState(tmp);
                bool hasSame = false;
                for(State* s:states){
                    if(s->items==tmp->items){
                        tmp->id = s->id;
                        hasSame = true;
                        break;
                    }
                }
                if(!hasSame){
                    // cout<<"before: "<<tmp->id<<", ";
                    tmp->id = states.size();
                    // cout<<tmp->id<<endl;
                    states.push_back(tmp);
                    q.push(tmp);
                    // printStates();
                }
                state->nexts.push_back(Next(symbol,tmp->id));
            }
        }
        // printState(s0);
    }
    // printStates();
    // cout<<"states size: "<<states.size()<<endl;
    // cout<<"-------------";
}

//计算state的闭包
void getClosure(State* state){
    set<Item> items = state->items;
    queue<string> q;
    for(Item item:items){
        if(!isFinished(item)){
            q.push(item.production.right[item.index]);
        }
    }
    while(!q.empty()){
        for(Production p:productions){
            if(p.left==q.front()){
                Item item = Item(p,0);
                if(items.find(item)==items.end()){
                    items.insert(item);
                    q.push(item.production.right[0]);
                }
            }
        }
        q.pop();
    }
    state->items = items;
    // printState(state);
}

//判断Item是否执行完
bool isFinished(Item item){
    if(item.index >= item.production.right.size()){
        return true;
    }else if(item.production.right[0]=="E"){//E为空，直接是完成状态
        return true;
    }
    return false;
}

//打印State
void printState(State* state){
    cout<<"-----------"<<endl;
    cout<<"S"<<state->id<<":"<<endl;
    cout<<"items: "<<endl;
    for(Item item:state->items){
        cout<< toStringProduction(item.production) <<" "<<item.index<<endl;
    }
    cout<<"nexts: "<<endl;
    for(Next n: state->nexts){
        cout<<n.symbol<<" --> "<<n.stateId<<endl;
    }
}
//打印所有的states
void printStates(){
    for(State* s:states){
        printState(s);
    }
    cout<<"------end------"<<endl;
}


//生成parsing table
void getTable(){
    for(State* state: states){
        for(Item item:state->items){
            if(isFinished(item)){
                string symbol = NEW_SYMBOL;
                if(item.production.left == symbol){
                    table[state->id]["$"]=TEntry("accept");
                }else{
                    for(string s: follow[item.production.left]){
                        table[state->id][s] = TEntry("reduce",item.production);
                    }
                }
            }
            for(Next next:state->nexts){
                if(terminals.find(next.symbol)!=terminals.end()){
                    table[state->id][next.symbol] = TEntry("shift",next.stateId);
                }else{
                    table[state->id][next.symbol] = TEntry("goto",next.stateId);
                }
            }
        }
    }
    // printTable();
    // printTable2();
}

//打印Table，输出所有states对应的symbols的entry
void printTable(){
    //输出Table
    cout<<"Table: "<<endl<<endl;
    //输出head
	cout<<setiosflags(ios::left)<<setw(WIDE)<<"";
	for(string t: symbols){
		cout<<setiosflags(ios::left)<<setw(WIDE)<<t;
	}
	cout<<endl;

    //输出row
	for(int i=0;i<states.size();i++){
		cout<<setiosflags(ios::left)<<setw(WIDE)<<to_string(i);
		for(string t:symbols){
            TEntry tEntry= table[i][t];
            cout<<setiosflags(ios::left)<<setw(WIDE)<<toStringTEntry(tEntry);
		}
		cout<<endl;
	}
}

//打印Table2，适用于很多symbols的情况，一行放不下，限制没行元素个数
void printTable2(){
    //输出table
	cout<<"Table: "<<endl<<endl;

	for(int i=0;i<symbols.size();i+=COLS){
		cout<<setiosflags(ios::left)<<setw(WIDE)<<"";
		int k=0;
		for(string t: symbols){
			if(k>=i && k<i+COLS){
				cout<<setiosflags(ios::left)<<setw(WIDE)<<t;
			}
			k++;
		}
		cout<<endl;

		for(int j = 0;j<states.size();j++){
			cout<<setiosflags(ios::left)<<setw(WIDE)<<j;
			int k=0;
			for(string t:symbols){
				if(k>=i && k<i+COLS){
					TEntry tEntry = table[j][t];
					cout<<setiosflags(ios::left)<<setw(WIDE)<<toStringTEntry(tEntry);
				}
				k++;	
			}
			cout<<endl;
		}
		cout<<endl<<endl;
	}
}

//生成TEntry的string
string toStringTEntry(TEntry tEntry){
    string str="";
    if(tEntry.type == "shift" || tEntry.type == "goto"){
        str = str + tEntry.type+" "+to_string(tEntry.stateId);
    }else if(tEntry.type =="reduce"){
        str = str + tEntry.type + " ";
        str += toStringProduction(tEntry.production);
    }else if(tEntry.type=="accept"){
        str += tEntry.type;
    }else{
        str+="error";
    }
    return str;
}

//打印单个产生式
string toStringProduction(Production p){
    string str = p.left+" ->";
	for(auto w:p.right){
		str = str + " " + w;
	}
    return str;
}


//准备inputs和stack
void preparation(string prog){
    //初始化inputs
    getTokens(prog);//分离出待分析程序的tokens
    inputs.push_back(Token("$",-1));//加入"$"
    // printInputs();
    //初始化stack
    myStack.push(SEntry(0));
}

//把程序拆分成Tokens放入inputs.
void getTokens(string prog){
	istringstream iss(prog);	// 输入流
	string line;
	int i=1;
	while(getline(iss,line)){
		vector<string> words = split(line, ' ');
		for(string w:words){
			//w!=" " && w!="\t"&&w!="\n"&&w!="\r"
			if(w.length()!=0){
				Token t = Token(w,i);
				inputs.push_back(t);
			}
		}
		i++;
	}
	// cout<<prog<<endl<<endl;
	// printInputs();
}

//输出inputs
void printInputs(){
	for(auto t:inputs){
		cout<<"line: "<<t.line<<",  "<<t.token<<endl;
	}
}


//LR分析
void processLR(){
    // cout<<"----------SLR process----------"<<endl;
    int i=0;//inputs指针
    while(true){
        int line = inputs[i].line;
		string input = inputs[i].token;
        SEntry top = myStack.top();
        TEntry tEntry = table[top.stateId][input];
		// printProcessState(top,input,tEntry);//输出当前stack和input的状态
        
        if(tEntry.type=="accept"){
            break;
        }else if(tEntry.type=="shift"){
            myStack.push(SEntry(tEntry.stateId,input));
            i++;
        }else if(tEntry.type=="reduce"){
            Production p = tEntry.production;
            int num = p.right.size();
            if(p.right[0]=="E"){//E为空，不处理
                num = 0;
            }
            for(int j=0;j<num;j++){
                myStack.pop();
            }
            top = myStack.top();
            tEntry = table[top.stateId][p.left];
            if(tEntry.type=="goto"){
                myStack.push(SEntry(tEntry.stateId, p.left));
            }else{
                //goto table元素为空，随意选择一个可以生成p.left的状态
                for(int j=0;j<states.size();j++){
                    if(table[j][p.left].type=="goto"){
                        myStack.push(SEntry(j,p.left));
                        string msg = "change from "+to_string(top.stateId)+" to "+to_string(j);
                        errors.push_back(Error(3,line,msg));
                        break;
                    }
                }
            }
            outputs.push_back(p);
            // cout<<toStringProduction(p)<<endl;
        }else{
            //action table为空，默认缺少输入,随机插入一个当前状态可以生成的terminal到inputs中。
            //优先缺少";"
            string str = ";";
            if(table[top.stateId][str].type.length()>0){
                    inputs.insert(inputs.begin()+i,str);
                    errors.push_back(Error(2,inputs[i-1].line,str));
            }else{
                for(string s:terminals){
                    if(table[top.stateId][s].type.length()>0){
                        inputs.insert(inputs.begin()+i,s);
                        errors.push_back(Error(2,inputs[i-1].line,s));
                        break;
                    }
                }
            }
        }
    }
	// printOutputs();
}

//打印LR分析过程中的状态
void printProcessState(SEntry sEntry, string input,TEntry tEntry){
    cout<<"stack state id:  "<<sEntry.stateId<<", symbol"<<sEntry.symbol<<", input: "<<input
    <<", action: "<<toStringTEntry(tEntry)<<endl;
}


//输出错误
void printErrors(){
    // cout<<"--------Standard Outputs---------"<<endl;
	for(Error e:errors){
		string type;
		string error_msg;
		switch(e.id){
			case 1:
				type = "Invalid Input: ";
				error_msg = "line "+to_string(e.line)+", invalid input: "+e.msg;//input
				break;
			case 2:
				type = "Input Error: ";
				if(ERROR){//普适输出
					error_msg = "line "+to_string(e.line)+", no expected input: "+e.msg;//symbol	
				}else{//OJ输出
					error_msg = "语法错误， 第"+to_string(e.line)+"行， 缺少\""+e.msg+"\"";
					cout<<error_msg<<endl;
				}
				break;
            case 3:
                type = "Goto Error: ";
                error_msg = e.msg;
                break;
			default:
				type = "Error: ";
				error_msg = "line "+to_string(e.line)+", msg: "+e.msg;
				break;
		}
		if(ERROR){//普适输出
			cout<<type<<error_msg<<endl;	//如果需要提交代码到OJ，注释掉自定义错误输出
		}	
	}
}

//标准化输出
void printStandardOutput(){
	// cout<<"--------Standard Outputs---------"<<endl;
	vector<string> ovec;//输出栈
    ovec.push_back(START_SYMBOL);
    for(int i=outputs.size()-1;i>=0;i--){
        //输出当前的结果
        for(string s:ovec){
            if(s=="E"){
                continue;
            }
            cout<<s<<" ";
        }
        cout<<"=> "<<endl;
        //利用产生式更新结果
        stack<string> tmp;
        Production p = outputs[i];
        bool finish = false;
        while(!ovec.empty()){
            if(ovec.back()!=p.left || finish){
                tmp.push(ovec.back());            
            }else{
                for(auto it = p.right.rbegin();it!=p.right.rend();it++){
                    tmp.push(*it);
                }
                finish = true;
            }
            ovec.pop_back();
        }
        while(!tmp.empty()){
            ovec.push_back(tmp.top());
            tmp.pop();
        }
    }
    //输出当前的结果
    for(string s:ovec){
        if(s=="E"){
            continue;
        }
        cout<<s<<" ";
    }
}

//测试标准输出
void testStandardOutput(){
    string str = "F -> id, T -> F, F -> id, T -> T * F,\
    E -> T, F -> id, T -> F, E -> E + T";

    vector<string> pros = split(str, ',');

    for(string s:pros){
        Production p;
        vector<string> tmp = split(s,' ');
        p.left = tmp[0];
        for(int i=2;i<tmp.size();i++){
            p.right.push_back(tmp[i]);
        }
        outputs.push_back(p);
    }
    printStandardOutput();
}


//LR分析控制流
void Analysis()
{
	//读取待分析程序
	string prog;
	read_prog(prog);//读取待分析程序

    /********* Begin *********/
	//生成 SLR Parsing Table
	init();//分析grammar，生成productions，terminals，nonTerminals
    findFirstAndFollow();//计算全体first和follow集合
    getStates();//利用closure和goto计算所有states
	getTable();//计算parsing table

	//LR分析
    preparation(prog);//准备stack和inputs
    processLR();//LR分析
	printErrors();//输出错误
	printStandardOutput();//标准化输出，level*'\t'+token
    /********* End *********/
    
}
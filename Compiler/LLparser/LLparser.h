// C语言 LL1分析器
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
// #define TEST
# define OJ

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
const string START_SYMBOL= "S";
const string GRAMMAR = 
"S -> T S'\
\nS' -> + T S'\
\nS' -> E\
\nT -> F T'\
\nT' -> * F T'\
\nT' -> E\
\nF -> ( S )\
\nF -> id";

#else
const string START_SYMBOL= "program";
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
}Production;

// 待分析程序 & 标准化输出中的Token
typedef struct Token{
	string token;
	int line;//待分析程序的行号；输出token所处的语法树的高度（和输出产生式序号不完全相同）
	Token(string t="", int num=-1){
		token = t;
		line = num;
	}
}Token;

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

// LL1控制流函数
void Analysis();//LL分析总流程
void init();//分析grammar，提取productions，terminals，nonTerminals
void findFirstAndFollow();//计算整体的Follow和First
void getTable();//生成parsing table
void getInputs(string prog);//将目标程序拆分成inputs
void process();//LL1分析
void printErrors();//错误分析
void printStandardOutput();//标准化输出

// LL1辅助函数
void read_prog(string& prog);//读取程序片段
void getFirst(string target);//获取target的First集合
void getFollow(string target);//获取target的Follow集合
bool hasProduction(string target);//判断parsing table中能否生成target

// 其余辅助函数
vector<string> split(string str,const char split);//字符串分割函数
void printTerminals(set<string> terms);//打印终结符
void printNonTerminals(set<string> nonterms);//打印非终结符
void printProductions(vector<Production> productions);//打印grammar中的所有产生式
void printFirst();//打印First集合
void printFollow();//打印Follow集合
void printTable();//打印Table，每一行输出所有的terminals
void printTable2();//打印Table，每一行最多COLS个terminals
void printInputs();//打印Inputs
void printOutputs();//打印产生式序列
void printProduction(Production p);//打印一个产生式
void printProcessState(string symbol, string input);//打印LL1分析过程中的状态
void printOutToken(Token t);//打印输出的元素，'\t'*t.line + t.token

//-----------------------全局变量-------------------------
vector<Production> productions;//语法包含的所有产生式

set<string> terminals;//终结符集合
set<string> nonTerminals;//非终结符集合

unordered_map<string,set<string> > first;//First map；key为symbol，value为对应的first集合
unordered_map<string,set<string> > follow;//Follow map；key为symbol，value为对应的follow集合

unordered_map<string, unordered_map<string, Production> > table;//parsing table; table[nonTerminal][Terminal]对应值为产生式。

vector<Token> inputs;//待分析程序的token集合，以及"$"
stack<string> myStack;//分析栈

vector<Error> errors;//程序所有的错误
vector<Production> outputs;//程序分析结果（产生式序列）

//-----------------------函数实现---------------------------

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

	//打印
	// printTerminals(terminals);
	// printNonTerminals(nonTerminals);
	// printProductions(productions);
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
		// cout << token << endl; // 输出
        vec.push_back(token);
	}
    return vec;
}

//计算全体first和follow集合
void findFirstAndFollow(){
	//计算first
	for(string symbol:terminals){
		first[symbol].insert(symbol);
	}
	for(string symbol:nonTerminals){
		getFirst(symbol);
	}
	// printFirst();

	//计算follow
	follow[START_SYMBOL].insert("$");
	for(auto p:productions){
		getFollow(p.left);
	}
	printFollow();
}

//计算给定非终结符的first集合
void getFirst(string target){
	for(Production p:productions){
		if(p.left == target){
			bool hasRes = false;
			for(string symbol:p.right){
				if(first[symbol].size()==0){
					getFirst(symbol);
				}
				if(first[symbol].find("E")!=first[symbol].end()){
					continue;
				}
				for(string item:first[symbol]){
					first[target].insert(item);
				}
				hasRes = true;
				break;
			}
			if(hasRes==false){
				first[target].insert("E");
			}
		}
	}
}

//计算给定非终结符的follow集合
void getFollow(string target){
	for(auto p:productions){
		vector<string> right = p.right;
		int len = right.size();
		bool hasE = false;
		bool isEnd = false;
		for(int i=0;i<len;i++){
			if(right[i]==target){
				if(i+1<len){	
					for(string s:first[right[i+1]]){
						if(s=="E"){
							hasE = true;
						}else{
							follow[target].insert(s);
						}
					}
				}else{
					isEnd = true;
				}
			}
		}
		if((hasE || isEnd) && (p.left!=target)){
			for(string s:follow[p.left]){
				if(follow[p.left].size()==0){
					getFollow(p.left);
				}
				for(string s:follow[p.left]){
					follow[target].insert(s);
				}
			}
		}
	}
}

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


//生成parsing table
void getTable(){
	for(auto p:productions){
		bool hasE = false;
		bool done = false;
		for(string s: p.right){
			if((first[s].size()==1) && (first[s].find("E")!=first[s].end())){
				continue;
			}
			for(string item: first[s]){
				if(item=="E"){
					hasE = true;
				}else{
					table[p.left][item] = p;
				}
			}
			done = true;
			break;
		}
		if(hasE || !done){
			for(string item:follow[p.left]){
				table[p.left][item] = p;
			}
		}	
	}
	// printTable();//每行输出全部terminals元素
	// printTable2();//利用COLS限制每行元素个数
}

//打印Table
void printTable(){
	cout<<"Table: "<<endl<<endl;
	cout<<setiosflags(ios::left)<<setw(WIDE)<<"";
	for(string t: terminals){
		cout<<setiosflags(ios::left)<<setw(WIDE)<<t;
	}
	cout<<endl;

	for(string nt:nonTerminals){
		cout<<setiosflags(ios::left)<<setw(WIDE)<<nt;
		for(string t:terminals){
			Production p = table[nt][t];
			if(p.left==""){
				cout<<setiosflags(ios::left)<<setw(WIDE)<<"err";
			}else{
				string pro = p.left+"->";
				for(string s:p.right){
					pro+=" ";
					pro+=s;
				}
				cout<<setiosflags(ios::left)<<setw(WIDE)<<pro;
			}
		}
		cout<<endl;
	}
}

//打印Table2
void printTable2(){
	cout<<"Table: "<<endl<<endl;

	for(int i=0;i<terminals.size();i+=COLS){
		cout<<setiosflags(ios::left)<<setw(WIDE)<<"";
		int k=0;
		for(string t: terminals){
			if(k>=i && k<i+COLS){
				cout<<setiosflags(ios::left)<<setw(WIDE)<<t;
			}
			k++;
		}
		cout<<endl;

		for(string nt:nonTerminals){
			cout<<setiosflags(ios::left)<<setw(WIDE)<<nt;
			int k=0;
			for(string t:terminals){
				if(k>=i && k<i+COLS){
					Production p = table[nt][t];
					if(p.left==""){
						cout<<setiosflags(ios::left)<<setw(WIDE)<<"err";
					}else{
						string pro = p.left+"->";
						for(string s:p.right){
							pro+=" ";
							pro+=s;
						}
						cout<<setiosflags(ios::left)<<setw(WIDE)<<pro;
					}
				}
				k++;	
			}
			cout<<endl;
		}
		cout<<endl<<endl;
	}
}




//把程序拆分成Inputs
void getInputs(string prog){
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
	inputs.push_back(Token("$",-1));
	// cout<<prog<<endl<<endl;
	// printInputs();
}
void printInputs(){
	for(auto t:inputs){
		cout<<"line: "<<t.line<<",  "<<t.token<<endl;
	}
}


//LL1分析
void process(){
	//初始化分析栈
	myStack.push("$");
	myStack.push(START_SYMBOL);
	int i = 0;
	while(true){
		int line = inputs[i].line;
		string input = inputs[i].token;
		string symbol = myStack.top();
		// printProcessState(symbol,input);//输出当前stack和input的状态

		if(symbol==input){
			myStack.pop();
			i++;
			if(symbol=="$"){	
				break;
			}
		}else{
			if(symbol=="E"){
				myStack.pop();
				continue;
			}
			Production p = table[symbol][input];
			if(p.left.length()>0){
				// printProduction(p);//输出分析过程中使用的产生式
				outputs.push_back(p);
				myStack.pop();
				vector<string> right = p.right;
				for(auto it = right.rbegin();it!=right.rend();it++){
					myStack.push(*it);
				}
			}else{//错误处理
				if(!hasProduction(input)){//输入非法
					i++;
					errors.push_back(Error(1,line,input));
					if(i>=inputs.size()){
						break;
					}else{
						continue;
					}
				}
				if(terminals.find(symbol)!=terminals.end()){//栈顶symbol为终结符，且与input不匹配
					myStack.pop();
					errors.push_back(Error(2,inputs[i-1].line,symbol));
				}else{//parsing table为空
					myStack.pop();
					errors.push_back(Error(3,line,symbol+", "+input));
					//将symbol->E放入outputs
					Production pro = Production(symbol);
					pro.right.push_back("E");
					outputs.push_back(pro);
				}
				if(myStack.empty()){//栈空了
					for(auto s:nonTerminals){
						// cout<<s<<endl;
						if(table[s][input].left.length()>0){
							myStack.push(s);
							errors.push_back(Error(4,line,s));
							//将E->symbol放入outputs
							Production pro = Production("E");
							pro.right.push_back(s);
							outputs.push_back(pro);
							// cout<<"idea: "<<s<<endl;
							break;
						}
					}
				}
			}
		}
	}
	if(!myStack.empty()){//栈非空
		string msg;
		while(!myStack.empty()){
			msg+=myStack.top()+" ";
			myStack.pop();
		}
		errors.push_back(Error(5,-1,msg));
	}
	// printOutputs();
}

//判断parsing table中能否生成target
bool hasProduction(string target){
	if(terminals.find(target)==terminals.end()){
		return false;
	}
	for(string nt:nonTerminals){
		if(table[nt][target].left.length()>0){
			return true;
		}
	}
	return false;
}

//输出错误
void printErrors(){
	for(Error e:errors){
		string type;
		string error_msg;
		switch(e.id){
			case 1:
				type = "Invalid Input: ";
				error_msg = "line "+to_string(e.line)+", invalid input: "+e.msg;//input
				break;
			case 2:
				type = "No Expected Input Error: ";
				if(ERROR){//普适输出
					error_msg = "line "+to_string(e.line)+", no expected input: "+e.msg;//symbol	
				}else{//OJ输出
					error_msg = "语法错误,第"+to_string(e.line)+"行,缺少\""+e.msg+"\"";
					cout<<error_msg<<endl;
				}
				break;
			case 3:
				type = "Syntax Error: ";
				error_msg = "line "+to_string(e.line)+", no production in parsing table, from"+e.msg;//symbol,input
				break;
			case 4:
				type = "Empty Stack Error: ";
				error_msg = "line "+to_string(e.line)+", generate symbol: "+e.msg;//使用的产生式
				break;
			case 5:
				type = "Remaining Stack Error: ";
				error_msg = "line "+to_string(e.line)+", invalid input: "+e.msg;//栈内剩余元素
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

//打印LL1分析过程中的状态
void printProcessState(string symbol, string input){
	cout<<"stack: "<<symbol<<", input: "<<input<<endl;
}
//打印单个产生式
void printProduction(Production p){
	cout<<p.left<<" -> ";
	for(auto w:p.right){
		cout<<w<<" ";
	}
	cout<<endl;
}
//打印输出的产生式序列
void printOutputs(){
	cout<<"Production Outputs: "<<endl;
	for(auto p:outputs){
		cout<<p.left<<" -> ";
		for(auto s:p.right){
			cout<<s<<" ";
		}
		cout<<endl;
	}
}

// 利用栈进行标准化输出
// 输出要求： 每个单词占据一行，体现语法树结构
// 输出格式： 单词位于的语法树层级 * '\t' + 单词
void printStandardOutput(){
	// cout<<"Standard Outputs:"<<endl;
	stack<Token> ostack;//输出栈
	ostack.push(Token(START_SYMBOL,0));
	for(Production p:outputs){
		//输出非产生式左侧的元素
		while(p.left!=ostack.top().token){
			printOutToken(ostack.top());
			ostack.pop();
		}
		//输出产生式左侧的元素
		int line = ostack.top().line;
		printOutToken(ostack.top());
		ostack.pop();
		//将产生式右侧元素逆序入栈
		for(auto it = p.right.rbegin();it!=p.right.rend();it++){
			ostack.push(Token(*it,line+1));
		}
	}
	//输出栈内剩余元素
	while(!ostack.empty()){
		printOutToken(ostack.top());
		ostack.pop();
	}
}
//打印输出的元素，\t*line + token
void printOutToken(Token t){
	cout<<string(t.line,'\t');
	cout<<t.token<<endl;
}


// 标准输入函数
/* 不要修改这个标准输入函数 */
void read_prog(string& prog)
{
	char c;
	while(scanf("%c",&c)!=EOF){
		prog += c;
	}
}

void Analysis()
{
	//读取待分析程序
	string prog;
	read_prog(prog);//读取待分析程序

    /********* Begin *********/
	//生成parsing table
	init();//分析grammar，生成productions，terminals，nonTerminals
    findFirstAndFollow();//计算全体first和follow集合
	getTable();//计算parsing table

	//LL1分析
	getInputs(prog);//对待分析程序进行处理，分离出tokens，加入"$"
	process();//LL1分析
	printErrors();//输出错误
	printStandardOutput();//标准化输出，level*'\t'+token
    /********* End *********/
	
}
// C语言词法分析器
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>

#define SINGLE_QUOTE 1
#define SINGLE_COMMENT 2
#define WRONG_INPUT 3
using namespace std;

//Token结构
struct Token{
    string token;
    int num;
    Token(string t="",int n=0){
        token = t;
        num = n;
    }
};

unordered_map<string,int> reserveWord; //保留字，查询时间为O(1)
unordered_map<char,int> singleWord;//长度为1的符号，包括部分界符
vector<Token> results; //存储所有的Tokens

//用来构造unordered_map
static string reserveWordStr[32] = {
    "auto", "break", "case", "char", "const", "continue",
    "default", "do", "double", "else", "enum", "extern",
    "float", "for", "goto", "if", "int", "long",
    "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while"
};

/* 不要修改这个标准输入函数 */
void read_prog(string& prog)
{
	char c;
	while(scanf("%c",&c)!=EOF){
		prog += c;
	}
}

void init(){//初始化保留字+单长字
    //初始化reserveWord
    for(int i=0;i<32;i++){
        reserveWord.insert(pair<string,int>(reserveWordStr[i],i+1));
    }
    //初始化SingleWord
    singleWord.insert(pair<char,int>('(',44));
    singleWord.insert(pair<char,int>(')',45));
    singleWord.insert(pair<char,int>(',',48));
    singleWord.insert(pair<char,int>('.',49));
    singleWord.insert(pair<char,int>(':',52));
    singleWord.insert(pair<char,int>(';',53));
    singleWord.insert(pair<char,int>('?',54));
    singleWord.insert(pair<char,int>('[',55));
    singleWord.insert(pair<char,int>(']',56));
    singleWord.insert(pair<char,int>('{',59));
    singleWord.insert(pair<char,int>('}',63));
    singleWord.insert(pair<char,int>('~',64));
}

void printTokens(){//输出所有的Token
    int num = 1;
    for(vector<Token>::iterator it = results.begin();it!=results.end();it++){
        Token t = *it;
        if(num!=1){
            cout<<endl;
        }
        cout<<num++<<": <"<<t.token<<","<<t.num<<">";
    }
}
void putToken(string token, int num){
    results.push_back(Token(token,num));
}


bool isDigit(char ch)//判断是否为数字
{
    if(ch>= '0' && ch <= '9')
        return true;
    return false;
}
bool isLetter(char ch)//判断是否为字母或下划线
{
    if((ch>= 'a' && ch<='z') || ( ch <= 'Z' && ch >= 'A')|| ch == '_')
        return true;
    return false;
}
bool isSingle(char ch){//判断是否为单长关键字（长度为1，不具备延伸效应）
    return singleWord.find(ch)!=singleWord.end();
}

void takeTokens(string prog);//分离token，并且存入results

void errorProcess(int where, int signal=-1, string msg=""){//错误处理，提示错误信息，并且继续执行
    switch(signal){
        case SINGLE_QUOTE:
            cout<<"SINGLE QUATATION ERROR: ";
            break;
        case SINGLE_COMMENT:
            cout<<"SINGLE COMMENT ERROR: ";
            break;
        case WRONG_INPUT:
            cout<<"WRONG INPUT ERROR: ";
            break;
        default:
            cout<<"ERROR: ";
            break;
    }
    cout<<"No."<<where<<", ";
    cout<<msg<<endl;
}

void Analysis()
{
    init();//初始化keywords
	string prog;
	read_prog(prog);
    takeTokens(prog);//计算tokens
	printTokens();//输出tokens
}

void takeTokens(string prog){
    int p = 0;
    while(true){
        char ch = prog[p];
        string token="";
        int num = -1;
        if(ch=='\0'){//程序读取完毕
            break;
        }else if(ch=='\n' || ch==' '|| ch=='\t'){//跳过换行符,空格
            p++;
            continue;
        }else if(ch=='/' && (prog[p+1]=='/'|| prog[p+1]=='*')){//处理单行注释与多行注释
            token+=prog[p++];
            if(prog[p]=='/'){// 单行注释
                token+=prog[p++];
                num = 79;
                while(prog[p]!='\n' && prog[p]!='\0'){//终止条件：换行符或文本结束
                    token+=prog[p++];
                }
            }else if(prog[p]=='*'){//多行注释
                token+=prog[p++];
                num = 79;
                while(prog[p]!='\0' && (prog[p]!='*' || prog[p+1]!='/')){//终止条件：遇见*/或者文本结束
                    token+=prog[p++];
                }
                if(prog[p]=='\0'){//单注释错误（只有/*，没有*/）
                    string msg= "Only /*, and no */";
                    errorProcess(results.size()+1,SINGLE_COMMENT,msg);
                }else{//多行注释结束
                    token+=prog[p++];
                    token+=prog[p++];
                }
            }
            //只有一个‘/’,默认为除法
            // else{//只输入了一个'/',报错,忽略，继续执行
            //     errorProcess(results.size(),WRONG_INPUT,"single /");
            //     continue;
            // }
        }else if(isLetter(ch)){//标识符 & 保留字
            token+= prog[p++];
            while(isLetter(prog[p]) || isDigit(prog[p])){
                token+= prog[p++];
            }
            if(reserveWord.find(token)!=reserveWord.end()){//保留字
                num = reserveWord[token];
            }else{//标识符
                num = 81;
            }
        }else if(isDigit(ch)){//纯数字（没考虑浮点数、科学计数）
            token+=prog[p++];
            while(isDigit(prog[p])){
                token+=prog[p++];
            }
            num = 80;
        }else if(isSingle(ch)){//单长关键字
            token+=prog[p++];
            num = singleWord[ch];
        }else if(ch == '-'){//'-'
            token+=prog[p++];
            num = 33;
            if(prog[p]=='-'){//'--'
                token+=prog[p++];
                num = 34;
            }else if(prog[p]=='='){//'-='
                token+=prog[p++];
                num = 35;
            }else if(prog[p]=='>'){//'->'
                token+=prog[p++];
                num = 36;
            }
        }else if(ch=='!'){//'!'
            token+=prog[p++];
            num = 37;
            if(prog[p]=='='){//'!='
                token+=prog[p++];
                num = 38;
            }
        }else if(ch=='%'){//'%'
            token+=prog[p++];
            num = 39;
            if(prog[p]=='='){//'%='
                token+=prog[p++];
                num = 40;
            }
        }else if(ch=='&'){//'&'
            token+=prog[p++];
            num = 41;
            if(prog[p]=='&'){//'&&'
                token+=prog[p++];
                num = 42;
            }else if(prog[p]=='='){//'&='
                token+=prog[p++];
                num = 43;
            }
        }else if(ch=='*'){//'*'
            token+=prog[p++];
            num = 46;
            if(prog[p]=='='){//'*='
                token+=prog[p++];
                num = 47;
            }
        }else if(ch=='/'){//'/'
            token+=prog[p++];
            num = 50;
            if(prog[p]=='='){//'/='
                token+=prog[p++];
                num = 51;
            }
        }else if(ch=='^'){//'%'
            token+=prog[p++];
            num = 57;
            if(prog[p]=='='){//'^='
                token+=prog[p++];
                num = 58;
            }
        }else if(ch=='|'){//'|'
            token+=prog[p++];
            num = 60;
            if(prog[p]=='|'){//'||'
                token+=prog[p++];
                num = 61;
            }else if(prog[p]=='='){//'|='
                token+=prog[p++];
                num = 62;
            }
        }else if(ch=='+'){//'+'
            token+=prog[p++];
            num = 65;
            if(prog[p]=='+'){//'++'
                token+=prog[p++];
                num = 66;
            }else if(prog[p]=='='){//'+='
                token+=prog[p++];
                num = 67;
            }
        }else if(ch=='<'){//'<'
            token+=prog[p++];
            num = 68;
            if(prog[p]=='<'){//'<<'
                token+=prog[p++];
                num = 69;
                if(prog[p]=='='){//'<<='
                    token+=prog[p++];
                    num = 70;
                }
            }else if(prog[p]=='='){//'<='
                token+=prog[p++];
                num = 71;
            }
        }else if(ch=='='){//'='
            token+=prog[p++];
            num = 72;
            if(prog[p]=='='){//'=='
                token+=prog[p++];
                num = 73;
            }
        }else if(ch=='>'){//'>'
            token+=prog[p++];
            num = 74;
            if(prog[p]=='='){//'>='
                token+=prog[p++];
                num = 75;
            }else if(prog[p]=='>'){//'>>'
                token+=prog[p++];
                num = 76;
                if(prog[p]=='='){//'>>='
                token+=prog[p++];
                num = 77;
                }
            }
        }else if(ch=='\"'){//处理字符串"xxxx"
            //第一个'\"'
            token+=prog[p++];
            num = 78;
            putToken(token,num);
            //""包裹内容（默认包裹内容为标识符）
            token = "";
            while(p<prog.size() && prog[p]!='\"'){//程序可能存在错误，比如只有一个引号
                token+=prog[p++];
            }
            putToken(token,81);
            if(p==prog.size()){//单引号错误（只有一个单引号，一直读到程序结束）
                string msg = "Only one quotation mark!";
                
                errorProcess(results.size(),SINGLE_QUOTE,msg);
                continue;
            }else{//继续执行，第二个'\"'
                token=prog[p++];
                num = 78;
            }
        }else{//错误字符，忽略，继续执行
            p++;
            string msg = "wrong letter: ";
            msg+=ch;
            errorProcess(results.size()+1,WRONG_INPUT,msg);
            continue;
        }
        putToken(token,num);
    }
}
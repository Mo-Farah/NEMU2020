#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ,NEQ,NUMBER,HNUMBER,REGISTER,AND,OR,MARK,POINTOR,MINUS,R_DS

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
	int priority;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */
	{"\\b[0-9]+\\b",NUMBER,0}, //number
	{"\\b0[xX][0-9a-fA-F]+\\b",HNUMBER,0},//16number
	{"\\$[a-zA-Z]+",REGISTER,0}, //register
	{"!=",NEQ,3}, //not equal
	{"!",'!',6}, //not
	{"\\*",'*',5}, //mul
	{"/",'/',5},//div
	{"	+",NOTYPE,0},//tabs
	{" +",	NOTYPE,0},				// spaces
	{"\\+", '+',4},					// plus
	{"-",'-',4},//sub
	{"==", EQ,3},						// equal
	{"&&",AND,2},//and
	{"\\|\\|",OR,1},//or
	{"\\(",'(',7},//left bracket
	{"\\)",')',7},//right bracket
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
	int priority;
} Token;

Token token[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE: break;
					default: 
						token[nr_token].type = rules[i].token_type;
						token[nr_token].priority=rules[i].priority;
						strncpy(token[nr_token].str,substr_start,substr_len);
						token[nr_token].str[substr_len]='\0';
						nr_token ++;
				}
				position +=substr_len;

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

bool check_similar(int u,int v)
{
	int i;
	if(token[u].type == '('&&token[v].type ==')')
	{
		int uc=0,vc=0;
		for(i=u+1;i<v;i++)
		{
			if(token[i].type == '(')uc++;
			if(token[i].type ==')')vc++;
			if(vc>uc)return false;
		}
		if(uc==vc)return true;
	}
	return false;
}


int dominant_operator(int l,int r)
{
	int i,j;
	int min_priority=10;
	int oper=l;
	for(i=l;i<=r;i++)
	{
		if(token[i].type==NUMBER||token[i].type==HNUMBER||token[i].type==REGISTER||token[i].type==MARK)
			continue;
		int cnt=0;
		bool key=true;
		for(j=i-1;j>=l;j--)
		{
			if(token[j].type=='('&&!cnt){key =false;break;}
			if(token[j].type=='(')cnt --;
			if(token[j].type==')')cnt ++;
		}
		if(!key)continue;
		if(token[i].priority<=min_priority){printf("%d\n",min_priority) ;min_priority=token[i].priority;oper=i;}
	}
	printf("%d\n",oper);
	return oper;
}


uint32_t eval(int l,int r){
	if(l>r){Assert(l>r,"something happened!\n");return 0;}
	if(l==r){
	uint32_t num=0;
	if(token[l].type==NUMBER)
		sscanf(token[l].str,"%d",&num);
		return num;
	if(token[l].type==HNUMBER)
		sscanf(token[l].str,"%x",&num);
	if(token[l].type==REGISTER)
		{
			if(strlen(token[l].str)==3){
			int i;
			for(i=R_EAX;i<=R_EDI;i++)
				if(strcmp(token[l].str,regsl[i])==0)break;
				if(i>R_EDI)
				if(strcmp(token[l].str,"eip")==0)
					num=cpu.eip;
				else Assert(1,"no this register!\n");
			else num =reg_l(i);
			
			}
			else if(strlen(token[l].str)==2){
			if(token[l].str[1]=='x'||token[l].str[1]=='p'||token[l].str[1]=='i'){
				int i;
				for(i=R_AX;i<=R_DI;i++)
					if(strcmp(token[l].str,regsw[i])==0)break;
				num=reg_w(i);
			}
			else if(token[l].str[1]=='l'||token[l].str[1]=='h'){
				int i;
				for(i=R_AL;i<=R_BH;i++)
					if(strcmp(token[l].str,regsb[i])==0)break;
				num=reg_b(i);
			}
			else assert(1);
		}
		return num;
	}
	}
	else if(check_similar(l,r)==true)return eval(l+1,r-1);
	else {
		int op=dominant_operator(l,r);
		printf("%d\n",op);
		if(l==op||token[op].type==POINTOR||token[op].type==MINUS||token[op].type=='!')
		{
			uint32_t val=eval(l+1,r);
			switch(token[l].type)
			{
				case POINTOR: ;return swaddr_read(val,4);
				case MINUS:return -val;
				case '!':return !val;
				default :Assert(1,"default\n");
			}
		}
		uint32_t val1=eval(l,op-1);
		uint32_t val2=eval(op+1,r);
		switch(token[op].type)
		{
			case '+':return val1+val2;
			case '-':return val1-val2;
			case '*':return val1*val2;
			case '/':return val1/val2;
			case  EQ:return val1==val2;
			case NEQ:return val1!=val2;
			case AND:return val1&&val2;
			case OR:return val1||val2;
			default:
			break;
		}
	}
		assert(1);
		return -123456;

}


uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	*success = true;	
	//printf("%d",nr_token-1);
	return eval(0,nr_token-1);
}
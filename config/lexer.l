%{
#include <cstdio>
#include <cstring>
#include <string>
#include "node.hh"
#include "tok.hh"

int yycolumn = 1;
extern int yyerror(const char*);
extern int error_num;

#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; yylloc.last_column = yycolumn+yyleng-1; \
    yycolumn += yyleng;
int comment=0; //0:正常,1:在评论中,2:评论中出现//
int flag=0;  //1:表示在多行评论中,2表示在单行评论中
%}

%option yylineno


letter			[A-Za-z_]
digit			[0-9]
delim			[ \t] 
ws				{delim}+
id				{letter}({letter}|{digit})*
number			{digit}+(\.{digit}+)?(E[+\-]?{digit}+)?



%%
{ws}			{/*没有动作,也不返回*/}
"/*"   			{ flag=1;comment=1; }
"*/"			{ flag=0;comment=0; }

"//"	  		{ flag=2;comment=2; }
while			{ if(comment==0) return(WHILE);}
if				{ if(comment==0) return(IF);}
else			{ if(comment==0) return(ELSE);}
void			{ if(comment==0) return(VOID);}
int				{ if(comment==0) return(INT);}
const			{ if(comment==0) return(CONST);}
for				{ if(comment==0) return(FOR);}
break			{ if(comment==0) {
				yylval.var = new std::string (yytext);
				return(BREAK);}}
continue		{ if(comment==0) {
				yylval.var = new std::string (yytext);
				return(BREAK);}}
return			{ if(comment==0) {
				yylval.var = new std::string (yytext);
				return(RETURN);}}

"<"				{ if(comment==0){
				yylval.var = new std::string (yytext);
			  	return(LT);}}
"<="			{ if(comment==0){
				yylval.var = new std::string (yytext);
			   	return(LE);}}
"=="			{ if(comment==0){
				yylval.var = new std::string (yytext);
			  	return(EQ);}}
">"				{ if(comment==0){
				yylval.var = new std::string (yytext);
			  	return(GT);}}
">="			{ if(comment==0){
				yylval.var = new std::string (yytext);
			  	return(GE);}}
"!"				{ if(comment==0){
				yylval.var = new std::string (yytext);
				return(NOT);}}
"!="			{ if(comment==0){
				yylval.var = new std::string (yytext);
			  	return(UE);}}	  	
"||"			{ if(comment==0){
				yylval.var = new std::string (yytext);
			  	return(OR);}}
"&&"			{ if(comment==0){
				yylval.var = new std::string (yytext);
			  	return(AND);}}
{number}        {   
			  		if(comment==0){
						yylval.num = atoi (yytext);
                        return NUMBER;
			  		}
                }
{id}    		{   
			  		if(comment==0){
			  		yylval.var = new std::string (yytext);
                    return IDENTIFIER;
			  		}
                }
"="             { if(comment==0) return '=';}
"+"             { if(comment==0) return '+';}
"-"             { if(comment==0) return '-';}
"*"             { if(comment==0) return '*';}
"/"             { if(comment==0) return '/';}
"("             { if(comment==0) return '(';}
")"             { if(comment==0) return ')';}
"{"				{ if(comment==0) return '{';}
"}"				{ if(comment==0) return '}';}
"["				{ if(comment==0) return '[';}
"]"				{ if(comment==0) return ']';}
"%"	            { if(comment==0) return '%';}
","				{ if(comment==0) return ',';}
";"             { if(comment==0) return ';';}
\n              {   
			  		if(flag==1) 	comment=1;
			 		else	        comment=0;
			  		yycolumn = 1;
                }
[\t ]*			{ }
.               { if(comment==0) return *yytext;}

%%

int yywrap()
{
	return 1;
}



/*
	lex.c  --  lexical analyzer (main part)

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

$Log: RCS/lex.c $
 * revision 2.1 cond 89/10/04 23:29:12
 * Official version distributed with C-magazine, Dec 1989
 * 
 * revision 1.2 cond 89/09/17 20:35:05
 * Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
 * 
*/


#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
/* #include <jctype.h> */ /* by E.Kako */

#include "cmdef.h"
#include "cm.h"


static char rcsID[] = "$Header: RCS/lex.c 2.1 89/10/04 23:29:12 cond Exp $";

#define	height(ary)	(sizeof(ary) / sizeof((ary)[0]))
#define	h_bound(ary)	(ary + height(ary))


int	LineNo = 1;
extern	char	FileName[];

static	char	unget_char = 0;

extern	FILE	*inf;
extern	YYSTYPE	yylval;



/*************************************************************************/
/*                                                                       */
/*  readch and backch                                                    */
/*                                                                       */
/*************************************************************************/

public	char	readch(void)
{
	char	c;

	if ((c = unget_char) != 0)
		unget_char = 0;
	else
		c = getc(inf);
	if (c == '\n')
		LineNo++;
	return	c;
}

public	void	backch(char  c)
{
	unget_char = c;
	if (c == '\n')
		LineNo--;
}


/*************************************************************************/
/*                                                                       */
/*   IDENTIFIER                                                          */
/*                                                                       */
/*************************************************************************/

char	yyident[IDENTLENGTH+1];

private	YYLEX	lexId(char ch)
{
	char	*p;
	bool	err = NO;
	static	struct	keyword	{
		char	*name;
		YYLEX	token;
	}	kwdtbl[] =
	{
		"break",	BREAK,
		"case",		CASE,
		"char",		CHAR,
		"continue",	CONTINUE,
		"default",	DEFAULT,
		"do",		DO,
		"else",		ELSE,
		"extern",	EXTERN,
		"for",		FOR,
		"goto",		GOTO,
		"if",		IF,
		"int",		INT,
		"return",	RETURN,
		"sizeof",	SIZEOF,
		"switch",	SWITCH,
		"while",	WHILE,
	}, *kp;

	for (p = yyident; isalnum(ch) || ch == '_'; ch = readch()) {
		if (p >= &yyident[IDENTLENGTH]) {
			if (!err) {
				error("identifier too long");
				err = YES;
			}
		} else
			*p++ = ch;
	}
	backch(ch);
	*p = '\0';

	for (kp = kwdtbl; kp < h_bound(kwdtbl); kp++) {
		if (strcmp(yyident, kp->name) == 0)
			return	kp->token;
	}
	yylval.symbol = yyident;
	return	IDENTIFIER;
}


/*************************************************************************/
/*                                                                       */
/*   NUMBER                                                              */
/*                                                                       */
/*************************************************************************/

private	YYLEX	lexNumber(char  ch)
{
	int	radix, digit;
	long	val;

	if (ch == '0') {
		if ((ch = readch()) == 'x' || ch == 'X') {
			radix = 16;
			ch = readch();
		} else
			radix = 8;
	} else
		radix = 10;

	for (val = 0; (digit = tonumber(ch, radix)) != -1; ch = readch())
		val = val * radix + digit;
	backch(ch);

	yylval._const.type = CONST_INT;
	yylval._const.value = val;
	return	CONSTANT;
}


/*************************************************************************/
/*                                                                       */
/*   LEXICAL ANALYZER                                                    */
/*                                                                       */
/*************************************************************************/

private	bool	chkeq(void)
{
	char	c;

	if ((c = readch()) == '=')
		return	YES;
	else {
		backch(c);
		return	NO;
	}
}

private	void	skipComment(void)
{
	char	c;

	for (;;) {
		while ((c = readch()) != '*') {
			if (c == EOF)
				fatalError("unexpected EOF");
			if (iskanji(c))
				c = readch();
		}
		if ((c = readch()) == '/')
			return;
		backch(c);
	}
}

private	void	preprocess(void)
{
	char	c;
	char	*p;

	while ((c = readch()) == ' ' || c == '\t')
		;
	if (isdigit(c))
		goto _line;
	if (!isalpha(c))
		return;
	lexId(c);
	if (strcmp(yyident, "line") == 0) {
		while ((c = readch()) == ' ' || c == '\t')
			;
		if (!isdigit(c)) {
			error("bad #line");
			return;
		}
_line:
		LineNo = 0;
		while (isdigit(c)) {
			LineNo = LineNo * 10 + c - '0';
			c = readch();
		}
		while (c == ' ' || c == '\t')
			c = readch();
		if (c == '"') {
			p = readString();
			strcpy(FileName, p);
		}
	} else if (strcmp(yyident, "pragma") == 0 ||
		   strcmp(yyident, "p") == 0) {
		while ((c = readch()) != '\n')
			;
	} else
		error("bad # command");
}


public	YYLEX	yylex(void)
{
	char	c;

tryAgain:
	while ((c = readch()) == ' ' || c == '\t' || c == '\n')
		;
	if (isalpha(c) || c == '_')
		return	lexId(c);
	else if (isdigit(c))
		return	lexNumber(c);
	else if (c == '\'') {
		yylval._const.type = CONST_CHAR;
		yylval._const.value = chaval(readch());
		if((c = readch()) != '\'') {
			backch(c);
			error("missing quote");
		}
		return CONSTANT;
	}
	else if (c == '"')
		return lexString();
	else {
		yylval.op = c;
		switch (c) {
		Case '!':	return	chkeq() ? eqop : unop;
		Case '%':	return	chkeq() ? assignop : mulop;
		Case '&':	if ((c = readch()) == '&')
					return	logand;
				else if (c == '=')
					return	assignop;
				backch(c);
				return	'&';
		Case '*':	return	chkeq() ? assignop : '*';
		Case '+':	if ((c = readch()) == '+')
					return	incdecop;
				else if (c == '=')
					return	assignop;
				backch(c);
				return	addop;
		Case '-':	if ((c = readch()) == '-')
					return	incdecop;
				else if (c == '=')
					return	assignop;
				backch(c);
				return	addop;
		Case '/':	if ((c = readch()) == '=')
					return	assignop;
				else if (c == '*') {
					skipComment();
					goto tryAgain;
				}
				backch(c);
				return	mulop;
		Case ':':	return	':';
		Case '<':	if ((c = readch()) == '=') {
					yylval.op = '(';
					return	compop;
				} else if (c == '<') {
					if (chkeq())
						return	assignop;
					else
						return	shiftop;
				}
				backch(c);
				return	compop;
		Case '=':	return	chkeq() ? eqop : '=';
		Case '>':	if ((c = readch()) == '=') {
					yylval.op = ')';
					return	compop;
				} else if (c == '>') {
					if (chkeq())
						return	assignop;
					else
						return	shiftop;
				}
				backch(c);
				return	compop;
		Case '?':	return	'?';
		Case '^':	return	chkeq() ? assignop : '^';
		Case '|':	if ((c = readch()) == '=')
					return	assignop;
				else if (c == '|')
					return	logor;
				backch(c);
				return	'|';
		Case '~':	return	unop;
		Case '#':
				preprocess();
				goto tryAgain;
		Case EOF: return	-1;
		}
		return (c);
	}
}

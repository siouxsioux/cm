/*
	lexstr.c	lexical analyzer (string)

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

$Log: RCS/lexstr.c $
 * revision 2.1 cond 89/10/04 23:33:12
 * Official version distributed with C-magazine, Dec 1989
 * 
 * revision 1.2 cond 89/09/17 20:35:10
 * Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
 * 
*/

#include <stdio.h>
#include <ctype.h>
/* #include <jctype.h> */ /* by E.Kako */

#include "cmdef.h"
#include "cm.h"


static char rcsID[] = "$Header: RCS/lexstr.c 2.1 89/10/04 23:33:12 cond Exp $";

extern	YYSTYPE	yylval;


#define	MAXSTRPOOL	1500
#define	MAXSTROBJ	50


typedef	struct	strobj	{
	int	strId;
	char	*string;
	int	length;
}	STROBJ;

static	char	strpool[MAXSTRPOOL];
static	char	*poolptr;
static	STROBJ	strobj[MAXSTROBJ];
static	STROBJ	*objptr;

/*  initStr -- initialize string area (called on entry into function) */
public	void	initStr(void)
{
	poolptr = strpool;
	objptr = strobj;
}

/*  flushStr -- flushes string area to output file     */
/*              (called on exit from function)         */
public	void	flushStr(void)
{
	STROBJ	*p;
	char	*s;
	int	n;

	for (p = strobj; p < objptr; p++) {
		openseg(seg_DATA);
		gencode("@%d\tlabel\tbyte\n", p->strId);
		s = p->string;
		for (n = p->length; n > 0; n--)
			gencode("\tdb\t%d\n", (int)*s++);
	}
}

/*  appench -- put a character into string pool */
private	void	appendch(char  c)
{
	if (poolptr >= &strpool[MAXSTRPOOL])
		fatalError("string pool over flow");
	*poolptr++ = c;
}

/*  allocstrobj -- allocate string object */
private	STROBJ	*allocstrobj(void)
{
	if (objptr >= &strobj[MAXSTROBJ])
		fatalError("string pool over flow");
	return	objptr++;
}


/*  isoctal -- checks whether the character is Octal digit */
private	bool	isoctal(char  c)
{
	return (c >= '0' && c <= '7');
}

/*  tonumber -- convert one character to corresponding value */
/*              radix is given by parameter                  */
public	int	tonumber(char  c, int  radix)
{
	int	n;

	if (isdigit(c))
		n = c - '0';
	else if (isupper(c))
		n = c - 'A' + 10;
	else if (islower(c))
		n = c - 'a' + 10;
	else
		return	-1;
	return	n < radix ? n : -1;
}

/*  chaval -- convert one character into 'value'                   */
/*            handles Octals (such as \023), escapes (such as \n). */
public	char	chaval(char  ch)
{
	char	val;
	int	i, j;

	if (ch == '\\') {
		if (isoctal(ch = readch())) {
			val = 0;
			for (i = 0; isoctal(ch) && i < 3; i++) {
				val = val * 8 + ch - '0';
				ch = readch();
			}
			backch(ch);
		}
		else
			switch(ch) {
			Case 'n':	return ('\n');
			Case 't':	return ('\t');
			Case 'b':	return ('\b');
			Case 'r':	return ('\r');
			Case 'f':	return ('\f');
			Case 'v':	return ('\013');
			Case 'a':	return ('\007');
			Case 'x':
				val = 0;
				for (i = 0; i < 2; i++) {
					j = tonumber(ch = readch(), 16);
					if (j == -1) {
						backch(ch);
						break;
					}
					val = val * 16 + j;
				}
				return	val;
			Default:	return (ch);
			}
	}
	else
		val = ch;
	return (val);
}

public	char	*readString(void)
{
	char	ch;
	char	*from = poolptr;

	while ((ch = readch()) != '"' && ch != '\n' && ch != EOF) {
		if (ch == '\\') {
			if ((ch = readch()) == '\n')
				continue;
			backch(ch);
			ch = '\\';
		}
		appendch(chaval(ch));
		if (iskanji(ch))
			appendch(readch());
	}
	if (ch != '"') {
		backch(ch);
		error("missing quote");
	}
	appendch('\0');
	return from;
}

/*  lexString -- portion of lexical analyzer handling string */
public	YYLEX	lexString(void)
{
	char	*from;
	STROBJ	*s;

	from = readString();
        s = allocstrobj();
	s->string = from;
	s->length = poolptr - from;
	s->strId = yylval._const.value = gensym();
	yylval._const.type = CONST_STRING;
	return	CONSTANT;
}

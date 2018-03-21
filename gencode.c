/*
	gencode.c -- code generate routine

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

$Log: RCS/gencode.c $
 * revision 2.1 cond 89/10/04 23:30:58
 * Official version distributed with C-magazine, Dec 1989
 * 
 * revision 1.2 cond 89/09/17 20:34:46
 * Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
 * 
*/

#include <stdarg.h>
#include <stdio.h>


#include "cmdef.h"


static char rcsID[] = "$Header: RCS/gencode.c 2.1 89/10/04 23:30:58 cond Exp $";

extern	FILE	*outf;

public	void	gencode(char *fmt, ...)
{
	char	*p, optype;
	va_list	ap;
	SYMTBL	*sym;
	EXPR	*exp;
	int	offset;

	va_start(ap, fmt);
	p = fmt;
	for ( ; *p; p++) {
		if (*p == '%') {
			switch (*++p) {
 			Case 'r':		/*  register (AX,AL)  */
				optype = va_arg(ap, int);
				switch (optype) {
				Case 'C':		fputs("al", outf);
				Case 'K' or 'I':	fputs("eax", outf);
				Case 'R':		fputs("ebx", outf);
				Default:		bug("gencode");
				}
			Case 'a':		/*  aux register (DX,DL)  */
				optype = va_arg(ap, int);
				switch (optype) {
				Case 'C':		fputs("dl", outf);
				Case 'K' or 'I':	fputs("edx", outf);
				Case 'R':		fputs("edx", outf);
				Default:		bug("gencode");
				}
			Case 'y':		/*  aux register2 (CX,CL)  */
				optype = va_arg(ap, int);
				switch (optype) {
				Case 'C':		fputs("cl", outf);
				Case 'K' or 'I':	fputs("ecx", outf);
				Case 'R':		fputs("ecx", outf);
				Default:		bug("gencode");
				}
			Case 'v':
				exp = va_arg(ap, EXPR *);
				if (!isVariable(exp))
					bug("gencode:%v");
				sym = exp->e_left->e_var;
				offset = exp->e_left->e_offset;
				if (exp->type == TYPE_CHAR)
					fputs("byte ptr ", outf);
				else
					fputs("dword ptr ", outf);
				switch (sym->class) {
				Case SC_GLOBAL or SC_EXTERN:
					if (offset == 0)
						fprintf(outf, "_%s", sym->name);
					else if (offset > 0)
						fprintf(outf, "_%s+%d",
							sym->name, offset);
					else
						fprintf(outf, "_%s-%d",
							sym->name, -offset);
				Default:
					fprintf(outf, "[ebp%+d]",
						offset + sym->offset);
				}
			Case 'o':		/*  offset word/byte */
				optype = va_arg(ap, int);
				switch (optype) {
				Case 'C':	fputs("byte ptr ", outf);
				Case 'K' or 'I' or 'R':
						fputs("dword ptr ", outf);
				Default:		bug("gencode");
				}
			Case 'd':
				fprintf(outf, "%d", va_arg(ap, int));
			Case 's':
				fputs(va_arg(ap, char *), outf);
			Default:
				putc('%', outf);
				putc(*p, outf);
			}
		} else
			putc(*p, outf);
	}
}


public	void	genlea(EXPR *p)
{
	SYMTBL	*sym;
	int	offset;

	if (p->opcode != op_ADR)
		bug("genlea");
	sym = p->e_var;
	offset = p->e_offset;
	if (sym->class == SC_LOCAL || sym->class == SC_PARAM)
		gencode("\tlea\tbx,%d[ebp]\n", sym->offset + offset);
	else {
		if (isFunction(sym->type))
			gencode("\tmov\tebx,offset _%s\n", sym->name);
		else {
//			gencode("\tmov\tebx,offset DGROUP:_%s", sym->name);
			gencode("\tmov\tebx,offset _%s", sym->name);
			if (offset > 0)
				gencode("+%d", offset);
			else if (offset < 0)
				gencode("-%d", -offset);
			gencode("\n");
		}
	}
}


public	void	pushax(void)
{
	gencode("\tpush\teax\n");
}

public	void	pushbx(void)
{
	gencode("\tpush\tebx\n");
}

public	void	popax(void)
{
	gencode("\tpop\teax\n");
}

public	void	popbx(void)
{
	gencode("\tpop\tebx\n");
}

public	void	popcx(void)
{
	gencode("\tpop\tecx\n");
}

public	void	popdx(void)
{
	gencode("\tpop\tedx\n");
}

public	void	popdi(void)
{
	gencode("\tpop\tedi\n");
}


static	SEG_T	curSeg = seg_NO;

public	void	openseg(SEG_T seg)
{
static	char	text[] = "\t.code\n";
static	char	data[] = "\t.data\n";
static	char	bss[]  = "\t.data?\n";

	if (seg != curSeg) {
		switch (seg) {
		Case seg_TEXT:	gencode(text);
		Case seg_DATA:	gencode(data);
		Case seg_BSS:	gencode(bss);
		Default:	bug("openseg");
		}
		curSeg = seg;
	}
}

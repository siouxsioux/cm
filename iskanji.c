int iskanji(int c)
{
	if (((c>=0x80)&&(c<0xA0))||(c>=0xE0)) { return 1; }
	return 0;
}

int iskanji2(int c)
{
	return 1; /* Not Good, but works. */
}

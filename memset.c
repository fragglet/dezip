/* memset.c */

char *
memset(ptr,val,len)
register char *ptr;
register int val, len;
{
	char *start;

	start = ptr;
	while (len-- > 0)
		*ptr++ = val;
	return(start);
}

/* memcpy.c */

char *
memcpy(dst,src,len)
register char *dst, *src;
register int len;
{
	char *start;

	start = dst;
	while (len-- > 0)
		*dst++ = *src++;
	return(start);
}

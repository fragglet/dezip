/* zmemset - memset for systems without it
 *  bill davidsen - March 1990
 */

zmemset(buf, init, len)
 register char *buf, init;	/* buffer loc and initializer */
 register int len;		/* length of the buffer */
{
    while (len--) *(buf++) = init;
}

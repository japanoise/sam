#include <u.h>

static char t16e[] = "0123456789ABCDEF";

int dec16(uchar *out, int lim, char *in, int n) {
	int    c, w = 0, i = 0;
	uchar *start = out;
	uchar *eout = out + lim;

	while (n-- > 0) {
		c = *in++;
		if ('0' <= c && c <= '9') {
			c = c - '0';
		} else if ('a' <= c && c <= 'z') {
			c = c - 'a' + 10;
		} else if ('A' <= c && c <= 'Z') {
			c = c - 'A' + 10;
		} else {
			continue;
		}
		w = (w << 4) + c;
		i++;
		if (i == 2) {
			if (out + 1 > eout) {
				goto exhausted;
			}
			*out++ = w;
			w = 0;
			i = 0;
		}
	}
exhausted:
	return out - start;
}

int enc16(char *out, int lim, uchar *in, int n) {
	unsigned int c;
	char        *eout = out + lim;
	char        *start = out;

	while (n-- > 0) {
		c = *in++;
		if (out + 2 >= eout) {
			goto exhausted;
		}
		*out++ = t16e[c >> 4];
		*out++ = t16e[c & 0xf];
	}
exhausted:
	*out = 0;
	return out - start;
}

int dec32(uchar *dest, int ndest, char *src, int nsrc) {
	char  *s, *tab;
	uchar *start;
	int    i, u[8];

	if (ndest + 1 < (5 * nsrc + 7) / 8) {
		return -1;
	}
	start = dest;
	tab = "23456789abcdefghijkmnpqrstuvwxyz";
	while (nsrc >= 8) {
		for (i = 0; i < 8; i++) {
			s = strchr(tab, (int)src[i]);
			u[i] = s ? s - tab : 0;
		}
		*dest++ = (u[0] << 3) | (0x7 & (u[1] >> 2));
		*dest++ =
		    ((0x3 & u[1]) << 6) | (u[2] << 1) | (0x1 & (u[3] >> 4));
		*dest++ = ((0xf & u[3]) << 4) | (0xf & (u[4] >> 1));
		*dest++ =
		    ((0x1 & u[4]) << 7) | (u[5] << 2) | (0x3 & (u[6] >> 3));
		*dest++ = ((0x7 & u[6]) << 5) | u[7];
		src += 8;
		nsrc -= 8;
	}
	if (nsrc > 0) {
		if (nsrc == 1 || nsrc == 3 || nsrc == 6) {
			return -1;
		}
		for (i = 0; i < nsrc; i++) {
			s = strchr(tab, (int)src[i]);
			u[i] = s ? s - tab : 0;
		}
		*dest++ = (u[0] << 3) | (0x7 & (u[1] >> 2));
		if (nsrc == 2) {
			goto out;
		}
		*dest++ =
		    ((0x3 & u[1]) << 6) | (u[2] << 1) | (0x1 & (u[3] >> 4));
		if (nsrc == 4) {
			goto out;
		}
		*dest++ = ((0xf & u[3]) << 4) | (0xf & (u[4] >> 1));
		if (nsrc == 5) {
			goto out;
		}
		*dest++ =
		    ((0x1 & u[4]) << 7) | (u[5] << 2) | (0x3 & (u[6] >> 3));
	}
out:
	return dest - start;
}

int enc32(char *dest, int ndest, uchar *src, int nsrc) {
	char *tab, *start;
	int   j;

	if (ndest <= (8 * nsrc + 4) / 5) {
		return -1;
	}
	start = dest;
	tab = "23456789abcdefghijkmnpqrstuvwxyz";
	while (nsrc >= 5) {
		j = (0x1f & (src[0] >> 3));
		*dest++ = tab[j];
		j = (0x1c & (src[0] << 2)) | (0x03 & (src[1] >> 6));
		*dest++ = tab[j];
		j = (0x1f & (src[1] >> 1));
		*dest++ = tab[j];
		j = (0x10 & (src[1] << 4)) | (0x0f & (src[2] >> 4));
		*dest++ = tab[j];
		j = (0x1e & (src[2] << 1)) | (0x01 & (src[3] >> 7));
		*dest++ = tab[j];
		j = (0x1f & (src[3] >> 2));
		*dest++ = tab[j];
		j = (0x18 & (src[3] << 3)) | (0x07 & (src[4] >> 5));
		*dest++ = tab[j];
		j = (0x1f & (src[4]));
		*dest++ = tab[j];
		src += 5;
		nsrc -= 5;
	}
	if (nsrc) {
		j = (0x1f & (src[0] >> 3));
		*dest++ = tab[j];
		j = (0x1c & (src[0] << 2));
		if (nsrc == 1) {
			goto out;
		}
		j |= (0x03 & (src[1] >> 6));
		*dest++ = tab[j];
		j = (0x1f & (src[1] >> 1));
		if (nsrc == 2) {
			goto out;
		}
		*dest++ = tab[j];
		j = (0x10 & (src[1] << 4));
		if (nsrc == 3) {
			goto out;
		}
		j |= (0x0f & (src[2] >> 4));
		*dest++ = tab[j];
		j = (0x1e & (src[2] << 1));
		if (nsrc == 4) {
			goto out;
		}
		j |= (0x01 & (src[3] >> 7));
		*dest++ = tab[j];
		j = (0x1f & (src[3] >> 2));
		*dest++ = tab[j];
		j = (0x18 & (src[3] << 3));
	out:
		*dest++ = tab[j];
	}
	*dest = 0;
	return dest - start;
}

enum { INVAL = 255 };

static uchar t64d[256] = {
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, 62,
    INVAL, INVAL, INVAL, 63,    52,    53,    54,    55,    56,    57,    58,
    59,    60,    61,    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, 0,
    1,     2,     3,     4,     5,     6,     7,     8,     9,     10,    11,
    12,    13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
    23,    24,    25,    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, 26,    27,
    28,    29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
    39,    40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
    50,    51,    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL};
static char t64e[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int dec64(uchar *out, int lim, char *in, int n) {
	ulong  b24;
	uchar *start = out;
	uchar *e = out + lim;
	int    i, c;

	b24 = 0;
	i = 0;
	while (n-- > 0) {

		c = t64d[*(uchar *)in++];
		if (c == INVAL) {
			continue;
		}
		switch (i) {
		case 0:
			b24 = c << 18;
			break;
		case 1:
			b24 |= c << 12;
			break;
		case 2:
			b24 |= c << 6;
			break;
		case 3:
			if (out + 3 > e) {
				goto exhausted;
			}

			b24 |= c;
			*out++ = b24 >> 16;
			*out++ = b24 >> 8;
			*out++ = b24;
			i = -1;
			break;
		}
		i++;
	}
	switch (i) {
	case 2:
		if (out + 1 > e) {
			goto exhausted;
		}
		*out++ = b24 >> 16;
		break;
	case 3:
		if (out + 2 > e) {
			goto exhausted;
		}
		*out++ = b24 >> 16;
		*out++ = b24 >> 8;
		break;
	}
exhausted:
	return out - start;
}

int enc64(char *out, int lim, uchar *in, int n) {
	int   i;
	ulong b24;
	char *start = out;
	char *e = out + lim;

	for (i = n / 3; i > 0; i--) {
		b24 = (*in++) << 16;
		b24 |= (*in++) << 8;
		b24 |= *in++;
		if (out + 4 >= e) {
			goto exhausted;
		}
		*out++ = t64e[(b24 >> 18)];
		*out++ = t64e[(b24 >> 12) & 0x3f];
		*out++ = t64e[(b24 >> 6) & 0x3f];
		*out++ = t64e[(b24) & 0x3f];
	}

	switch (n % 3) {
	case 2:
		b24 = (*in++) << 16;
		b24 |= (*in) << 8;
		if (out + 4 >= e) {
			goto exhausted;
		}
		*out++ = t64e[(b24 >> 18)];
		*out++ = t64e[(b24 >> 12) & 0x3f];
		*out++ = t64e[(b24 >> 6) & 0x3f];
		*out++ = '=';
		break;
	case 1:
		b24 = (*in) << 16;
		if (out + 4 >= e) {
			goto exhausted;
		}
		*out++ = t64e[(b24 >> 18)];
		*out++ = t64e[(b24 >> 12) & 0x3f];
		*out++ = '=';
		*out++ = '=';
		break;
	}
exhausted:
	*out = 0;
	return out - start;
}

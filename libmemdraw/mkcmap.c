#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>

/*
struct Memcmap
{
	uchar	cmap2rgb[3*256];
	uchar	rgb2cmap[16*16*16];
};
*/

static Memcmap *mkcmap(void) {
	static Memcmap def;

	int i, rgb, r, g, b;

	for (i = 0; i < 256; i++) {
		rgb = cmap2rgb(i);
		r = (rgb >> 16) & 0xff;
		g = (rgb >> 8) & 0xff;
		b = rgb & 0xff;
		def.cmap2rgb[3 * i] = r;
		def.cmap2rgb[3 * i + 1] = g;
		def.cmap2rgb[3 * i + 2] = b;
	}

	for (r = 0; r < 16; r++) {
		for (g = 0; g < 16; g++) {
			for (b = 0; b < 16; b++) {
				def.rgb2cmap[r * 16 * 16 + g * 16 + b] =
				    rgb2cmap(r * 0x11, g * 0x11, b * 0x11);
			}
		}
	}
	return &def;
}

void main(int argc, char **argv) {
	Memcmap *c;
	int      i, j, inferno;

	inferno = 0;
	ARGBEGIN {
	case 'i':
		inferno = 1;
	}
	ARGEND

	memimageinit();
	c = mkcmap();
	if (!inferno) {
		printf("#include <u.h>\n#include <libc.h>\n");
	} else {
		printf("#include \"lib9.h\"\n");
	}
	printf("#include <draw.h>\n");
	printf("#include <memdraw.h>\n\n");
	printf("static Memcmap def = {\n");
	printf("/* cmap2rgb */ {\n");
	for (i = 0; i < sizeof(c->cmap2rgb);) {
		printf("\t");
		for (j = 0; j < 16; j++, i++) {
			printf("0x%2.2ux,", c->cmap2rgb[i]);
		}
		printf("\n");
	}
	printf("},\n");
	printf("/* rgb2cmap */ {\n");
	for (i = 0; i < sizeof(c->rgb2cmap);) {
		printf("\t");
		for (j = 0; j < 16; j++, i++) {
			printf("0x%2.2ux,", c->rgb2cmap[i]);
		}
		printf("\n");
	}
	printf("}\n");
	printf("};\n");
	printf("Memcmap *memdefcmap = &def;\n");
	printf("void _memmkcmap(void){}\n");
	exits(0);
}

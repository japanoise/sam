#include "sam.h"

#include <libgen.h>
#include <stdint.h>

List	 file = {'p'};
uint16_t tag;

File	*newfile(void) {
	   File *f;

	   f = fileopen();
	   inslist(&file, 0, (int64_t)f);
	   f->tag = tag++;
	   if (downloaded) {
		   outTs(Hnewname, f->tag);
	   }
	   /* already sorted; file name is "" */
	   return f;
}

int whichmenu(File *f) {
	int i;

	for (i = 0; i < file.nused; i++) {
		if (file.filepptr[i] == f) {
			return i;
		}
	}
	return -1;
}

void delfile(File *f) {
	int w = whichmenu(f);

	if (w < 0) { /* e.g. x/./D */
		return;
	}
	if (downloaded) {
		outTs(Hdelname, f->tag);
	}
	dellist(&file, w);
	fileclose(f);
}

void fullname(String *name) {
	if (name->n > 0 && name->s[0] != '/' && name->s[0] != 0) {
		Strinsert(name, &wd, (Posn)0);
	}
}

void fixname(String *name) {
	String *t;
	char   *s;

	fullname(name);
	s = Strtoc(name);
	if (strlen(s) > 0) {
		s = cleanname(s);
	}
	t = tmpcstr(s);
	Strduplstr(name, t);
	free(s);
	freetmpstr(t);

	if (Strispre(&wd, name)) {
		Strdelete(name, 0, wd.n);
	}
}

void sortname(File *f) {
	int i, cmp, w;
	int dupwarned;

	w = whichmenu(f);
	dupwarned = false;
	dellist(&file, w);
	if (f == cmd) {
		i = 0;
	} else {
		for (i = 0; i < file.nused; i++) {
			cmp = Strcmp(&f->name, &file.filepptr[i]->name);
			if (cmp == 0 && !dupwarned) {
				dupwarned = true;
				warn_S(Wdupname, &f->name);
			} else if (cmp < 0 && (i > 0 || cmd == 0)) {
				break;
			}
		}
	}
	inslist(&file, i, f);
	if (downloaded) {
		outTsS(Hmovname, f->tag, &f->name);
	}
}

void state(File *f, int cleandirty) {
	if (f == cmd) {
		return;
	}
	f->unread = false;
	if (downloaded && whichmenu(f) >= 0) { /* else flist or menu */
		if (f->mod && cleandirty != Dirty) {
			outTs(Hclean, f->tag);
		} else if (!f->mod && cleandirty == Dirty) {
			outTs(Hdirty, f->tag);
		}
	}
	if (cleandirty == Clean) {
		f->mod = false;
	} else {
		f->mod = true;
	}
}

File *lookfile(String *s, bool fuzzy) {
	int    i;
	File  *b = NULL;
	String t;
	bool   esc = false;
	Strinit(&t);

	for (size_t i = 0; i < s->n; i++) {
		if (esc) {
			esc = false;
		} else if (s->s[i] == L'\\') {
			esc = true;
			continue;
		}
		Straddc(&t, s->s[i]);
	}

	char *sc = Strtoc(&t);

	for (i = 0; i < file.nused; i++) {
		if (Strcmp(&file.filepptr[i]->name, s) == 0) {
			return file.filepptr[i];
		}

		if (fuzzy) {
			char *ac = Strtoc(&file.filepptr[i]->name);
			if (strcmp(basename(sc), ac) == 0) {
				return free(sc), free(ac), file.filepptr[i];
			}

			if (!b && strstr(ac, sc)) {
				b = file.filepptr[i];
			}
			free(ac);
		}
	}

	Strclose(&t);
	free(sc);
	return b;
}

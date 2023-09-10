# sam documentation

* `sam.1` is the manpage for sam, B, etc.
* `samrc.5` is the manpage for sam's configuration file.
* `samrc` is a sample configuration file.
* `sam.ps` is the sam whitepaper by Rob Pike. It can be turned into a pdf with
  the command `ps2pdf`: `ps2pdf sam.ps` will produce a file `sam.pdf`.
* `se.ps` is Structural Regular Expressions by Rob Pike, which is a shorter
  paper that explains the SRE, the fundamental unit of the sam command
  language. It can be turned into a pdf the same way.
* `sam.tut.ms` is a good tutorial of sam's command language. It's useful even if
  you'd rather use acme instead - [check out my fork][acme2k] if so! You can
  turn the file into a ps file using troff and then a pdf with ps2pdf, but I've
  converted it into the gods' own format (80-column markdown) so, if you're
  looking at this on GitHub, you can [read it right here](sam_tut.md).

[acme2k]: https://github.com/japanoise/acme2k

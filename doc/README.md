# sam documentation

* `sam.1` is the manpage for sam, B, etc.
* `ssam.1` is the manpage for ssam, stream interface to sam
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

## Miscellaneous Hints & Tips

[From here](https://github.com/deadpixi/sam/issues/63) - thanks github user
Screwtapello

* sam lets you configure a font for drawing text, and a colour to draw it
  with. However, due to the way sam draws selected text, you should set the
  foreground colour to black OR use a font without anti-aliasing. If you don't
  do at least one of those, selected text will be rainbow gibberish.
* If you're using a scalable font, you can force sam to draw it without
  anti-aliasing by adding `:antialias=0` to the end of the font pattern: `font
  Envy Code R:size=10:antialias=0`
* If the font you want to use looks terrible without anti-aliasing because it
  does not include rasterization hints, applying FreeType's autohinter by adding
  `:autohint=1` may help.
* If you're using a bitmap-based font, it may include a separate copy limited to
  Latin1 (ISO8859-1) encoding for legacy applications. Because sam doesn't do
  glyph scavenging, it's better to use a font with as many glyphs as
  possible. There's no FontConfig syntax for "give me a font in Unicode", but
  apparently "give me a font that supports Afrikaans" is enough to nudge it
  toward a Unicode font: `:lang=af`

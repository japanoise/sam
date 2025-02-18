# TODO: Future plans for sam

Things have changed for me a bit, and I will be using sam as my primary editor
on OpenBSD.  This gives me the motivation to fully work on sam.  You may have
noticed I've started opening issues, commenting on old ones, and even closing
(fixing!) some of them.

My high-level goals are:

- sam becomes a viable alternative to my existing terminal editor (emsys).
- sam feels at least a little like it fits in with a Unix system, rather than a
  shim around a plan9 program, while not betraying what it is.
- sam is super customizable
- sam is easy to work on and has a higher bus factor, so it doesn't require
  obsessives (like me) to make new patches. Ideally, this will involve
  documenting everything and commenting the code more thoroughly, as well as
  where possible/desirable, making things a bit more verbose so they're easier
  to understand when coming into them cold.
- This version of sam is at least as good as the one in plan9port, and
  integrates enhancements from there and from 9front where possible.
- Get this version of sam into aur, ports, etc. and become the new primary
  maintainer.

Right now the priority is in fixing things that have been feature requests or
bugs for a while.  I've already merged the PRs that existed in the existing repo
and started on some of the longer-term issues.  By working on well-understood
things, I hope to get a familiarity with and comfort with the codebase.

The current (25-02-18) plan is:

- Fix the scroll issues - should get me familiar with drawing code & how
  commands are defined
- Add redo - will be a relatively simple change (redo is just undo in reverse) &
  should allow me to get very familiar with both commands & the data structures
  of the editor.
- Look into selection colors - will give me an opportunity to update the drawing
  code & allow me to look into customization.

## Rob's old TODO

The unfinished primary goals were:

- Keybindings:
    - nextlayer / prevlayer
    - maximize / tile left / tile right
- Support Unicode beyond the Basic Multilingual Plane
- Support font fallback
- Support mouse button reassignment
- Support runtime mouse button reassignment
- Support the CDPATH environment variable for the `cd` command
- Create RPMs, DEBs, etc
- Add localization support
- Add a Desktop Entry file, icon, etc
- Refactor all code to be as clean and standards-compliant as possible; remove
  all legacy code
- Compile with no warnings, with all and extra warnings and `-pedantic` enabled
  on GCC in C99 mode

### Primary Goals

- Scalable font support (DONE)
- Support big- and little-endian 64-bit systems (DONE)
- Support compilation and use on modern \*nix systems (DONE)
- Support two-button mice (DONE)
- Support tab expansion (DONE)
- Support runtime configuration of tab sizes (DONE)
- Support scroll wheels on mice (DONE)
- Support fuzzy matching in the `b` command (DONE)
- Raise the window when opening a new file (DONE)
- Support a configurable set of keybindings (i.e. rework the keyboard layer) (DONE)
- Support multiple background colors at once (DONE)
- Support spaces in filenames (DONE)
- Support the following commands for keybindings
    - escape (DONE)
    - scrollup / scrolldown (DONE)
    - charright / charleft (DONE)
    - lineup / linedown (DONE)
    - jump to/from command window (DONE)
    - delword / delbol / del / delbs (DONE)
    - snarf / cut / paste / exchange (DONE)
    - write (DONE)
    - nextlayer / prevlayer (TODO)
    - maximize / tile left / tile right (TODO, also looking into acme-like tiling)
    - look (DONE)
    - /regex (DONE)
    - send (DONE)
    - eol / bol (DONE)
- Support a configurable scroll factor;
  scrolling is a bit drastic now (DONE)
- Support Unicode beyond the Basic Multilingual Plane (TODO, possibly making sam agnostic about encoding)
- Support font fallback (TODO)
- Allow runtime configuration of key bindings (DONE)
- Support a configurable set of mouse chords (DONE)
- Support runtime configuration of mouse chords (DONE)
- Support mouse button reassignment (TODO)
- Support runtime mouse button reassignment (TODO)
- Remove external command FIFO, switch to UNIX domain sockets for IPC
  (email me if you want to know why I think this is a good idea) (DONE)
- Support the CDPATH environment variable for the `cd` command (TODO)
- Split the man page into documentation for `samterm`, `sam`, `keyboard`, and `samrc` (DONE)
- Add localization support (TODO)
- Add a Desktop Entry file, icon, etc (TODO)
- Create RPMs, DEBs, etc (TODO)
- Refactor all code to be as clean and standards-compliant as possible;
  remove all legacy code (TODO)
- Compile with no warnings,
  with all and extra warnings and `-pedantic` enabled on GCC in C99 mode (TODO)
- Run with no Valgrind-detected leaks or errors (DONE)

### Stretch Goals

- Remove Xt dependency (TODO)
- Switch to a more X11-y model (e.g. one child window per layer) (TODO)
- Shell windows (TODO)

### Very Unlikely Goals

- Windows port (no, seriously, stop laughing)
- Non-X11 Mac OS X port
- Console port

### Permissible Changes in Maintenance Mode

Once the above goals are met, the only changes that will be made to sam are:

- Bugfixes
- Translation updates
- Binary package updates
- Updates necessary to keep sam compiling on whatever systems its users are using

### Things That Won't Ever Happen (Sorry)

- Syntax highlighting
- Multiple cursors
- Complex text rendering (I really am sorry about this one; I want speakers of
  languages with more complex writing systems to use sam, but getting it to work
  would be nigh impossible)

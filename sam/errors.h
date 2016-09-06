/* Copyright (c) 1998 Lucent Technologies - All rights reserved. */
typedef enum Err{
    /* error_s */
    Eopen,
    Ecreate,
    Emenu,
    Emodified,
    Eio,
    /* error_c */
    Eunk,
    Emissop,
    Edelim,
    /* error */
    Efork,
    Eintr,
    Eaddress,
    Esearch,
    Epattern,
    Enewline,
    Eblank,
    Enopattern,
    EnestXY,
    Enolbrace,
    Enoaddr,
    Eoverlap,
    Enosub,
    Elongrhs,
    Ebadrhs,
    Erange,
    Esequence,
    Eorder,
    Enoname,
    Eleftpar,
    Erightpar,
    Ebadclass,
    Ebadregexp,
    Eoverflow,
    Enocmd,
    Epipe,
    Enofile,
    Etoolong,
    Echanges,
    Eempty,
    Efsearch,
    Emanyfiles,
    Elongtag,
    Esubexp,
    Etmpovfl,
    Eappend
}Err;
typedef enum Warn{
    /* warn_s */
    Wdupname,
    Wfile,
    Wdate,
    /* warn_ss */
    Wdupfile,
    /* warn */
    Wnulls,
    Wpwd,
    Wnotnewline,
    Wbadstatus
}Warn;

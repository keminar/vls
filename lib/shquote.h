

#define CTLESC '\001'
#define CTLNUL '\177'

extern char *
sh_single_quote (const char *string);

extern char *
sh_backslash_quote (char *string);

int
sh_contains_shell_metas (char *string);
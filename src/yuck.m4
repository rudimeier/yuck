changequote([,])dnl
divert([-1])

## this is a little domain language for the yuck processor

# foreachq(x, [item_1, item_2, ..., item_n], stmt)
#   quoted list, alternate improved version
define([foreachq], [ifelse([$2], [], [],
	[pushdef([$1])_$0([$1], [$3], [], $2)popdef([$1])])])
define([_foreachq], [ifelse([$#], [3], [],
	[define([$1], [$4])$2[]$0([$1], [$2],
		shift(shift(shift($@))))])])

define([append], [dnl
	define([$1], ifdef([$1], [defn([$1])[$3]])[$2])
])
define([append_ne], [dnl
	## like append, but don't append empty arguments
	ifelse([$2], [], [], [append([$1], [$2], [$3])])
])

define([first_nonnil], [dnl
	ifelse([$#], [0], [], [$1], [], [first_nonnil(shift($@))], [], [], [$1])
])


define([yuck_set_umbrella], [dnl
	define([YUCK_UMB], [$1])
])
define([yuck_add_command], [dnl
	pushdef([cmd], [$1])
	pushdef([str], [ifelse([$#], [2], [$2], [$1])])
	append([YUCK_CMD], cmd, [,])
	define([YUCK_STR.]cmd, str)
	popdef([str])
	popdef([cmd])
])

## yuck_add_option(short, long, type, [CMD])
define([yuck_add_option], [dnl
	pushdef([short], [$1])
	pushdef([long], [$2])
	pushdef([type], [$3])
	pushdef([cmd], [$4])

	pushdef([ident], ifelse(long, [], ifelse(short, [], [define([cnt], ifdef([cnt], [incr(cnt)], [0]))[s]cnt], [dash]short), long))
	pushdef([slot], ifelse(type, [], [ident], [ident[_]type]))

	append_ne([YUCK.]cmd[.S], short, [,])
	append_ne([YUCK.]cmd[.L], long, [,])
	append_ne([YUCK.]cmd[.I], ident, [,])

	define([YUCK.]cmd[.]short[.slot], slot)
	define([YUCK.]cmd[.]long[.slot], slot)
	define([YUCK.]cmd[.]ident[.slot], slot)
	define([YUCK.]cmd[.]short[.type], type)
	define([YUCK.]cmd[.]long[.type], type)
	define([YUCK.]cmd[.]ident[.type], type)

	popdef([ident])
	popdef([slot])
	popdef([cmd])
	popdef([type])
	popdef([long])
	popdef([short])
])

## helpers for the m4c and m4h

## yuck_slot_decl([option], [[cmd]])
define([yuck_slot_decl], [dnl
pushdef([opt], [$1])dnl
pushdef([cmd], [$2])dnl
pushdef([type], defn([YUCK.]cmd[.]opt[.type]))dnl
dnl
pushdef([ctype],
	ifelse(type, [], [void ],
		type, [flag], [unsigned int ],
		type, [arg], [const char *],
		type, [marg], [const char **]))dnl
dnl
ctype[]yuck_slot_identifier(opt, cmd)dnl
dnl
popdef([ctype])dnl
popdef([type])dnl
popdef([cmd])dnl
popdef([opt])dnl
])

## yuck_slot_identifier([option], [[cmd]])
define([yuck_slot_identifier], [dnl
pushdef([opt], [$1])dnl
pushdef([cmd], [$2])dnl
dnl
defn([YUCK.]cmd[.]opt[.slot])dnl
dnl
popdef([cmd])dnl
popdef([opt])dnl
])

## yuck_slot([option], [[cmd]])
define([yuck_slot], [dnl
pushdef([opt], [$1])dnl
pushdef([cmd], [$2])dnl
pushdef([idn], [yuck_slot_identifier(opt, cmd)])dnl
dnl
ifelse(cmd, [], [idn], [cmd.idn])[]dnl
dnl
popdef([idn])dnl
popdef([cmd])dnl
popdef([opt])dnl
])

## yuck_iftype([opt], [cmd], [type], [body], [[type], [body]]...)
define([yuck_iftype], [dnl
pushdef([opt], [$1])dnl
pushdef([cmd], [$2])dnl
pushdef([type], defn([YUCK.]cmd[.]opt[.type]))dnl
popdef([cmd])dnl
popdef([opt])dnl
[]ifelse(_splice(type, shift(shift($@))))[]dnl
popdef([type])dnl
])
define([_splice], [ifelse([$#], [3], [[$1], [$2], [$3]], [[$1], [$2], [$3], _splice([$1], shift(shift(shift($@))))])])

## yuck_cmds()
define([yuck_cmds], [[YUCK_CMD]])

## yuck_cmd
define([yuck_cmd], [YUCK_UMB[_]ifelse([$1], [], [NONE], [$1])])

## yuck_cmd_string
define([yuck_cmd_string], [defn([YUCK_STR.]$1)])

## yuck_shorts([cmd])
define([yuck_shorts], [defn([YUCK.]$1[.S])])

## yuck_longs([cmd])
define([yuck_longs], [defn([YUCK.]$1[.L])])

## yuck_idents([cmd])
define([yuck_idents], [defn([YUCK.]$1[.I])])


## test case
yuck_set_umbrella([yuck])
yuck_add_option([], [help], [flag])
yuck_add_command([gen])
yuck_add_option([x], [extra], [arg], [gen])
yuck_add_option([h], [], [flag], [gen])
yuck_add_option([], [version], [flag], [gen])
yuck_add_command([tst])
yuck_add_option([], [version], [flag], [tst])
yuck_add_option([], [], [flag], [tst])
yuck_add_option([], [], [flag], [tst])


## coroutine stuff
define([yield], [goto $1; back_from_$1:])
define([coroutine], [define([this_coru], [$1])$1:])
define([resume], [goto back_from_[]this_coru])
define([resume_at], [goto $1])
define([quit], [goto out])

divert[]dnl
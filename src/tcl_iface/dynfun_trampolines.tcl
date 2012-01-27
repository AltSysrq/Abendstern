#! /usr/bin/tclsh

set create [open "dynfun_create_trampolines.hxx" w]
set list   [open "dynfun_list_trampolines.hxx"   w]
for {set i 0} {$i < 16} {incr i} {
  puts $create "static R tramp${i}(
  #undef CURR_ARG
  #define CURR_ARG 0
  #define DECLARE_FUNCTION_ARGUMENTS
  #include \"dynfun.hxx\"
  ) { return impls\[$i\]->invoke(
    #undef CURR_ARG
    #define CURR_ARG 0
    #define LIST_FUNCTION_ARGUMENTS
    #include \"dynfun.hxx\"
    ); }\n"
  puts $list "tramp$i, "
}
close $create
close $list


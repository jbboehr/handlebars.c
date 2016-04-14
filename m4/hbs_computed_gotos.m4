
AC_DEFUN([HBS_COMPUTED_GOTOS],[dnl
AC_MSG_CHECKING([if ${CC-gcc} supports computed gotos])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
    [],
    [[
      void *my_label_ptr = &&my_label; /* GCC syntax */
      goto *my_label_ptr;
      return 1;
      my_label:
      return 0;
    ]])],
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_COMPUTED_GOTOS, 1,
     [Define to 1 if the compiler supports computed gotos])],
  [AC_MSG_RESULT(no)])
])

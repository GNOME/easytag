dnl Checks if a particular URI appears in the XML catalog
dnl Usage:
dnl	JH_CHECK_XML_CATALOG(URI, [FRIENDLY-NAME], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
AC_DEFUN([JH_CHECK_XML_CATALOG],
[
	AC_REQUIRE([JH_PATH_XML_CATALOG],[JH_PATH_XML_CATALOG(,[:])])dnl
	AC_MSG_CHECKING([for ifelse([$2],,[$1],[$2]) in XML catalog])
	if $jh_found_xmlcatalog && \
		AC_RUN_LOG([$XMLCATALOG --noout "$XML_CATALOG_FILE" "$1" >&2]); then
		AC_MSG_RESULT([found])
		ifelse([$3],,,[$3])
	else
		AC_MSG_RESULT([not found])
		ifelse([$4],,[AC_MSG_ERROR([could not find ifelse([$2],,[$1],[$2]) in XML catalog])],[$4])
	fi
])

dnl Checks the location of the XML Catalog
dnl Usage:
dnl	JH_PATH_XML_CATALOG([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Defines XMLCATALOG and XML_CATALOG_FILE substitutions
AC_DEFUN([JH_PATH_XML_CATALOG],
[
	dnl check for the presence of the XML catalog
	AC_ARG_WITH([xml-catalog],
		AS_HELP_STRING([--with-xml-catalog=CATALOG],
		[path to xml catalog to use]),,
		[with_xml_catalog=/etc/xml/catalog])
	jh_found_xmlcatalog=true
	XML_CATALOG_FILE="$with_xml_catalog"
	AC_SUBST([XML_CATALOG_FILE])
	AC_MSG_CHECKING([for XML catalog ($XML_CATALOG_FILE)])
	if test -f "$XML_CATALOG_FILE"; then
		AC_MSG_RESULT([found])
	else
		jh_found_xmlcatalog=false
		AC_MSG_RESULT([not found])
	fi

	dnl check for the xmlcatalog program
	AC_PATH_PROG(XMLCATALOG, xmlcatalog, no)
	if test "x$XMLCATALOG" = xno; then
		jh_found_xmlcatalog=false
	fi

	if $jh_found_xmlcatalog; then
		ifelse([$1],,[:],[$1])
	else
		ifelse([$2],,[AC_MSG_ERROR([could not find XML catalog])],[$2])
	fi
])

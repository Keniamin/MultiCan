#ifndef _MULTICAN_STRINGS_H
#define _MULTICAN_STRINGS_H

#define STR_MENU_FILE "&����"
#define STR_MENU_EDIT "&������"
#define STR_MENU_CALC "&������"
#define STR_MENU_HELP "��&����"


#define STR_MENUITEM_NEW "&�������\tCtrl+N"
#define STR_MENUITEM_OPEN "&�������...\tCtrl+O"
#define STR_MENUITEM_SAVE "��&�������\tCtrl+S"
#define STR_MENUITEM_SAVEAS "��������� &���...\tShift+Ctrl+S"

#define STR_MENUITEM_EXIT "&�����\tAlt+F4"


#define STR_MENUITEM_ADDFACT "�������� &��������..."
#define STR_MENUITEM_DELFACT "������� �&�������..."

#define STR_MENUITEM_ADDSET "�������� &�������..."
#define STR_MENUITEM_DELSET "������� ��&�����..."


#define STR_MENUITEM_CANAN "&������������ ������\tCtrl+1"

#define STR_MENUITEM_CHDECSEP "&������� ���������� �����������"


#define STR_MENUITEM_REGEXT "&������������� � ������� .mcd � .mcr"
#define STR_MENUITEM_HELP "&������� �� MultiCan\tF1"
#define STR_MENUITEM_ABOUT "&� ���������..."


static const char STR_TAB_FACT[] = "��������";
static const char STR_TAB_SET[] = "�������";
static const char STR_TAB_COR[] = "�������������� �������";

static const char STR_TAB_VAR[] = "������������ ����������";
static const char STR_TAB_STD[] = "����������������� ������������ ��������������� �������";
static const char STR_TAB_PNT[] = "��������� �������";
static const char STR_TAB_DIST[] = "������� ���������� ������������";


static const char STR_MSG_WANTSAVE[] = "������� ���� �� ��������!\n������ ��������� ��������?";
static const char STR_MSG_BADVAL[] = "������ �� ����� ���� ��������! ����� ������� ������ ���������� ��������, �� ���������� ���������� ������.";
static const char STR_MSG_NOINFO[] = "������������ ���������� ��� �������! ���������� ������� ������ ���� �� ������ 2, � ���������� �������� ��������� �� ������ 1.";
static const char STR_MSG_OPENERROR[] = "���������� ������� ����! ��������, ���� ���������� ��� ����� ����������� ������.\n��������� � ������������ ����� � ��������� ������� �����.";
static const char STR_MSG_SAVEERROR[] = "���������� ��������� ������! ��������, ���� ���������� ��� ������� �� ������.\n��������� ������ ��� ������ ������ ��� ��������� ������� �����.";


static const char STR_COL_NAME[] = "���";
static const char STR_COL_SETVOL[] = "�����\n�������";
static const char STR_COL_SETMARK[] = "�����\n(�����)";
static const char STR_COL_FACTIVE[] = "��������\n������� �\n������";
static const char STR_COL_STDDEV[] = "�����������\n��������.\n����������";

static const char STR_ROW_CONST[] = "���������";
static const char STR_ROW_EIGVAL[] = "�����������\n��������";
static const char STR_ROW_CUMPROP[] = "% �����������\n������������";

static const char STR_HDR_MEANBYFACT[] = "������� ��\n�������� ";
static const char STR_HDR_VAR[] = "���������� ";
static const char STR_HDR_FACT[] = "������� ";
static const char STR_HDR_SET[] = "������� ";


static const char STR_FILTER_NOEXT[] = "��� �����\0*.*\0";
static const char STR_FILTER_ALLEXT[] = "����� MultiCan (.mcd, .mcr)\0*.mcd;*.mcr\0";
static const char STR_FILTER_DATAEXT[] = "������ ��� ������� MultiCan (.mcd)\0*.mcd\0";
static const char STR_FILTER_RESEXT[] = "���������� ������������� ������� MultiCan (.mcr)\0*.mcr\0";

static const char STR_RESEXT[] = "mcr";
static const char STR_DATAEXT[] = "mcd";

static const char STR_DATAEXT_REGTYPE[] = "MultiCan.Data";
static const char STR_RESEXT_REGTYPE[] = "MultiCan.Result";
static const char STR_DATAEXT_REGDESC[] = "���� ������ MultiCan";
static const char STR_RESEXT_REGDESC[] = "���� ����������� MultiCan";


static const char STR_TITLE_NOFILE[] = "����������";
static const char STR_TITLE_MAINWINDOW[] = "MultiCan";
static const char STR_TITLE_ABOUTDLG[] = "� ���������";
static const char STR_TITLE_ADDDLG[] = "��������";
static const char STR_TITLE_DELDLG[] = "�������";

static const char STR_TITLE_PATTERNERROR[] = "������ �����";


static const char STR_DLG_ADDSET[] = "&���������� ����������� �������";
static const char STR_DLG_ADDFACT[] = "&���������� ����������� ���������";

static const char STR_DLG_ADDPATTERN[] = "0123456789";
static const char STR_DLG_ADDERROR[] = "������� ����� �� 1 �� 999";


static const char STR_DLG_DELSET[] = "&������ ��������� �������\n(������ ����� ��� ����������)";
static const char STR_DLG_DELFACT[] = "&������ ��������� ���������\n(������ ����� ��� ����������)";

static const char STR_DLG_DELPATTERN[] = "0123456789;,-";
static const char STR_DLG_DELERROR[] = "������� ������ ��� ���������, �������� \"1-7,12,25-26\"";

static const char STR_INVALID_INTERVAL[] = "������������ ������ ���������! ��������� ��������� ����� ��� ��� ����� ����� ����.\n��������� ��������: ";
static const char STR_INVALID_INTERVAL_CHAR[] = "������������ ������ � ���������! ������� ���������� ������ ���� �������������� �������.\n��������� ��������: ";
static const char STR_INVALID_INTERVAL_ORDER[] = "������������ ������� ����� � ���������! ����� ��������� �� ������ ���� ������ ������.\n��������� ��������: ";
static const char STR_INTERVAL_OVERFLOW[] = "������������ ��������! ������� ��������� ������ ���� �� ������� �� ���������� ��������.\n��������� ��������: ";
static const char STR_INTERVALS_OVERLAP[] = "�������� ��������� ������������! ����������, ������� ����������� ���������.";


static const char STR_DLG_ABOUT[] = "\
��������� ������������ �� �������� \"as is\" (\"��� ����\"), ��� ��������������� �� ����������� � ���������� ����� �� �������� ������������.\r\n\r\n\
���������� �����������, ���������� � �������������� ���������, �������� ������ � ��������� ����� � ����������! ��������� �������� � �������� �������� � ������������ (����� ���� \"�������\").\r\n\r\n\
� ������ ��������� ����� ������ �������� �.�. �������� \"��������\" � \"��������\" (1983-1995 ����).\r\n\r\n\
����������: �.�. ��������\r\n\
(inngoncharov@yandex.ru),\r\n\
2013 ���.\r\n\r\n\
������������� � ����������� ���� �� �� �2016610803, ������, 2016 ���.";


static const char STR_HELP_DIRNAME[] = "MultiCan\\";
static const char STR_HELP_FILENAME[] = "Help.chm";
static const char STR_VERSION[] = "1.2";

#endif

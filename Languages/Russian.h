#ifndef _MULTICAN_STRINGS_H
#define _MULTICAN_STRINGS_H

#define STR_MENU_FILE "&Файл"
#define STR_MENU_EDIT "&Правка"
#define STR_MENU_CALC "&Расчет"
#define STR_MENU_HELP "По&мощь"


#define STR_MENUITEM_NEW "&Создать\tCtrl+N"
#define STR_MENUITEM_OPEN "&Открыть...\tCtrl+O"
#define STR_MENUITEM_SAVE "Со&хранить\tCtrl+S"
#define STR_MENUITEM_SAVEAS "Сохранить &как...\tShift+Ctrl+S"

#define STR_MENUITEM_EXIT "&Выход\tAlt+F4"


#define STR_MENUITEM_ADDFACT "Добавить &признаки..."
#define STR_MENUITEM_DELFACT "Удалить п&ризнаки..."

#define STR_MENUITEM_ADDSET "Добавить &выборки..."
#define STR_MENUITEM_DELSET "Удалить вы&борки..."


#define STR_MENUITEM_CANAN "&Канонический анализ\tCtrl+1"

#define STR_MENUITEM_CHDECSEP "&Сменить десятичный разделитель"


#define STR_MENUITEM_REGEXT "&Ассоциировать с файлами .mcd и .mcr"
#define STR_MENUITEM_HELP "&Справка по MultiCan\tF1"
#define STR_MENUITEM_ABOUT "&О программе..."


static const char STR_TAB_FACT[] = "Признаки";
static const char STR_TAB_SET[] = "Выборки";
static const char STR_TAB_COR[] = "Корреляционная матрица";

static const char STR_TAB_VAR[] = "Канонические переменные";
static const char STR_TAB_STD[] = "Стандартизованные коэффициенты дискриминантной функции";
static const char STR_TAB_PNT[] = "Положения выборок";
static const char STR_TAB_DIST[] = "Матрица расстояний Махаланобиса";


static const char STR_MSG_WANTSAVE[] = "Текущий файл не сохранен!\nХотите сохранить измененя?";
static const char STR_MSG_BADVAL[] = "Анализ не может быть выполнен! Среди входных данных обнаружено значение, не являющееся корректным числом.";
static const char STR_MSG_NOINFO[] = "Недостаточно информации для анализа! Количество выборок должно быть не меньше 2, а количество активных признаков не меньше 1.";
static const char STR_MSG_OPENERROR[] = "Невозможно открыть файл! Возможно, файл недоступен или имеет неизвестный формат.\nУбедитесь в корректности файла и повторите попытку позже.";
static const char STR_MSG_SAVEERROR[] = "Невозможно сохранить данные! Возможно, файл недоступен или защищен от записи.\nСохраните данные под другим именем или повторите попытку позже.";


static const char STR_COL_NAME[] = "Имя";
static const char STR_COL_SETVOL[] = "Объем\nвыборки";
static const char STR_COL_SETMARK[] = "Метка\n(число)";
static const char STR_COL_FACTIVE[] = "Включить\nпризнак в\nанализ";
static const char STR_COL_STDDEV[] = "Стандартное\nсреднекв.\nотклонение";

static const char STR_ROW_CONST[] = "Константа";
static const char STR_ROW_EIGVAL[] = "Собственное\nзначение";
static const char STR_ROW_CUMPROP[] = "% объясняемой\nизменчивости";

static const char STR_HDR_MEANBYFACT[] = "Среднее по\nпризнаку ";
static const char STR_HDR_VAR[] = "Переменная ";
static const char STR_HDR_FACT[] = "Признак ";
static const char STR_HDR_SET[] = "Выборка ";


static const char STR_FILTER_NOEXT[] = "Все файлы\0*.*\0";
static const char STR_FILTER_ALLEXT[] = "Файлы MultiCan (.mcd, .mcr)\0*.mcd;*.mcr\0";
static const char STR_FILTER_DATAEXT[] = "Данные для анализа MultiCan (.mcd)\0*.mcd\0";
static const char STR_FILTER_RESEXT[] = "Результаты канонического анализа MultiCan (.mcr)\0*.mcr\0";

static const char STR_RESEXT[] = "mcr";
static const char STR_DATAEXT[] = "mcd";

static const char STR_DATAEXT_REGTYPE[] = "MultiCan.Data";
static const char STR_RESEXT_REGTYPE[] = "MultiCan.Result";
static const char STR_DATAEXT_REGDESC[] = "Файл данных MultiCan";
static const char STR_RESEXT_REGDESC[] = "Файл результатов MultiCan";


static const char STR_TITLE_NOFILE[] = "Безымянный";
static const char STR_TITLE_MAINWINDOW[] = "MultiCan";
static const char STR_TITLE_ABOUTDLG[] = "О программе";
static const char STR_TITLE_ADDDLG[] = "Добавить";
static const char STR_TITLE_DELDLG[] = "Удалить";

static const char STR_TITLE_PATTERNERROR[] = "Ошибка ввода";


static const char STR_DLG_ADDSET[] = "&Количество добавляемых выборок";
static const char STR_DLG_ADDFACT[] = "&Количество добавляемых признаков";

static const char STR_DLG_ADDPATTERN[] = "0123456789";
static const char STR_DLG_ADDERROR[] = "Введите число от 1 до 999";


static const char STR_DLG_DELSET[] = "&Номера удаляемых выборок\n(список чисел или диапазонов)";
static const char STR_DLG_DELFACT[] = "&Номера удаляемых признаков\n(список чисел или диапазонов)";

static const char STR_DLG_DELPATTERN[] = "0123456789;,-";
static const char STR_DLG_DELERROR[] = "Введите номера или диапазоны, например \"1-7,12,25-26\"";

static const char STR_INVALID_INTERVAL[] = "Некорректный формат диапазона! Ожидается отдельное число или два числа через тире.\nОшибочное значение: ";
static const char STR_INVALID_INTERVAL_CHAR[] = "Некорректный символ в диапазоне! Границы диапазонов должны быть положительными числами.\nОшибочное значение: ";
static const char STR_INVALID_INTERVAL_ORDER[] = "Некорректный порядок чисел в диапазоне! Конец интервала не должен быть меньше начала.\nОшибочное значение: ";
static const char STR_INTERVAL_OVERFLOW[] = "Некорректный диапазон! Границы диапазона должны быть от единицы до количества объектов.\nОшибочное значение: ";
static const char STR_INTERVALS_OVERLAP[] = "Введённые диапазоны пересекаются! Пожалуйста, введите независимые диапазоны.";


static const char STR_DLG_ABOUT[] = "\
Программа поставляется на условиях \"as is\" (\"как есть\"), вся ответственность за последствия её применения лежит на конечном пользователе.\r\n\r\n\
Публикация результатов, полученных с использованием программы, возможна только с указанием факта её применения! Подробные сведения о лицензии доступны в документации (пункт меню \"Справка\").\r\n\r\n\
В основе программы лежат методы программ В.Е. Дерябина \"Простант\" и \"Каноклас\" (1983-1995 года).\r\n\r\n\
Разработка: И.А. Гончаров\r\n\
(inngoncharov@yandex.ru),\r\n\
2013 год.\r\n\r\n\
Свидетельство о регистрации прав на ПО №2016610803, Москва, 2016 год.";


static const char STR_HELP_DIRNAME[] = "MultiCan\\";
static const char STR_HELP_FILENAME[] = "Help.chm";
static const char STR_VERSION[] = "1.2";

#endif

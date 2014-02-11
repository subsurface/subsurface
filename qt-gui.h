#ifndef QT_GUI_H
#define QT_GUI_H

void init_ui(int *argcp, char ***argvp);
void init_qt_ui(int *argcp, char ***argvp, char *errormessage);

void run_ui(void);
void exit_ui(void);

#endif // QT_GUI_H

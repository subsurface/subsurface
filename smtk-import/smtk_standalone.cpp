// SPDX-License-Identifier: GPL-2.0
#include <stdlib.h>
#include <stdio.h>
#include "core/dive.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "smrtk2ssrfc_window.h"
#include <QApplication>
#include <QDebug>

/*
 * Simple command line interface to call directly smartrak_import() or launch
 * the GUI if called without arguments.
 */

int main(int argc, char *argv[])
{
	char *infile, *outfile;
	int i;
#ifndef COMMANDLINE
	QApplication a(argc, argv);
	Smrtk2ssrfcWindow w;
#else
	QCoreApplication a(argc, argv);
#endif

	switch (argc) {
	case 1:
#ifndef COMMANDLINE
		w.show();
		return a.exec();
#endif
		break;
	case 2:
		qDebug() << "\nUsage:\n";
		qDebug() << "Smrtk2ssrfc importer can be used without arguments (in a graphical UI)";
		qDebug() << "or with, at least, two arguments (in a CLI, the file to be imported and";
		qDebug() << "the file to store the Subsurface formatted dives), so you have to use one";
		qDebug() << "of these examples:\n";
		qDebug() << "$smrtk2ssrfc";
		qDebug() << "or";
		qDebug() << "$smrtk2ssrfc /input/file.slg[ file_2[ file_n]] /output/file.xml\n\n";
		break;
	default:
		outfile = argv[argc - 1];
		qDebug() << "\n[Importing]\n";
		for(i = 1; i < argc -1; i++) {
			infile = argv[i];
			qDebug() << "\t" << infile << "\n";
			smartrak_import(infile, &divelog);
		}
		qDebug() << "\n[Writing]\n\t" << outfile << "\n";
		save_dives_logic(outfile, false, false);
		break;
	}
}

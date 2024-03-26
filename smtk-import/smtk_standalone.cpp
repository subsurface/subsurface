// SPDX-License-Identifier: GPL-2.0
#include <stdlib.h>
#include <stdio.h>
#include "core/dive.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/errorhelper.h"
#include "smrtk2ssrfc_window.h"
#include <QApplication>

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
		report_info("\nUsage:\n");
		report_info("Smrtk2ssrfc importer can be used without arguments (in a graphical UI)");
		report_info("or with, at least, two arguments (in a CLI, the file to be imported and");
		report_info("the file to store the Subsurface formatted dives), so you have to use one");
		report_info("of these examples:\n");
		report_info("$smrtk2ssrfc");
		report_info("or");
		report_info("$smrtk2ssrfc /input/file.slg[ file_2[ file_n]] /output/file.xml\n\n");
		break;
	default:
		outfile = argv[argc - 1];
		report_info("\n[Importing]\n");
		for(i = 1; i < argc -1; i++) {
			infile = argv[i];
			report_info("\t%s\n", infile);
			smartrak_import(infile, &divelog);
		}
		report_info("\n[Writing]\n\t %s\n", outfile);
		save_dives_logic(outfile, false, false);
		break;
	}
}

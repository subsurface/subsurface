set(SMTK_IMPORT_SRCS
	smartrak.c
	smrtk2ssrfc_window.ui
	smrtk2ssrfc_window.h
	smrtk2ssrfc_window.cpp
)
source_group("SmartTrak Import libs" FILES ${SMTK_IMPORT_SRCS})

add_library(smtk_import STATIC ${SMTK_IMPORT_SRCS})
target_link_libraries(smtk_import subsurface_corelib ${SUBSURFACE_LINK_LIBRARIES})

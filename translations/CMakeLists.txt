# without this a "make clean" would delete the .ts files
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

# active translations - these will be built
#
set(TRANSLATION_FILES
	subsurface_source.ts
	subsurface_bg_BG.ts
	subsurface_cs.ts
	subsurface_da_DK.ts
	subsurface_de_CH.ts
	subsurface_de_DE.ts
	subsurface_en_GB.ts
	subsurface_es_ES.ts
	subsurface_et_EE.ts
	subsurface_fi_FI.ts
	subsurface_fr_FR.ts
	subsurface_it_IT.ts
	subsurface_lv_LV.ts
	subsurface_nb_NO.ts
	subsurface_nl_NL.ts
	subsurface_pl_PL.ts
	subsurface_pt_BR.ts
	subsurface_pt_PT.ts
	subsurface_ru_RU.ts
	subsurface_sk_SK.ts
	subsurface_sv_SE.ts
	subsurface_tr.ts
	subsurface_zh_TW.ts
)

# disabled translations as they are below 50%:
# 	subsurface_el_GR.ts
# 	subsurface_he.ts
# 	subsurface_hu.ts
# 	subsurface_ro_RO.ts

qt5_add_translation(TRANSLATIONS ${TRANSLATION_FILES})
set(TRANSLATIONS ${TRANSLATIONS} PARENT_SCOPE)
add_custom_target (translations ALL DEPENDS ${TRANSLATIONS})

# enable the following two lines to recreate the .ts files

# qt5_create_translation(TRANSLATION_MESSAGES ${CMAKE_SOURCE_DIR} ${TRANSLATION_FILES})
# add_custom_target(translations_update DEPENDS ${TRANSLATION_MESSAGES})

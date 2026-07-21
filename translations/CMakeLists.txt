# without this a "make clean" would delete the .ts files
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

# active translations - these will be built
#
set(TRANSLATION_FILES
	subsurface_source.ts
	subsurface_bg_BG.ts
	subsurface_ca.ts
	subsurface_cs.ts
	subsurface_da_DK.ts
	subsurface_de_CH.ts
	subsurface_de_DE.ts
	subsurface_el_GR.ts
	subsurface_en_GB.ts
	subsurface_es_ES.ts
	subsurface_et_EE.ts
	subsurface_fi_FI.ts
	subsurface_fr_FR.ts
	subsurface_he.ts
	subsurface_hr_HR.ts
	subsurface_it_IT.ts
	subsurface_ko_KR.ts
	subsurface_nb_NO.ts
	subsurface_nl_NL.ts
	subsurface_pl_PL.ts
	subsurface_pt_BR.ts
	subsurface_pt_PT.ts
	subsurface_ro_RO.ts
	subsurface_ru_RU.ts
	subsurface_sk_SK.ts
	subsurface_sv_SE.ts
	subsurface_zh_TW.ts
)

set(US_EN_PLURALS subsurface_en_US.ts)

# disabled translations as they are below 40%:
#	subsurface_hu.ts
#	subsurface_lv_LV.ts
#	subsurface_tr.ts

# new translations that didn't make it far enough:
#	subsurface_af.ts
#	subsurface_ar.ts
#	subsurface_hi_IN.ts
#	subsurface_hu.ts
#	subsurface_id.ts
#	subsurface_ja_JP.ts
#	subsurface_sl.ts
#	subsurface_sw_KE.ts
#	subsurface_te.ts
#	subsurface_th.ts
#	subsurface_vi.ts
#	subsurface_zh_CN.ts

if(USINGQT6)
	file(GLOB MOBILE_SOURCES ${CMAKE_SOURCE_DIR}/mobile-widgets/*.cpp ${CMAKE_SOURCE_DIR}/mobile-widgets/qml/*.qml)
	qt_add_translations(${SUBSURFACE_TARGET} TS_FILES ${TRANSLATION_FILES} PLURALS_TS_FILE ${US_EN_PLURALS} SOURCES ${MOBILE_SOURCES} QM_FILES_OUTPUT_VARIABLE TRANSLATIONS)
	add_custom_target(translations ALL)
	# use add_dependencies (not DEPENDS) so the name is treated as a target, not a file
	add_dependencies(translations ${PROJECT_NAME}_lrelease)
else()
	qt5_add_translation(TRANSLATIONS ${TRANSLATION_FILES} ${US_EN_PLURALS})
	add_custom_target(translations ALL DEPENDS ${TRANSLATIONS})
endif()

set(TRANSLATIONS ${TRANSLATIONS} PARENT_SCOPE)

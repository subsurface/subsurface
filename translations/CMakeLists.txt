QT5_ADD_TRANSLATION(TRANSLATIONS
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
# translations/subsurface_el_GR.ts \
# translations/subsurface_he.ts \
# translations/subsurface_hu.ts \
# translations/subsurface_ro_RO.ts \

SET(TRANSLATIONS ${TRANSLATIONS} PARENT_SCOPE)
ADD_CUSTOM_TARGET (translations ALL DEPENDS ${TRANSLATIONS})

# the data models that will interface
# with the views.
set(SUBSURFACE_MODELS_LIB_SRCS
	cleanertablemodel.cpp
	cylindermodel.cpp
	diveplannermodel.cpp
	models.cpp
	filtermodels.cpp
	tankinfomodel.cpp
	weigthsysteminfomodel.cpp
	weightmodel.cpp
	divecomputermodel.cpp
	treemodel.cpp
	yearlystatisticsmodel.cpp
	divetripmodel.cpp
	divecomputerextradatamodel.cpp
	completionmodels.cpp
	divepicturemodel.cpp
	diveplotdatamodel.cpp
	divelocationmodel.cpp
	divesitepicturesmodel.cpp
	ssrfsortfilterproxymodel.cpp
)

source_group("Subsurface Models" FILES ${SUBSURFACE_MODELS})
add_library(subsurface_models STATIC ${SUBSURFACE_MODELS_LIB_SRCS})
target_link_libraries(subsurface_models ${QT_LIBRARIES})

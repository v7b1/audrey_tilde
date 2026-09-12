
include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)

set(LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sources")
set(DAISY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/DaisySP")

include_directories( 
	"${C74_INCLUDES}"
	"${LIB_PATH}"
	"${DAISY_PATH}"
)

#add_subdirectory(${LIB_PATH})

set(SC_SOURCES
	${LIB_PATH}/BiquadFilters.cpp
	${LIB_PATH}/BiquadFilters.h
	${LIB_PATH}/DSPUtils.h
	${LIB_PATH}/EchoDelay.h
	#${LIB_PATH}/FeedbackSynthControls.cpp
	#${LIB_PATH}/FeedbackSynthControls.h
	${LIB_PATH}/FeedbackSynthEngine.cpp
	${LIB_PATH}/FeedbackSynthEngine.h
	${LIB_PATH}/KarplusString.cpp
	${LIB_PATH}/KarplusString.h
	${LIB_PATH}/SmoothedValue.h
	
)

set(DAISY_LGPL_SOURCES
	${DAISY_LGPL_PATH}/tone.cpp
	${DAISY_LGPL_PATH}/tone.h
)

set(DAISY_SOURCES
	${DAISY_PATH}/daisysp.h
	${DAISY_PATH}/Dynamics/limiter.cpp
	${DAISY_PATH}/Dynamics/limiter.h
	${DAISY_PATH}/Dynamics/crossfade.cpp
	${DAISY_PATH}/Dynamics/crossfade.h
	${DAISY_PATH}/Filters/tone.cpp
	${DAISY_PATH}/Filters/tone.h
	${DAISY_PATH}/Effects/reverbsc.cpp
	${DAISY_PATH}/Effects/reverbsc.h
	${DAISY_PATH}/Effects/overdrive.cpp
	${DAISY_PATH}/Effects/overdrive.h
	${DAISY_PATH}/Utility/dcblock.cpp
	${DAISY_PATH}/Utility/dcblock.h
	
)

add_library( 
	${PROJECT_NAME} 
	MODULE
	${PROJECT_NAME}.cpp
	${SC_SOURCES}
	${DAISY_SOURCES}
	
)



# create groups in our project
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SC_SOURCES} ${DAISY_SOURCES})

#${DAISY_LGPL_PATH}


include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)

# Variables
include settings.mk
SOURCE_DIRECTORY ?= src
OBJECT_DIRECTORY ?= obj
TARGET_DIRECTORY ?= builds
OBJECT_FILE_EXTENSION ?= .obj
TARGET_NAME ?= AVExtensions.dll
COMPILER ?= cl
FLAGS_COMPILE ?= /EHsc /O2
FLAGS_LINK ?=
OUTPUT_FLAGS ?= > NUL 2> NUL
LINK_LIBRARIES ?=
INCLUDE_FUNCTIONS ?=



# Functions
include functions.mk



# Architecture target
ifndef AVEXTENSIONS_SETUP_ARCHITECTURE
    # Setup needed
    $(error Setup has not been performed; execute `setup.bat` or `setup64.bat`, then run make again)
else
    ifeq (x86,$(AVEXTENSIONS_SETUP_ARCHITECTURE))
        # x86
    else ifeq (x64,$(AVEXTENSIONS_SETUP_ARCHITECTURE))
        # x64
        TARGET_NAME_X64_EXTENSION = -x64
        TARGET_NAME := $(join $(join $(basename $(TARGET_NAME)),$(TARGET_NAME_X64_EXTENSION)),$(suffix $(TARGET_NAME)))
        OBJECT_FILE_EXTENSION := $(join -x64,$(OBJECT_FILE_EXTENSION))
    else
        # Unknown
        $(error Architecture type is unknown ("$(AVEXTENSIONS_SETUP_ARCHITECTURE)"); open a new terminal)
    endif
endif








# Compile everything
.PHONY : Everything
Everything : $(TARGET_DIRECTORY)\$(TARGET_NAME)



# Cleaning (cleans object files)
.PHONY : Clean
Clean :
	@del /S /Q ".\$(OBJECT_DIRECTORY)\*" > NUL 2> NUL
	@echo Cleaned



# Debug (enables output)
.PHONY : Out
Out : Everything
ifneq (,$(call has_word_ci,out,$(MAKECMDGOALS)))
    # Disable output hiding if debug is enabled
    OUTPUT_FLAGS :=
endif



# Main
.PHONY : Main
Main : $(call object_expand,Main)

$(call source_expand,Main.cpp) : $(call source_expand,Header.hpp)

$(call object_expand,Main) : $(call source_expand,Main.cpp)
	$(call compile_info)
	$(call compile)



# Helpers
.PHONY : Helpers
Helpers : $(call object_expand,Helpers)

$(call source_expand,Helpers.cpp) : $(call source_expand,Helpers.hpp)

$(call object_expand,Helpers) : $(call source_expand,Helpers.cpp)
	$(call compile_info)
	$(call compile)




# Graphics
.PHONY : Graphics
Graphics : $(call object_expand,Graphics)

$(call source_expand,Graphics.cpp) : $(call source_expand,Graphics.hpp)

$(call object_expand,Graphics) : $(call source_expand,Graphics.cpp)
	$(call compile_info)
	$(call compile)



# VGraphicsBase
.PHONY : VGraphicsBase
.PHONY : VGraphicsBase25
.PHONY : VGraphicsBase26
VGraphicsBase   : $(call version_expand,VGraphicsBase)
VGraphicsBase25 : $(call object_expand,VGraphicsBase25)
VGraphicsBase26 : $(call object_expand,VGraphicsBase26)

$(call source_expand,Video\VGraphicsBase.cpp) : $(call source_expand,Video\VGraphicsBase.hpp Graphics.hpp)
$(call source_expand,Video\VGraphicsBase.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,VGraphicsBase25) : $(call source_expand,Video\VGraphicsBase.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VGraphicsBase26) : $(call source_expand,Video\VGraphicsBase.cpp)
	$(call compile_info,26)
	$(call compile,26)



# Any target files are included in here
include targets.mk



# Expand target strings
INCLUDE_FUNCTIONS_EXPANDED := $(call version_expand,$(sort $(INCLUDE_FUNCTIONS)))
INCLUDE_FUNCTIONS_REQUIRED := Main Helpers Graphics VGraphicsBase25 VGraphicsBase26



# Final target
$(TARGET_DIRECTORY)\$(TARGET_NAME) : $(INCLUDE_FUNCTIONS_REQUIRED) $(INCLUDE_FUNCTIONS_EXPANDED)
	@echo Linking "$(notdir $@)"...
	@$(COMPILER) /LD /Fe$(OBJECT_DIRECTORY)\$(TARGET_NAME) $(call object_expand,$^) $(LINK_LIBRARIES) /link $(FLAGS_LINK) /out:$@ $(OUTPUT_FLAGS)
	@echo Done



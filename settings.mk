# Variables
SOURCE_DIRECTORY := src
OBJECT_DIRECTORY := obj
TARGET_DIRECTORY := builds
OBJECT_FILE_EXTENSION := .obj
TARGET_NAME := AVExtensions.dll
COMPILER := cl
FLAGS_COMPILE := /EHsc /O2
FLAGS_LINK :=
OUTPUT_FLAGS := > NUL 2> NUL
LINK_LIBRARIES := user32.lib gdi32.lib opengl32.lib glu32.lib
INCLUDE_FUNCTIONS := VCache VChannel VNoise VMostCommon VBlend VCompare VBlur AApproximate AVideoToWaveform VGetPixel VSubtitle VSimilar VSlideFPS MArchitecture



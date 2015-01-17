# VCache
.PHONY : VCache
.PHONY : VCache25
.PHONY : VCache26
VCache   : $(call version_expand,VCache)
VCache25 : $(call object_expand,VCache25)
VCache26 : $(call object_expand,VCache26)

$(call source_expand,Video\VCache.cpp) : $(call source_expand,Video\VCache.hpp)
$(call source_expand,Video\VCache.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,VCache25) : $(call source_expand,Video\VCache.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VCache26) : $(call source_expand,Video\VCache.cpp)
	$(call compile_info,26)
	$(call compile,26)



# VChannel
.PHONY : VChannel
.PHONY : VChannel25
.PHONY : VChannel26
VChannel   : $(call version_expand,VChannel)
VChannel25 : $(call object_expand,VChannel25)
VChannel26 : $(call object_expand,VChannel26)

$(call source_expand,Video\VChannel.cpp) : $(call source_expand,Video\VChannel.hpp)
$(call source_expand,Video\VChannel.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,VChannel25) : $(call source_expand,Video\VChannel.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VChannel26) : $(call source_expand,Video\VChannel.cpp)
	$(call compile_info,26)
	$(call compile,26)



# VNoise
.PHONY : VNoise
.PHONY : VNoise25
.PHONY : VNoise26
VNoise   : $(call version_expand,VNoise)
VNoise25 : $(call object_expand,VNoise25)
VNoise26 : $(call object_expand,VNoise26)

$(call source_expand,Video\VNoise.cpp) : $(call source_expand,Video\VNoise.hpp)
$(call source_expand,Video\VNoise.hpp) : $(call source_expand,Header.hpp Helpers.hpp)

$(call object_expand,VNoise25) : $(call source_expand,Video\VNoise.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VNoise26) : $(call source_expand,Video\VNoise.cpp)
	$(call compile_info,26)
	$(call compile,26)



# VMostCommon
.PHONY : VMostCommon
.PHONY : VMostCommon25
.PHONY : VMostCommon26
VMostCommon   : $(call version_expand,VMostCommon)
VMostCommon25 : $(call object_expand,VMostCommon25)
VMostCommon26 : $(call object_expand,VMostCommon26)

$(call source_expand,Video\VMostCommon.cpp) : $(call source_expand,Video\VMostCommon.hpp)
$(call source_expand,Video\VMostCommon.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,VMostCommon25) : $(call source_expand,Video\VMostCommon.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VMostCommon26) : $(call source_expand,Video\VMostCommon.cpp)
	$(call compile_info,26)
	$(call compile,26)



# VBlend
.PHONY : VBlend
.PHONY : VBlend25
.PHONY : VBlend26
VBlend   : $(call version_expand,VBlend)
VBlend25 : $(call object_expand,VBlend25)
VBlend26 : $(call object_expand,VBlend26)

$(call source_expand,Video\VBlend.cpp) : $(call source_expand,Video\VBlend.hpp)
$(call source_expand,Video\VBlend.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,VBlend25) : $(call source_expand,Video\VBlend.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VBlend26) : $(call source_expand,Video\VBlend.cpp)
	$(call compile_info,26)
	$(call compile,26)



# VCompare
.PHONY : VCompare
.PHONY : VCompare25
.PHONY : VCompare26
VCompare   : $(call version_expand,VCompare)
VCompare25 : $(call object_expand,VCompare25)
VCompare26 : $(call object_expand,VCompare26)

$(call source_expand,Video\VCompare.cpp) : $(call source_expand,Video\VCompare.hpp Helpers.hpp)
$(call source_expand,Video\VCompare.hpp) : $(call source_expand,Header.hpp Video\VGraphicsBase.hpp Graphics.hpp)

$(call object_expand,VCompare25) : $(call source_expand,Video\VCompare.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VCompare26) : $(call source_expand,Video\VCompare.cpp)
	$(call compile_info,26)
	$(call compile,26)



# VBlur
.PHONY : VBlur
.PHONY : VBlur25
.PHONY : VBlur26
VBlur   : $(call version_expand,VBlur)
VBlur25 : $(call object_expand,VBlur25)
VBlur26 : $(call object_expand,VBlur26)

$(call source_expand,Video\VBlur.cpp) : $(call source_expand,Video\VBlur.hpp Helpers.hpp)
$(call source_expand,Video\VBlur.hpp) : $(call source_expand,Header.hpp Video\VGraphicsBase.hpp Graphics.hpp)

$(call object_expand,VBlur25) : $(call source_expand,Video\VBlur.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VBlur26) : $(call source_expand,Video\VBlur.cpp)
	$(call compile_info,26)
	$(call compile,26)



# VGetPixel
.PHONY : VGetPixel
.PHONY : VGetPixel25
.PHONY : VGetPixel26
VGetPixel   : $(call version_expand,VGetPixel)
VGetPixel25 : $(call object_expand,VGetPixel25)
VGetPixel26 : $(call object_expand,VGetPixel26)

$(call source_expand,Video\VGetPixel.cpp) : $(call source_expand,Video\VGetPixel.hpp)
$(call source_expand,Video\VGetPixel.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,VGetPixel25) : $(call source_expand,Video\VGetPixel.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VGetPixel26) : $(call source_expand,Video\VGetPixel.cpp)
	$(call compile_info,26)
	$(call compile,26)



# VSubtitle
.PHONY : VSubtitle
.PHONY : VSubtitle25
.PHONY : VSubtitle26
VSubtitle   : $(call version_expand,VSubtitle)
VSubtitle25 : $(call object_expand,VSubtitle25)
VSubtitle26 : $(call object_expand,VSubtitle26)

$(call source_expand,Video\VSubtitle.cpp) : $(call source_expand,Video\VSubtitle.hpp)
$(call source_expand,Video\VSubtitle.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,VSubtitle25) : $(call source_expand,Video\VSubtitle.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VSubtitle26) : $(call source_expand,Video\VSubtitle.cpp)
	$(call compile_info,26)
	$(call compile,26)



# VSimilar
.PHONY : VSimilar
.PHONY : VSimilar25
.PHONY : VSimilar26
VSimilar   : $(call version_expand,VSimilar)
VSimilar25 : $(call object_expand,VSimilar25)
VSimilar26 : $(call object_expand,VSimilar26)

$(call source_expand,Video\VSimilar.cpp) : $(call source_expand,Video\VSimilar.hpp)
$(call source_expand,Video\VSimilar.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,VSimilar25) : $(call source_expand,Video\VSimilar.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VSimilar26) : $(call source_expand,Video\VSimilar.cpp)
	$(call compile_info,26)
	$(call compile,26)



# VSlideFPS
.PHONY : VSlideFPS
.PHONY : VSlideFPS25
.PHONY : VSlideFPS26
VSlideFPS   : $(call version_expand,VSlideFPS)
VSlideFPS25 : $(call object_expand,VSlideFPS25)
VSlideFPS26 : $(call object_expand,VSlideFPS26)

$(call source_expand,Video\VSlideFPS.cpp) : $(call source_expand,Video\VSlideFPS.hpp)
$(call source_expand,Video\VSlideFPS.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,VSlideFPS25) : $(call source_expand,Video\VSlideFPS.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VSlideFPS26) : $(call source_expand,Video\VSlideFPS.cpp)
	$(call compile_info,26)
	$(call compile,26)






# AApproximate
.PHONY : AApproximate
.PHONY : AApproximate25
.PHONY : AApproximate26
AApproximate   : $(call version_expand,AApproximate)
AApproximate25 : $(call object_expand,AApproximate25)
AApproximate26 : $(call object_expand,AApproximate26)

$(call source_expand,Audio\AApproximate.cpp) : $(call source_expand,Audio\AApproximate.hpp)
$(call source_expand,Audio\AApproximate.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,AApproximate25) : $(call source_expand,Audio\AApproximate.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,AApproximate26) : $(call source_expand,Audio\AApproximate.cpp)
	$(call compile_info,26)
	$(call compile,26)



# AVideoToWaveform
.PHONY : AVideoToWaveform
.PHONY : AVideoToWaveform25
.PHONY : AVideoToWaveform26
AVideoToWaveform   : $(call version_expand,AVideoToWaveform)
AVideoToWaveform25 : $(call object_expand,AVideoToWaveform25)
AVideoToWaveform26 : $(call object_expand,AVideoToWaveform26)

$(call source_expand,Audio\AVideoToWaveform.cpp) : $(call source_expand,Audio\AVideoToWaveform.hpp)
$(call source_expand,Audio\AVideoToWaveform.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,AVideoToWaveform25) : $(call source_expand,Audio\AVideoToWaveform.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,AVideoToWaveform26) : $(call source_expand,Audio\AVideoToWaveform.cpp)
	$(call compile_info,26)
	$(call compile,26)



# MArchitecture
.PHONY : MArchitecture
.PHONY : MArchitecture25
.PHONY : MArchitecture26
MArchitecture   : $(call version_expand,MArchitecture)
MArchitecture25 : $(call object_expand,MArchitecture25)
MArchitecture26 : $(call object_expand,MArchitecture26)

$(call source_expand,Misc\MArchitecture.cpp) : $(call source_expand,Misc\MArchitecture.hpp)
$(call source_expand,Misc\MArchitecture.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,MArchitecture25) : $(call source_expand,Misc\MArchitecture.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,MArchitecture26) : $(call source_expand,Misc\MArchitecture.cpp)
	$(call compile_info,26)
	$(call compile,26)



# VNoClear
.PHONY : VNoClear
.PHONY : VNoClear25
.PHONY : VNoClear26
VNoClear   : $(call version_expand,VNoClear)
VNoClear25 : $(call object_expand,VNoClear25)
VNoClear26 : $(call object_expand,VNoClear26)

$(call source_expand,Video\VNoClear.cpp) : $(call source_expand,Video\VNoClear.hpp)
$(call source_expand,Video\VNoClear.hpp) : $(call source_expand,Header.hpp)

$(call object_expand,VNoClear25) : $(call source_expand,Video\VNoClear.cpp)
	$(call compile_info,25)
	$(call compile,25)

$(call object_expand,VNoClear26) : $(call source_expand,Video\VNoClear.cpp)
	$(call compile_info,26)
	$(call compile,26)



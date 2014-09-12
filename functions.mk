# Expand an include target to its 2 versions if necessary
# Usage:
#	$(call version_expand,Target_Names_With_Or_Without_Version)
# Examples:
#	$(call version_expand,Something) -> "Something25 Something26"
#	$(call version_expand,Something25) -> "Something25"
version_expand = $(foreach x,$(1),$(if $(filter %25 %26,$(x)),$(x),$(x)25 $(x)26))



# Expand version targets to their proper filenames
# Usage:
#	$(call object_expand,Target_Names_With_Version)
# Example:
#	$(call object_expand,Something25) -> "$(OBJECT_DIRECTORY)\Something25$(OBJECT_FILE_EXTENSION)"
object_expand = $(addprefix $(OBJECT_DIRECTORY)\,$(addsuffix $(OBJECT_FILE_EXTENSION),$(patsubst %25,%-25,$(patsubst %26,%-26,$(1)))))



# Expand sources to contain the source directory
# Usage:
#	$(call source_expand,Source_Files_Without_Path)
# Example:
#	$(call source_expand,Something.cpp) -> "$(SOURCE_DIRECTORY)\Something.cpp"
source_expand = $(addprefix $(SOURCE_DIRECTORY)\,$(1))



# Convert a string to lower case
# Usage:
#	$(call lower_case,String_To_Be_Converted)
# Example:
#	$(call lower_case,Hello World) -> "hello world"
lower_case = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))



# Check if a string sequence has a given word in it (as an entire word)
# The _ci version is case in-sensitive
# Usage:
#	$(call has_word,Search_String,Word_To_Find)
#	$(call has_word_ci,Search_String,Word_To_Find)
# Examples:
#	$(call has_word,this is a string,Is) -> ""
#	$(call has_word,this is a string,is) -> "true"
#	$(call has_word_ci,this is a string,Is) -> "true"
#	$(call has_word_ci,this is a string,is) -> "true"
has_word = $(if $(strip $(foreach x,$(2),$(filter $(1),$(x)))),true,)
has_word_ci = $(call has_word,$(call lower_case,$(1)),$(call lower_case,$(2)))



# Display info about the compile target
# Usage:
#	$(call compile_info[,Version])
# Examples:
#	$(call compile_info) -> displays compiling info for a non-version-specific file
#	$(call compile_info,25) -> displays compiling info for a version 2.5x file
#	$(call compile_info,26) -> displays compiling info for a version 2.6x file
compile_info = @echo Compiling "$(notdir $<)"$(if $(1), for $(if $(filter 25,$(1)),2.5x,2.6x),)...



# Compile command for a target
# Usage:
#	$(call compile[,Version])
# Examples:
#	$(call compile) -> compiles for a non-version-specific target
#	$(call compile,25) -> compiles for a version 2.5x target
#	$(call compile,26) -> compiles for a version 2.6x target
compile = @$(COMPILER) /c $(FLAGS_COMPILE) /Fo$@ $(if $(1),/DPLUGIN_AVISYNTH_VERSION=$(1),) $< $(OUTPUT_FLAGS)



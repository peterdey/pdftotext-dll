file(REMOVE_RECURSE
  "libLibPng.a"
  "libLibPng.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/LibPng.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()

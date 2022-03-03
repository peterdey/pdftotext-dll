file(REMOVE_RECURSE
  "libZlib.a"
  "libZlib.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/Zlib.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()

# C++ static cast instructions. Read and apply to all generated source and header files

Do not insert static_cast to suppress signed/unsigned mismatches between Idx (long long)
and container index types. Let the compiler perform the implicit conversion unless you canp
rove that that would result in an error.
If a specific compilation unit requires -Wsign-conversion,  -Wconversion  or equivalent, suppress it
with a single #pragma at the top of that file or block rather than casting at each access site.
When a type conversion for a container index is unavoidable, cast the fully computed index
expression exactly once, at the point of container access. Prefer declaring all loop and 
index variables with a single consistent 'int' type across a translation unit so that
mixed-type arithmetic, and therefore casts, does not arise in the first place.
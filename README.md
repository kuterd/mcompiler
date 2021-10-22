# SSA Based compiler targeting x86_64


![IR dot image](https://github.com/kuterd/mcompiler/blob/master/ir_example.png?raw=true)

## Code structure:
 * `buffer.h` contains buffer utils, `dbuffer_t` range `position_t` etc.
 * `format.h` text formating, similar to printf but can output into a `dbuffer`.
 * `hashmap.h` separate chaining hashmap, intrusive, doesn't do many allocations.
 * `ir.h` *IR* definition and utils.
 * `zone_alloc.h`, a bump pointer allocator. Useful for storing a entire data structure.
 * `dot_builder.h` builds a **GraphViz** dot file, very useful.
 * `dominators.h` **dominator tree** and **dominance frontier** calculation.
 * `x86_64_assembly.h` encoder for **x86_64** assembly.
 * `parser.h`/`parser.c` AST definition and recursive descent parser.
 * `ir_creation.h` creates **non-SSA IR** from **AST**, **SSA** conversion happens later.
 * `ssa_conversion.h` is the code for converting **non SSA IR** to **SSA**
 * `elf.h` elf file handling.
 * `platform_utils.h` allocating executable memory. used for **JIT** execution.
 * `relocation.h` used for linking. 


# Generated Virtuals

## Scope

Generated virtual method helpers used by native classes to expose overridable methods to script and extension backends.

## Entry Points

- `GDVIRTUAL` macros
- `gdvirtual.gen.h`
- `gdvirtual.gen.inc`
- `make_virtuals.py`

## Flow Notes

- `make_virtuals.py` generates macro families for virtual methods with different argument and return shapes.
- Native classes use generated virtual helpers to call script overrides consistently.
- Generated files are large and should usually be edited through the generator, not by hand.

## Code Links

- `core/object/gdvirtual.gen.h`
- `core/object/gdvirtual.gen.inc`
- `core/object/make_virtuals.py`
# Resource Loading And Saving

## Scope

Resource loader and saver registries, format selection, threaded loading, callbacks, and custom script loaders/savers.

## Entry Points

- `ResourceLoader`
- `ResourceFormatLoader`
- `ResourceSaver`
- `ResourceFormatSaver`
- `ResourceLoader::load()`
- `ResourceSaver::save()`

## Flow Notes

- Loader/saver registries choose handlers by path extension and recognized resource type.
- Threaded loading tracks background tasks and must coordinate cache and cleanup state.
- Custom loaders and savers can be added by scripts or extensions.

## Code Links

- `core/io/resource_loader.h`
- `core/io/resource_loader.cpp`
- `core/io/resource_saver.h`
- `core/io/resource_saver.cpp`
- `core/io/resource_loader_constants.h`
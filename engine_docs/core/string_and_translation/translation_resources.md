# Translation Resources

## Scope

Translation data resources, optimized translation storage, PO file loading, and translation resource formats.

## Entry Points

- `Translation`
- `OptimizedTranslation`
- `TranslationLoaderPO`

## Flow Notes

- Translation resources map source strings and contexts to localized strings.
- Optimized translations use compact lookup structures for runtime use.
- PO loading bridges external localization files into engine resources.

## Code Links

- `core/string/translation.*`
- `core/string/optimized_translation.*`
- `core/io/translation_loader_po.*`
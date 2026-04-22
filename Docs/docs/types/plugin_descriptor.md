# `PluginDescriptor`

## Purpose

`PluginDescriptor` is a deterministic ABI metadata aggregate for plugin compatibility checks and interface discovery.

## Invariants and Contracts

- Descriptor fields: plugin id (`InterfaceId`), plugin ABI version (`AbiVersion`), exposed interfaces (`Slice<const InterfaceId>`).
- Valid descriptor requires: valid plugin id, non-zero ABI version, all exposed interfaces valid, no duplicate exposed interfaces.
- `Supports(requiredAbi, requiredInterface)` requires valid descriptor, ABI compatibility, and exposed interface match.
- No allocation, no hidden synchronization, non-owning interface list view.

## API Summary

- Construction: `PluginDescriptor()`, `PluginDescriptor(pluginId, abiVersion, exposedInterfaces)`.
- Factories: `Invalid()`, `FromRawUnchecked(...)`.
- Accessors: `PluginId()`, `PluginAbiVersion()`, `ExposedInterfaces()`, `InterfaceCount()`.
- Checks: `IsValid()`, `IsInvalid()`, `Exposes(interfaceId)`, `Supports(requiredAbi, requiredInterface)`.
- Mutation: `Reset()`.
- Operators: `==`, `<=>` (value semantics over fields + interface list content).
- Hash adapters: `zcore::hash::Hash<PluginDescriptor>` and `std::hash<PluginDescriptor>`.

Public include:

```cpp
#include <zcore/plugin_descriptor.hpp>
```

## Usage Example

```cpp
constexpr zcore::InterfaceId kPlugin = zcore::InterfaceId::FromLiteral("zcore.plugin.reader_impl.v1");
constexpr zcore::InterfaceId kReader = zcore::InterfaceId::FromLiteral("zcore.io.reader.v1");
const zcore::InterfaceId interfaces[] = {kReader};
const zcore::PluginDescriptor descriptor(kPlugin, zcore::AbiVersion(1U, 0U), zcore::Slice<const zcore::InterfaceId>(interfaces));
const bool supported = descriptor.Supports(zcore::AbiVersion(1U, 0U), kReader);
```

## Warnings and Edge Cases

- Exposed interface slice is non-owning; caller owns backing storage lifetime.
- Equality compares interface list content and order; reorder changes descriptor equality/hash.

## Thread-Safety and Ownership Notes

- Pure value type; thread behavior follows normal copy-by-value semantics.

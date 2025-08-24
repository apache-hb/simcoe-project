# Build notes - 2024-10-30

## Game Progression

444 files changed, 27377 insertions(+), 9442 deletions(-)

| Feature                      | Status      | Estimated Effort |
|------------------------------|-------------|------------------|
| VIC20 Palette Drawing        | Complete    |                  |
| VIC20 Character Maps         | In Progress | 2 days           |
| VIC20 Screen Region          | In Progress | 2 days           |
| Game Audio - Music - Effects | Incomplete  | 4 days           |
| Input                        | Complete    |                  |
| Input rebinding              | Incomplete  | 1 day            |
| Enemy AI                     | Incomplete  | 1/2 day          |
| Persistent state - score     | Incomplete  | 1/2 day          |
| Menu                         | Incomplete  | 2 days           |

## Testing

Test coverage has increased from 6 tests covering 2 modules to 38 tests covering 9 modules and 1 game module.

A Complete test log can be found attached.

## Benchmarks

2 benchmarks have been added to track performance of key components, the async structured logger `logs/appenders/benchmark`, and the new high level render component `draw`.

### Async Logging Benchmark Results

All tests have been conducted with the following hardware.

| Component | Name                | Parameters                                 |
|-----------|---------------------|--------------------------------------------|
| CPU       | EPYC 9654           | 360W TDP, power determinstic configuration |
| Memory    | 384GB               | 4800MT/s 12x32GB DIMM layout               |
| Storage   | Samsung EVO 990 2TB | PCIe5.0x2 via ASUS Hyper Card              |

Structured logging message submission benchmarks.

| Format parameters | Messages submitted per second |
|-------------------|-------------------------------|
| 0                 | 8,400,000                     |
| 1                 | 8,200,000                     |

400,000 messages flushed to disk per second

## API Changes

The `render` and `draw` modules are being deprecated in favour of `render-next` and `draw-next`. These 2 new render modules are significantly simpler, more modular, and have higher test coverage.

Initial IBM DB2® support has been added to the `db` module, a richer generic database api surface, and more featured db DAO generator tool `daocc`.

## Compiler Compatibility Efforts

Work towards MSVC and mainline clang support is moving along well, currently the reflection tool is blocked on [llvm-project#86426](https://github.com/llvm/llvm-project/pull/86426). It is currently impossible to test this change, an RFC is in progress to add the necassery compiler flags.

## Work for November

1. Finish incomplete line items in [Game Progression](#game-progression).
2. Improve `render-next` and `draw-next` modules
   1. Implement `graph` module for automatic renderpass scheduling and resource state management.
   2. Implement async asset loading. Currently this is blocked on incomplete work in the `archive` module.
3. Work on `db` module.
   1. Complete support for IBM DB2®
   2. Surface currently private db specific driver interfaces in public `db` API surface.
4. Implement new archive format suitable for DirectStorage
   1. Split metadata and asset data into seperate files

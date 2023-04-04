# Hybractal

Hybractal is a tool-chain for computing and rendering hybrid fractal.

## Dependencies

1. [fractal_utils](https://github.com/ToKiNoBug/FractalUtils)
2. [libpng](http://www.libpng.org/pub/png/libpng.html)
3. [zstd](https://facebook.github.io/zstd/)
4. [Qt](https://www.qt.io/)
5. [CLI11](https://github.com/CLIUtils/CLI11)
6. [fmtlib](https://github.com/fmtlib/fmt)
7. [nlohmann json](https://github.com/nlohmann/json)
8. [yalantinglibs](https://github.com/alibaba/yalantinglibs)
9. [CUDA](https://developer.nvidia.com/zh-cn/cuda-zone)
10. [Boost.multiprecision](https://github.com/boostorg/multiprecision)

## Dots

## double

| Sequence  |             Center hex             | Critical y span | Precision |
| :-------: | :--------------------------------: | :-------------: | :-------: |
|  1011101  | 0x3813c81e03fbd53fbfe21384b148bfbf |   3.55271e-15   |     2     |
| 100101001 | 0x96034F7405FFD73FC6AAA91F09CCE6BF |    1.394-12     |     2     |


### Oct

|     Sequence      | Critical y span | Precision | Center hex                                                                                                                                                                                                                                                         |
| :---------------: | :-------------: | :-------: | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 10000000000000001 |   7.24454e-70   |     8     | 0x87b0ec26e6744c257a186b2004df61181fb382bd8715ea936acbc8386c19000004000000000000006f6c29002f706c75feffffff00696d616765666f726d617457e7fbd43703c74829f84edf7801cb1342a2976ff21ca3271a1633620017000004000000000000003063220300000000ffffffff0100000080db290300000000 |
| 10111111111111111 |      4e-71      |     8     | 0xE41DFBF55A139952D75A2E73AC9C393967012F322F21AA1D2C2824C5CA1F000004000000000000005DDC83D4A77F00000000000001000000A8FF3120FC7F00007FA144C934C73A14AD77F6DFF7428EC6A185CC5BF2AE2756601E178130100000040000000000000020EEFFFFFFFFFFFFEAFFFFFF0100000030013220FC7F0000 |
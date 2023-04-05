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

|     Sequence      | Critical y span | Precision | Center hex                                                                                                                       |
| :---------------: | :-------------: | :-------: | :------------------------------------------------------------------------------------------------------------------------------- |
| 10000000000000001 |   7.24454e-70   |     8     | 87B0EC26E6744C257A186B2004DF61181FB382BD8715EA936ACBC8386CD9FF3F57E7FBD43703C74829F84EDF7801CB1342A2976FF21CA3271A16336200E7FFBF |
| 10111111111111111 |      4e-71      |     8     | E41DFBF55A139952D75A2E73AC9C393967012F322F21AA1D2C2824C5CAFFFFBF7FA144C934C73A14AD77F6DFF7428EC6A185CC5BF2AE2756601E17813090FEBF |
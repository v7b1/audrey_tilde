# audrey_tilde
porting Audrey_II (feedback synth engine) by [Synthux Academy](https://www.synthux.academy/store/audrey-ii) to [MaxMSP](https://cycling74.com/).

Based on the original sources: https://github.com/Synthux-Academy/Audrey-II

The Audrey_II project is itself based on [DaisySP](https://github.com/electro-smith/DaisySP) by [electro-smith](https://daisy.audio/).



Audrey_II source code is published under the MIT license.

Daisy_SP is published under the MIT license.





## Building

0. Clone the code from Github, including submodules: 

   ```bash
   git clone --recurse-submodules https://github.com/v7b1/audrey_tilde.git
   ```

1. change directories (cd) into the project folder:

   ```bash
   cd audrey_tilde
   ```

2. create a folder for your various build files and step inside:

   ```bash
   mkdir build && cd build
   ```




Now you can generate the projects for your chosen build environment:

- Make: 

```bash
cmake ..
cmake --build . --config 'Release'
```

- Xcode, VScode etc.

```bash
cmake -G Xcode ..  # or cmake -G "Visual Studio 16 2019" or cmake -G Ninja ..
cmake --build .
```


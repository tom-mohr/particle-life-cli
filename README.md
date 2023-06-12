# Particle Life in the Terminal!

The `particle-life` command for creating beautiful simulations in the terminal.

If you don't know what Particle Life is, watch [this video](https://youtu.be/p4YirERTVF0) (showcase) and [this video](https://youtu.be/scvuli-zcRc) (detailed explanation).

# Installation

## Chocolatey (Windows)

```sh
choco install particle-life-cli
```

## apt (Linux)

```sh
sudo add-apt-repository ppa:tom-mohr/particle-life
sudo apt-get update
sudo apt-get install particle-life-cli
```

## Manual Download (Windows, Linux)

1. Visit the [latest release](https://github.com/tom-mohr/particle-life-cli/releases/latest) page on GitHub.
2. Download the appropriate binary for your operating system:
   - For Windows, download `particle-life.exe`.
   - For Linux, download `particle-life`.

**Note:** If you want to have access to the `particle-life` command from anywhere in your system, you can add the folder containing the binary to your system's PATH environment variable. This will allow you to run particle-life from any location in the command line. 

# Usage

After installation, you can execute the `particle-life` command from a terminal:
```sh
particle-life
```

Press `q` to quit.

## GUI Commands

| Command |  |
|---|---|
| `q` | quit |
| `<Space>` | pause |
| `i` | show info UI |
| `I` | show debug UI |
| `r <float><Enter>` | set rmax |
| `t <float><Enter>` | set dt |
| `z <float><Enter>` | set zoom |
| `zz` | set zoom = 1.0 to fit smaller screen dim |
| `Z` | set zoom to fit larger screen dim |
| `a <int>` | set attraction mode |
| `aa` | apply current attraction mode |
| `p <int>` | set position mode |
| `pp` | apply current position mode |
| `c <int>` | set color mode |
| `x <string><Enter>` | set display symbols (ordered by particle density) |
| `d` | disable clearing of colors |

| Attraction Mode |   |
|---|---|
| 1 (default) | random |
| 2 | snakes |

| Position Mode |   |
|---|---|
| 1 | uniform |
| 2 (default) | centered |
| 3 | horizontal |
| 4 | spiral |

| Color Mode |   |
|---|---|
| 0 | black & white |
| 1 (default) | color |

## CLI Options

All UI controls have a corresponding CLI option.
To see all available options, run `particle-life -h`.
For example, the "-p <mode>" option will set the position mode upon startup:
```sh
particle-life -p 4
```
This initializes particle position with a spiral,
as if the user had pressed `p`, followed by `4`:
```
         8O.
     oOo:  oo:o
   :O.         OO
   @            ::
   0.     @      .
    @0   @@
      0o8:
```

Instead of having the interactive GUI open, you can also simply print the output to stdout with the `-o` flag.
Use `-O` instead if you want the frame to be rendered in-place in the terminal, but still in stdtout.

If you only want the program to display a single frame and then quit, add the `-q` flag.
This allows you to do something like this:
```sh
C:\users\tom> particle-life -oq | tee frame.txt
```
This will write the first rendered frame into a new file `frame.txt`.

If no `-z <float>` option is given, the zoom is set to fit the larger screen dimension,
as if the user had pressed `Z`.

# Build

Build executable:
```sh
./build
```

Run executable:
```sh
dist/linux/particle-life
```
or, on Windows:
```sh
dist/windows/particle-life
```

Notes:

- To build on unix, make sure that `ncurses` is installed. E.g. on Ubuntu like this:
  ```sh
  sudo apt-get install libncurses-dev
  ```
  This uses `gcc` to compile the C-code and links it with `ncurses` (using the `-lncurses` option).
  Unlike on Windows, `getopt` should be implicitly available.
- On Windows, the build uses `gcc` to compile the C-code and links it with `pdcurses` and `getopt`,
  since both these libraries are not being shipped with Windows.
  They are therefore included in the source code, i.e. you don't have to do anything except run the build script.
  I'm using this implementation of getopt for Windows:
  [Chunde/getopt-for-windows](https://github.com/Chunde/getopt-for-windows).

# Contributing

Contributions are welcome!<br>
For feature requests and bug reports create an issue in the [issues page](https://github.com/tom-mohr/particle-life-cli/releases/latest).

# License

This project is licensed under the [MIT License](https://github.com/tom-mohr/particle-life-cli/blob/main/LICENSE).

# Contact

Feel free to reach out to me with any questions or suggestions:

- [Twitter](https://twitter.com/tom_mohr_)


## Setup

```sh
# Clone repository and initialize submodules.
git clone https://github.com/adobko/LearnWGPU.git
cd LearnWGPU/
git submodule update --init
```

### On Arch Linux
```sh
sudo pacman -S cmake emscripten npm
```

## Build
### Native
```sh
bash scripts/build.sh
```
- for a debug build
```sh
bash scripts/build.sh --debug
```
### Web
```sh
bash scripts/build-web.sh
```
- for a debug build
```sh
bash scripts/build-web.sh --debug
```
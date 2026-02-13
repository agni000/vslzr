# vslzr

A music visualizer built in Raylib. The program loads a stereo music file and renders the left and right audio channels as waveforms (red and blue).

## Requirements

- C compiler (gcc)
- [raylib](https://www.raylib.com/) library

## Build and Run 

Clone the repo and use the provided makefile: 

```text
make build 
```

Add a file inside <code>assets/</code> and then run the executable:

```text
./vslzr <filename.extension>
```

## Controls

- SPACE: Pause/Resume
- Q/ESC: Quit
- L: Toggle left channe
- R: Toggle right channel 

## TODO

- Add frequency spectrum option (Fast Fourier Transform);
- Improve/simplify buffer swapping logic;

## License

MIT License - feel free to use and modify.

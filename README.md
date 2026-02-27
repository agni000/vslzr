# vslzr

A music visualizer built in Raylib. The program loads a stereo music file and renders the left and right audio channels as waveforms (red and blue).

## Demo

https://github.com/user-attachments/assets/e10c05b8-9a04-4e42-895c-fde31642520a

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
- L: Toggle left channel
- R: Toggle right channel 

## TODO

- ~~Add frequency spectrum option (Fast Fourier Transform)~~;
- Optimize fft; 
- Improve buffer swapping logic;
- Remove potential thread issues; 
- Support drag and drop; 
- Implement music queue;

## License

MIT License - feel free to use and modify.

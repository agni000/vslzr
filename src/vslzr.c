#include <stdio.h> 
#include <stdlib.h>
#include <raylib.h> 
#include <string.h>
#include <stdint.h>
#include <float.h>
#include <math.h>
#include <stdatomic.h>
#include <assert.h>
#include <stdbool.h>
#include <complex.h>

#define WIDTH 900
#define HEIGHT 600 
#define FPS 60
#define BUFFER_SIZE 2048
#define HALF_BUFFER 1024

typedef struct {
  float samples[BUFFER_SIZE]; 
  _Atomic size_t framesCount; 
} MusicBuffer;

static MusicBuffer globalSamplesBuffer[2] = {0}; 
static _Atomic int writeIndex = 0;
static _Atomic int readIndex = 1;
static _Atomic bool isBufferReady = false;

static bool showLeftCh = true;
static bool showRightCh = true;
static const float pi = atan2f(1, 1)*4; 

/* Functions header */
void renderSamples();
void processorCallback(void *bufferData, unsigned int frames);
void transform(float complex fftOutL[], float complex fftOutR[]);
void fft(float in[], size_t stride, float complex out[], size_t samples);
void renderFFT(float complex fftOutL[], float complex fftOutR[], size_t samples);
void drawRecFFT(float complex out[], size_t numBins, float count, bool isLeft);
Color customColor(float mag);

int main(int argc, char *argv[]) { 
  if (argc != 2) {
    printf("Usage: %s <music_file>\n", argv[0]);
    return 1;
  } 
  
  float complex fftOutL[HALF_BUFFER] = {0}; 
  float complex fftOutR[HALF_BUFFER] = {0};

  InitWindow(WIDTH, HEIGHT, "vslzr"); 
  InitAudioDevice(); 
  SetTargetFPS(FPS);
 
  char filepath[256];
  snprintf(filepath, sizeof(filepath), "./assets/%s", argv[1]);
  Music music = LoadMusicStream(filepath);
  assert(music.stream.channels == 2);

  SetMusicVolume(music, 0.75f);
  PlayMusicStream(music); 
  AttachAudioStreamProcessor(music.stream, processorCallback);
  
  while(!WindowShouldClose()) {
    UpdateMusicStream(music);
 
    BeginDrawing(); 
    ClearBackground(BLACK);

    renderSamples(); 
    transform(fftOutL, fftOutR);

     if (IsKeyPressed(KEY_SPACE)) {
      if (IsMusicStreamPlaying(music)) {
        PauseMusicStream(music); 
      } else {
        ResumeMusicStream(music);
      }
    }

    if (IsKeyPressed(KEY_Q)) break; 
    if (IsKeyPressed(KEY_L)) showLeftCh = !showLeftCh; 
    if (IsKeyPressed(KEY_R)) showRightCh = !showRightCh;

    const char *leftStatus = showLeftCh ? "ON" : "OFF";
    const char *rightStatus = showRightCh ? "ON" : "OFF";
    DrawText(TextFormat("Left (L): %s", leftStatus), 10, 10, 10, RED);
    DrawText(TextFormat("Right (R): %s", rightStatus), 100, 10, 10, BLUE);
    DrawText(TextFormat("Pause(SPACE)"), 190, 10, 10, WHITE);
    DrawText(TextFormat("Quit(Q)"), 280, 10, 10, WHITE);
    EndDrawing(); 
  }
 
  DetachAudioStreamProcessor(music.stream, processorCallback);
  UnloadMusicStream(music);
  CloseAudioDevice();  
  CloseWindow();

  return 0;
}

/*@brief Callback function to process audio
  @params bufferData contains the interleaved samples (frames*2 size in this case), e.g. [LRLRLR...];
          frames is the number of frames 
  */
void processorCallback(void *bufferData, unsigned int frames) { 
  if (frames == 0) return;

  if (frames*2 > BUFFER_SIZE) {
    frames =  BUFFER_SIZE/2; 
  }

  float *samplesBuffer = (float *)bufferData; 

  int wi = atomic_load_explicit(&writeIndex, memory_order_acquire); 
  memcpy(globalSamplesBuffer[wi].samples, samplesBuffer, (frames*2) * sizeof(float)); 
  atomic_store_explicit(&globalSamplesBuffer[wi].framesCount, frames, memory_order_release); 

  atomic_store_explicit(&isBufferReady, true, memory_order_release);
}

/*@brief Render the samples left(red) and right(blue) each one as tiny rectangles
 */
void renderSamples() {
  int width = GetRenderWidth();
  
  int ri = atomic_load_explicit(&readIndex, memory_order_acquire);
  size_t frames = atomic_load_explicit(&globalSamplesBuffer[ri].framesCount, memory_order_acquire);
 
  if (frames == 0) return; 

  float recProp = (float)width/(frames); 
  
  int previewH = 80;
  int previewY = 40;
  DrawRectangle(0, previewY, width, previewH, Fade(LIGHTGRAY, 0.15f));

  for (size_t i = 0; i < frames; i++) {
    /* each frame has two samples, left and right */
    float lsNorm = globalSamplesBuffer[ri].samples[i*2];
    float rsNorm = globalSamplesBuffer[ri].samples[(i*2) + 1];

    float xRecStart = recProp*i;

    if (showLeftCh) DrawRectangle(xRecStart, ceilf((previewH) - (previewH)*fabs(lsNorm/2)), 1, (previewH)*fabs(lsNorm/2), Fade(RED, 0.75f)); 
    if (showRightCh) DrawRectangle(xRecStart, ceilf(previewH), 1, (previewH)*fabs(rsNorm/2), Fade(BLUE, 0.75f)); 
  }
}

/*@brief Calls the fft for both left and right side ~ I need to check for thread issues possibility related to buffer swapping
  @params left and right output vectors */
void transform(float complex fftOutL[], float complex fftOutR[]) {
  if (atomic_load_explicit(&isBufferReady, memory_order_acquire)) {
    /* swap */
    int wi = atomic_load(&writeIndex);
    int ri = atomic_load(&readIndex); 

    atomic_store(&readIndex, wi); 
    atomic_store(&writeIndex, ri); 

    atomic_store(&isBufferReady, false);
  }

  int ri = atomic_load_explicit(&readIndex, memory_order_acquire);
  size_t frames = atomic_load_explicit(&globalSamplesBuffer[ri].framesCount, memory_order_acquire); 
  size_t fftSamples = 1;
  while (fftSamples*2 <= frames) fftSamples *= 2;

  float samplesLeft[HALF_BUFFER] = {0};
  float samplesRight[HALF_BUFFER] = {0}; 
  
  for (size_t i = 0; i < frames; i++) {
    samplesLeft[i] = globalSamplesBuffer[ri].samples[i*2]; 
    samplesRight[i] = globalSamplesBuffer[ri].samples[i*2 + 1]; 
  }

  fft(samplesLeft, 1, fftOutL, fftSamples);
  fft(samplesRight, 1, fftOutR, fftSamples); 
  renderFFT(fftOutL, fftOutR, fftSamples); 
}

/* @brief Cooley-Tukey Fast Fourier transform algorithm 
   @params input and output vectors; number of samples; step size (stride) between consecutive samples */
void fft(float in[], size_t stride, float complex out[], size_t samples) {
  if (samples == 1) {
    out[0] = in[0]; 
    return; 
  }

  fft(in, stride*2, out, samples/2); 
  fft(in + stride, stride*2, out + samples/2, samples/2); 

  for (size_t k = 0; k < samples/2; k++) {
    float t = (float)k/samples; 
    float complex v = cexp(-2*I*pi*t)*out[k + samples/2]; 
    float complex e = out[k]; 
    out[k] = e + v; 
    out[k + samples/2] = e - v; 
  }
}

/*@brief render the bins after the DSP 
  @params left and right output vectors; number of samples */
void renderFFT(float complex fftOutL[], float complex fftOutR[], size_t samples) {
  size_t numBins = samples/4;
  float count = numBins/2;
  
  drawRecFFT(fftOutL, numBins, count, true); /* left */
  drawRecFFT(fftOutR, numBins, count + 1, false); /* right */
}

/*@brief Draw the rectangles for each side 
  @params Output vector; number of bins; count variable to control x position; isLeft to check side */
void drawRecFFT(float complex out[], size_t numBins, float count, bool isLeft) {
  int width  = GetRenderWidth();
  int height = GetRenderHeight();
  float binWidth = (float)width/numBins;

  if (isLeft) {
    for (size_t i = 0; i < numBins; i++) {
      /* normalize */ 
      float mag = cabsf(out[i])/(numBins);  
      mag = logf(1.0f + mag);
      
      float barHeight = (mag)*height*0.6f; 
      if (barHeight > 300.0f) barHeight = (float)GetRandomValue(270.0f, 300.0f); 
      
      float x = (count*binWidth);
      count--;

      Color col = customColor(mag); 

      DrawRectangle(x, 550 - (barHeight), binWidth/2, barHeight, col);
    }
    return; 
  }

  for (size_t i = 0; i < numBins; i++) {
    /* normalize */ 
    float mag = cabsf(out[i])/(numBins);
    mag = logf(1.0f + mag);

    float barHeight = mag*height*0.6f;
    if (barHeight > 300.0f) barHeight = (float)GetRandomValue(270.0f, 300.0f);
 
    float x = (count*binWidth);
    count++;

    Color col = customColor(mag); 

    DrawRectangle(x, 550 - (barHeight), binWidth/2, barHeight, col);
  }
}

/*@brief Create a custom color based on signal magnitude
  @params Signal magnitude 
  @return Custom color */
Color customColor(float mag) {
    unsigned char r = (unsigned char)(255.0f*mag);
    unsigned char g = (unsigned char)(160.0f*(1.0f - mag));
    unsigned char bcol = (unsigned char)(255.0f*(1.0f - 0.50f*mag));
    Color col = (Color){ r, g, bcol, 255 };
    return col; 
}

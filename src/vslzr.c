#include <stdio.h> 
#include <stdlib.h>
#include <raylib.h> 
#include <string.h>
#include <stdint.h>
#include <float.h>
#include <math.h>
#include <stdatomic.h>
#include <assert.h>

#define WIDTH 900
#define HEIGHT 600 
#define FPS 60
#define BUFFER_SIZE 2048

typedef struct {
  float samples[BUFFER_SIZE]; 
  _Atomic size_t framesCount; 
} MusicBuffer;

MusicBuffer globalSamplesBuffer[2] = {0}; 
_Atomic int writeIndex = 0;
_Atomic int readIndex = 1;  

void renderSamples();
void processorCallback(void *bufferData, unsigned int frames);

int main(int argc, char *argv[]) { 
  if (argc != 2) {
    printf("Usage: %s <music_file>\n", argv[0]);
    return 1;
  } 

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
    ClearBackground(WHITE);
    renderSamples(); 

    if (IsKeyPressed(KEY_SPACE)) {
      if (IsMusicStreamPlaying(music)) {
        PauseMusicStream(music); 
      } else {
        ResumeMusicStream(music);
      }
    }

    if (IsKeyPressed(KEY_Q)) {
      break; 
    }

    EndDrawing(); 
  }
 
  DetachAudioStreamProcessor(music.stream, processorCallback);
  UnloadMusicStream(music);
  CloseAudioDevice();  
  CloseWindow();

  return 0;
}

/*@brief Callback function to process audio
  @params bufferData, as far as I know, contains the interleaved samples (frames * 2 size in this case), e.g. [LRLRLR...];
          frames is the number of frames 
  */
void processorCallback(void *bufferData, unsigned int frames) { 
  if (frames == 0) return;

  if (frames * 2 > BUFFER_SIZE) {
    frames =  BUFFER_SIZE / 2; 
  }

  float *samplesBuffer = (float *)bufferData; 

  int wi = atomic_load_explicit(&writeIndex, memory_order_acquire); 
  memcpy(globalSamplesBuffer[wi].samples, samplesBuffer, (frames * 2) * sizeof(float)); 
  atomic_store_explicit(&globalSamplesBuffer[wi].framesCount, frames, memory_order_release); 

  int ri = atomic_load_explicit(&readIndex, memory_order_acquire);
  atomic_store_explicit(&writeIndex, ri, memory_order_release);
  atomic_store_explicit(&readIndex, wi, memory_order_release);
}

/*@brief Render the samples left(red) and right(blue) each one as tiny rectangles
 */
void renderSamples() {
  int width = GetRenderWidth();
  int height = GetRenderHeight();  

  int ri = atomic_load_explicit(&readIndex, memory_order_acquire);
  size_t frames = atomic_load_explicit(&globalSamplesBuffer[ri].framesCount, memory_order_acquire);
 
  if (frames == 0) return; 

  float recProp = (float)width / (frames); 

  for (size_t i = 0; i < frames; i++) {
    /* each frame has two samples, left and right */
    float lsNorm = globalSamplesBuffer[ri].samples[i * 2];
    float rsNorm = globalSamplesBuffer[ri].samples[(i * 2) + 1];

    float xRecStart = recProp * i; 
    //float xRecEnd = recProp * (i + 1); 
    //float recWidth = xRecEnd - xRecStart; 
    
    if (lsNorm > 0) { 
      DrawRectangle(xRecStart, ceilf((height / 2) - (height / 2) * lsNorm), 1, (height / 2) * lsNorm, RED); 
    } else {
      DrawRectangle(xRecStart, ceilf((height / 2) - (height / 2) * fabs(lsNorm)), 1, (height / 2) * fabs(lsNorm), RED);
    }
    if (rsNorm > 0) { 
      DrawRectangle(xRecStart, ceilf((height / 2)), 1, (height / 2) * rsNorm, BLUE); 
    } else {
      DrawRectangle(xRecStart, ceilf((height / 2)), 1, (height / 2) * fabs(rsNorm), BLUE);
    }
  }
}

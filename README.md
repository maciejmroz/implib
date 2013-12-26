implib
=========

**implib** stands for "image processing library". I created it as a proof of concept as part of my M.Sc. thesis work, which was on image processing using GPUs.

  - It was created at the time when pixel shaders 2.x were cutting edge, and 3.0 was just a paper spec, and using GPU for general purpose computations was rather new concept 
  - Generalized memory access and integer arithmetic in shaders (that we have today) was science fiction :)

The code contains reference implementation of some basic image processing/procedural image generation and composition algorithms, done on both CPU and GPU side. CPU implementation is using C++ with some MMX/SSE optimizations, no multithreading. GPU implementation is using Direct3D 9 API and HLSL (no raw shaders are used).

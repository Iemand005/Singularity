# Singularity

Singularity is a lightweight, native AI inference application for running generative AI models locally. It provides a native desktop interface for LLM inference and Stable Diffusion image generation, with GPU acceleration and no cloud service required

## Why Singularity?

Unlike most tools, this one does not rely on Electron, python or any web UI. It's fully native and focused on keeping the UI close to the runtime.

It is intended to offer inference support to lower end devices as well, supporting Intel integrated graphics through Vulkan with shared memory support explicitly enabled for both LLM and Stable Diffusion inference. This way you can load larger SD models allowing the excess memory to overflow into shared VRAM.

## Offline

The client is designed to be offline-first. You own your AI, your chats, your models. Everything stays on your device.

OpenAI API is supported as well by the core runtime library. You can use your api key to do remote inference if your device isn't powerufl enough.
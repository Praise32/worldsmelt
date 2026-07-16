# 10 — Fonti verificate

Aggiornamento: 16 luglio 2026.

## Pixel art e riferimento di qualità

- [Retro Diffusion — sito ufficiale](https://www.retrodiffusion.ai/): modelli, reference, animazioni, sprite sheet, editing, palette, tileset e prezzi del servizio.
- [Retro Diffusion — API examples](https://github.com/Retro-Diffusion/api-examples): esempi ufficiali di integrazione.
- [Retro Diffusion — termini](https://www.retrodiffusion.ai/assets/terms-8312c717.pdf): da ricontrollare prima di qualunque uso dei suoi output come training data.

## Dataset e licenze

- [Kenney Support](https://kenney.nl/support): gli asset delle pagine asset sono dichiarati CC0; logo escluso.
- [OpenGameArt FAQ](https://opengameart.org/node/5571): licenze per asset e avvertenza sulle preview.
- [CC0 1.0 Legal Code](https://creativecommons.org/publicdomain/zero/1.0/legalcode.en): testo ufficiale CC0.
- [Hugging Face Dataset Cards](https://huggingface.co/docs/hub/datasets-cards): metadati, licenza e documentazione dei dataset.
- [Superpowers Asset Packs](https://github.com/sparklinlabs/superpowers-asset-packs): repository CC0.
- [OpenDuelyst](https://github.com/open-duelyst/duelyst): repository del corpus CC0.
- [Urizen 1Bit Tileset](https://vurmux.itch.io/urizen-onebit-tileset): pagina ufficiale del pack.
- [Ninja Adventure](https://pixel-boy.itch.io/ninja-adventure-asset-pack): pagina ufficiale del pack.

## Stable Diffusion

- [Hugging Face Diffusers — LoRA](https://huggingface.co/docs/diffusers/main/training/lora): training LoRA e riferimento temporale su 2080 Ti.
- [Hugging Face Diffusers — ControlNet](https://huggingface.co/docs/diffusers/training/controlnet): requisiti e ottimizzazioni di memoria.
- [Hugging Face Diffusers — training overview](https://huggingface.co/docs/diffusers/training/overview): script ufficiali di training.
- [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp): runtime C/C++ locale.
- [LCM-LoRA SD 1.5](https://huggingface.co/latent-consistency/lcm-lora-sdv1-5): adapter ufficialmente pubblicato sul model hub.

## Qwen e llama.cpp

- [Qwen2.5-Coder-7B-Instruct GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct-GGUF): quantizzazioni ufficiali.
- [Qwen2.5 con llama.cpp](https://qwen.readthedocs.io/en/v2.5/run_locally/llama.cpp.html): uso e conversione GGUF.
- [Qwen 2.5 e ms-swift](https://qwen.readthedocs.io/en/v2.5/training/RL/ms_swift.html): framework di training con LoRA e Q-LoRA.
- [Qwen fine-tuning profiles](https://github.com/QwenLM/Qwen/blob/main/recipes/finetune/deepspeed/readme.md): profili indicativi della precedente famiglia 7B, non benchmark del modello Coder 2.5.
- [llama.cpp grammars](https://github.com/ggml-org/llama.cpp/blob/master/grammars/README.md): output vincolato.
- [llama.cpp speculative decoding](https://github.com/ggml-org/llama.cpp/blob/master/docs/speculative.md): documentazione ufficiale del percorso speculativo.

## Hardware AMD

- [AMD RX 5600 XT — comunicato e specifiche](https://www.amd.com/en/newsroom/press-releases/2020-1-6-amd-unveils-four-new-desktop-and-mobile-gpus-incl.html): 6 GB GDDR6.
- [ROCm Radeon Linux compatibility](https://rocm.docs.amd.com/projects/radeon-ryzen/en/latest/docs/compatibility/compatibilityrad/native_linux/native_linux_compatibility.html): GPU Radeon ufficialmente supportate.

## Noleggio GPU

- [RunPod pricing](https://www.runpod.io/pricing): listino Pods e Serverless.
- [RunPod RTX 4090](https://www.runpod.io/gpu-models/rtx-4090): 24 GB e prezzi Community/Secure.
- [RunPod RTX A6000](https://www.runpod.io/gpu-models/rtx-a6000): 48 GB e prezzi Community/Secure.
- [Vast.ai pricing documentation](https://docs.vast.ai/guides/instances/pricing): funzionamento del marketplace; prezzi dinamici.
- [Lambda on-demand](https://docs.lambda.ai/public-cloud/on-demand/): configurazioni e disponibilità.
- [Modal GPU documentation](https://modal.com/docs/guide/gpu): tipi GPU e fatturazione del servizio.

## Note sull’uso delle fonti

- Le pagine tecniche sono state preferite a blog e aggregatori.
- Le tariffe sono temporanee e vanno verificate prima di avviare il training.
- I tempi di addestramento sono stime di progetto, salvo il riferimento esplicito della documentazione Diffusers.
- La licenza di una raccolta non sostituisce il manifest per singolo file.
- Nessuna di queste note costituisce parere legale.

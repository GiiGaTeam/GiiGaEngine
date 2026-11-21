# Source notes

[Literature review](README.md) · [Search log](search-log.md) · [BibTeX](references.bib)

## Combo

Chen et al. (2023), *Maximizing the Utilization of GPUs Used by Cloud Gaming through Adaptive Co-location with Combo*, ACM SoCC. Full text checked. [DOI](https://doi.org/10.1145/3620678.3624660) · [Author PDF](https://mivenhan.github.io/publication/2023combo/2023combo.pdf)

- **§2.3.1, Table 3.** The conventional CUDA plus Tensor Core pair achieves 44.12% makespan reduction, calculated as `1 - paired_duration / (solo_A + solo_B)`. The RTX 3090 microbenchmark matches solo durations and uses register-only conventional and tensor compute. The percentage describes the time to finish both kernels, rather than the retained foreground throughput.
- **§4.1, Fig. 7.** With more resource-intensive RT kernels and managed co-location, RT plus conventional CUDA work gives 13.7% mean and 25.9% maximum makespan reduction in the tested set. RT plus Tensor Core work gives zero or negative reduction. The authors attribute that result to implicit resource contention beyond thread slots and shared-memory capacity.
- **§5.1.2.** The relaxed intra-SM policy allows game RT with background conventional CUDA work, or background CUDA with background Tensor Core work. The authors exclude pairings of their game CUDA/RT kernels with background tensor kernels under the resources used in this setup.
- **§6.2–6.3, Table 7 and Fig. 13.** Real game traces and substitute OptiX workloads on an RTX 3090 drive a study of scheduling background workloads. The reported 14.0% mean and 38.2% maximum improvement over PilotFish combine temporal reuse, intra-SM pairing, and SM partitioning. The full setup evaluates the scheduler rather than a live D3D12 raster pipeline.

## ISPA

Zhao, Cui, Chen, and Guo (2023), *ISPA: Exploiting Intra-SM Parallelism in GPUs via Fine-Grained Resource Management*, IEEE Transactions on Computers 72(5), 1473–1487. Full text checked. [DOI](https://doi.org/10.1109/TC.2022.3214088) · [Full-text copy](https://www.researchgate.net/publication/364503041_ISPA_Exploiting_Intra-SM_Parallelism_in_GPUs_via_Fine-grained_Resource_Management)

- **§3.1–3.2, Fig. 2.** A tuned CUDA-plus-tensor pair reduces total elapsed time by 45% versus the sum of solo durations. The authors also report zero reduction when they attempt to co-locate their default-input GEMM or convolution with a separate set of realistic kernels using plain streams.
- **§5, Figs. 5 and 7–8.** Persistent and elastic blocks and register controls expose enough launch resources for independently submitted conventional CUDA and Tensor Core work to share an SM. Streams provide the submission mechanism. Resource-aware kernel changes are central to the result.
- **§7.3, Figs. 15–17.** On an RTX 2080 Ti, system throughput improves by 15.3% on average for workloads with inspectable kernels and 7.1% for cuDNN workloads versus plain streams. Individual application duration also increases by 56.3% on average. Fig. 17 shows conventional and tensor activity overlapping for ResNet50 and FFT.

## Tacker

Zhao et al. (2022), *Tacker: Tensor-CUDA Core Kernel Fusion for Improving the GPU Utilization while Ensuring QoS*, IEEE HPCA. Full text checked. [DOI](https://doi.org/10.1109/HPCA53966.2022.00064) · [Author PDF](https://mivenhan.github.io/publication/2022tacker/2022tacker.pdf) · [Author code](https://github.com/sjtu-epcc/Tacker)

- **Fig. 14 and Eq. 10.** On RTX 2080 Ti and V100 hardware, the software chooses among original and fused kernels under a latency objective. The paper reports 18.6% mean background throughput improvement over Baymax, up to 41.1%. The measured intervention combines fusion and kernel reordering. The published fusion technique applies to compatible compute kernels.
- **Fig. 20.** The authors compare fusion with streams and MPS augmented with persistent blocks and added synchronization. The alternative methods overlap some pairs, while fusion gives the best overlap in their tested combinations.

## FlashAttention-3

Shah et al. (2024), *FlashAttention-3: Fast and Accurate Attention with Asynchrony and Low-precision*, NeurIPS. Full text checked. [Conference paper](https://proceedings.neurips.cc/paper_files/paper/2024/file/7ede97c3e082c6df10a8d6103a2eebd2-Paper-Conference.pdf) · [arXiv:2407.08608](https://arxiv.org/abs/2407.08608)

- **Introduction and §3, Algorithm 2.** An H100 attention kernel pipelines asynchronous WGMMA with softmax floating-point multiply-add and exponential operations. This is controlled overlap inside a single algorithm. The architecture and fine-grained scheduling matter when transferring the result to other targets.

## CRISP

Pan and Rogers (2024), *CRISP: Concurrent Rendering and Compute Simulation Platform for GPUs*, IEEE IISWC. Full text checked. [DOI](https://doi.org/10.1109/IISWC63097.2024.00011) · [Author PDF](https://engineering.purdue.edu/tgrogers/publication/pan-iiswc-2024/pan-iiswc-2024.pdf)

- **§II.** The authors identify next-frame fragment work and previous-frame DLSS post-processing as an async-compute opportunity based on their complementary use of floating-point and tensor resources.
- **§V-B, Figs. 12 and 14.** The simulator pairs Vulkan raster workloads with selected CUDA kernels, including kernels from RITnet eye segmentation. The paper does not attribute Tensor Core execution to those selected inference kernels. The DLSS/next-frame combination is a separate motivating example. [Reproduction artifact](https://zenodo.org/records/12803388)

## Atelico

- **GPU Sharing Impact.** Atelico reports a Windows 11 D3D12 renderer and Llama 3.2 3B inference scheduled in one hardware context. On an RTX 5080, the San Miguel 4K scene gives 123 FPS render-only, 122 FPS in graphics-priority mode and 121 FPS in balanced mode, with a 108 FPS 1% low in each. This is a vendor measurement. The shared-workload table reports renderer FPS without paired inference throughput or execution counters, so it cannot resolve whether inference progresses while the shader arithmetic pipeline is saturated. [Benchmark table](https://atelico.studio/ai-engine/benchmarks/#gpu-sharing-impact) · [D3D12 demo setup](https://atelico.studio/ai-engine/gpu-sharing/)

## PilotFish

Zhang et al. (2022), *PilotFish: Harvesting Free Cycles of Cloud Gaming with Deep Learning Training*, USENIX ATC. Publisher abstract and metadata checked. [USENIX paper page and full text](https://www.usenix.org/conference/atc22/presentation/zhang-wei)

- The abstract describes instrumenting graphics libraries to find idle intervals and scheduling background training work in those gaps. It reports harvesting up to 85.1% of **idle** GPU time. This is an example of the temporal-sharing alternative and provides a useful baseline for a concurrent-work experiment.

## Kitsune

Davies et al. (2025), *Kitsune: Enabling Dataflow Execution on GPUs*, arXiv preprint. Full text checked. [arXiv:2502.18403](https://arxiv.org/abs/2502.18403) · [Author PDF](https://www.nealcrago.com/wp-content/uploads/Kitsune_arXiv2025.pdf)

- **Spatial pipeline API and hardware implementation, Fig. 6.** The design adds SIMT and Tensor labels and separate arbiters to pair complementary blocks on each SM subject to residency limits. It proposes hardware scheduler changes and estimates performance. This is evidence about scheduling constraints and a possible design, rather than an available stock scheduler API.

## CUDA scheduling

- NVIDIA’s CUDA C++ Programming Guide explains concurrent kernel execution and the device capability `concurrentKernels` in §6.2.8.2. The guide also explains why registers and shared memory govern the blocks and warps that can reside together. Matrix instructions are exposed through the Warp Matrix Functions API and related architecture-specific mechanisms. [Version 12.9.2 concurrent execution and warp matrix functions](https://docs.nvidia.com/cuda/archive/12.9.2/cuda-c-programming-guide/index.html#concurrent-kernel-execution) · [Advanced Kernel Programming](https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/advanced-kernel-programming.html)

## Async compute

- Bondarev and Niverty (NVIDIA, 2021), *Advanced API Performance: Async Compute and Overlap*. The example table identifies DLSS plus acceleration-structure builds as a candidate because the work primarily uses different datapaths. The guidance recommends profiling individual pipelines and checking contention, synchronization and end-to-end results. [NVIDIA engineering guidance](https://developer.nvidia.com/blog/advanced-api-performance-async-compute-and-overlap/)

## GA102

- NVIDIA’s *Ampere GA102 GPU Architecture* whitepaper, Fig. 8, compares one *Wolfenstein: Youngblood* frame on an RTX 3080. It shows 37 ms with software ray tracing, 11 ms with graphics and RT Cores, and 6.7 ms with graphics, RT Cores and Tensor Cores with DLSS. This illustrates combined pipeline use. Changing the rendering path and workload limits what the frame-time comparison can establish. [NVIDIA whitepaper, Fig. 8](https://www.nvidia.com/content/dam/en-zz/Solutions/geforce/ampere/pdf/NVIDIA-ampere-GA102-GPU-Architecture-Whitepaper-V1.pdf#page=17)

## CUDA-in-Graphics

- NVIDIA’s *Simultaneous Compute and Graphics* describes TensorRT-RTX with D3D12 for game inference and rendering. It shows a CUDA context bound to a D3D12 compute queue with `cuCtxCreate_v4`, capability discovery through `CU_DEVICE_ATTRIBUTE_D3D12_CIG_SUPPORTED`, and a distinct `DIRECT` queue for graphics. It documents architecture- and queue-priority-dependent shared-memory limits for concurrent mode. This documents submission and support rather than a benchmark against a saturated FP32 pass. [TensorRT-RTX guide](https://docs.nvidia.com/deeplearning/tensorrt-rtx/latest/inference-library/compute-graphics.html) · [CUDA Driver API context parameters](https://docs.nvidia.com/cuda/cuda-driver-api/structCUctxCreateParams.html)
- NVIDIA’s in-game C++ inference SDK uses CiG and supports several inference backends. This is an implementation precedent for a creative or gameplay task on the rendering GPU. [NVIDIA In-Game Inferencing SDK](https://developer.nvidia.com/rtx/in-game-inferencing/)

## HLSL matrix operations

- Microsoft’s original Cooperative Vector preview exposed matrix-vector operations to HLSL, with NVIDIA describing Tensor Core acceleration on supported RTX configurations. Microsoft later deprecated that preview interface in favor of the unified Shader Model 6.10 linear algebra design. As of the review date, Microsoft documents Shader Model 6.10 under preview releases. Verify feature support and tensor use on the test adapter and driver. [Cooperative Vector preview](https://devblogs.microsoft.com/directx/cooperative-vector/) · [SM 6.9 retail and deprecation](https://devblogs.microsoft.com/directx/shader-model-6-9-retail-and-more/) · [SM 6.10 preview](https://devblogs.microsoft.com/directx/shader-model-6-10-agilitysdk-720-preview/) · [Agility SDK release list](https://devblogs.microsoft.com/directx/directx12agility/)

## Profiling

- Nsight Graphics GPU Trace exposes graphics and compute in-flight, SM occupancy, and pipeline throughput. Its context view distinguishes ordinary separate D3D12 and CUDA contexts from CiG work in a shared graphics context. Its system guide describes FMA throughput and a combined FP16-plus-tensor activity measurement. [GPU Trace system architecture](https://docs.nvidia.com/nsight-graphics/UserGuide/gpu-trace-system-architecture.html) · [GPU Trace UI Reference, CUDA Events and CiG](https://docs.nvidia.com/nsight-graphics/UserGuide/gpu-trace-ui.html)
- Nsight Compute’s triage guide identifies architecture-specific FMA and tensor pipeline metrics. This is useful for isolated CUDA profiling. Compare runtime timelines separately from potentially intrusive counter collection. [Compute Triage Guide](https://docs.nvidia.com/nsight-compute/ComputeTriage/)

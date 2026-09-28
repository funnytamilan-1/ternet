# GitHub Packages

Ternet publishes its CLI/runtime as an OCI container through GitHub Container Registry (GHCR).

Image: `ghcr.io/funnytamilan-1/ternet:latest`

The image contains the real `tnc` executable and runs the repository CTest suite before publishing.

```bash
docker pull ghcr.io/funnytamilan-1/ternet:latest
docker run --rm -v "$PWD:/workspace" ghcr.io/funnytamilan-1/ternet:latest run /workspace/Node.trn
```

Version tags are published when a Git tag matching `v*.*.*` is pushed.

This is the container distribution. The future native Ternet package registry remains separate.

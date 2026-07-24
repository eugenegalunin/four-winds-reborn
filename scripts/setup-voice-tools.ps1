param(
    [string]$Environment = ".venv-tts",
    [string]$TorchVersion = "2.11.0+cu128",
    [string]$TorchIndexUrl = "https://download.pytorch.org/whl/cu128"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$EnvironmentPath = Join-Path $Root $Environment
$Python = Join-Path $EnvironmentPath "Scripts\python.exe"

if (-not (Test-Path -LiteralPath $Python)) {
    python -m venv $EnvironmentPath
}

& $Python -m pip install --upgrade pip
& $Python -m pip install --upgrade qwen-tts

$CudaReady = & $Python -c "import torch; print('yes' if torch.cuda.is_available() else 'no')" 2>$null
if ($CudaReady -ne "yes") {
    Write-Host ""
    Write-Host "Replacing the CPU-only PyTorch package with the NVIDIA CUDA build..."
    & $Python -m pip install --upgrade --force-reinstall `
        "torch==$TorchVersion" `
        "torchaudio==$TorchVersion" `
        --index-url $TorchIndexUrl `
        --progress-bar off
}

& $Python -c "import torch; assert torch.cuda.is_available(), 'CUDA is not available'; print('CUDA ready:', torch.cuda.get_device_name(0), '| torch', torch.__version__)"

Write-Host ""
Write-Host "Voice tools are ready."
Write-Host "Use: $Python scripts\voicepack.py voicepacks\reborn-narrator\manifest.json preview --all"

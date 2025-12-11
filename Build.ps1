param(
    [switch]$Run  # enables --run or -Run or -r
)

# Paths
$ProjectPathWSL = "/mnt/d/Development/Git/Quantum"
$ImageWinPath   = "D:\Development\Git\Quantum\Build\Quantum.img"

Write-Host "=== Building Quantum (WSL) ==="

# Build inside WSL
wsl -e bash -lc "cd $ProjectPathWSL && make clean && make all"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed. Aborting."
    exit 1
}

Write-Host "`n[OK] Build completed."

# Optional run
if ($Run) {
    .\Debug
}
else {
    Write-Host "`nUse --run to automatically start QEMU."
}
